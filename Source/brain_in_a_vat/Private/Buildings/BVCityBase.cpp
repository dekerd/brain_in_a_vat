// Fill out your copyright notice in the Description page of Project Settings.


#include "Buildings/BVCityBase.h"

#include "AbilitySystemComponent.h"
#include "BVPlayerController.h"
#include "Characters/BVAutobotBase.h"
#include "Components/BoxComponent.h"
#include "Components/DecalComponent.h"
#include "Components/WidgetComponent.h"
#include "Data/BVCityData.h"
#include "GAS/CombatAttributeSet.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "Widget/BVCityCaptureWidget.h"

namespace
{
	// 단어 마지막 문자의 받침 여부로 목적격 조사(을/를)를 결정.
	// - 한글 음절: (유니코드 - 0xAC00) % 28 이 0이 아니면 받침 있음 → "을"
	// - 영문자: 자음으로 끝나면 "을", 모음으로 끝나면 "를" (단순 휴리스틱)
	// - 그 외(숫자/기호): 안전하게 "을" 폴백
	static FString GetObjectParticle(const FString& Word)
	{
		if (Word.IsEmpty()) return TEXT("을");

		const TCHAR Last = Word[Word.Len() - 1];

		// 한글 완성형 범위: AC00 ~ D7A3
		if (Last >= 0xAC00 && Last <= 0xD7A3)
		{
			const int32 Jongseong = (Last - 0xAC00) % 28;
			return (Jongseong > 0) ? TEXT("을") : TEXT("를");
		}

		// 영문 간단 판정.
		const TCHAR Lower = FChar::ToLower(Last);
		if (Lower >= TEXT('a') && Lower <= TEXT('z'))
		{
			const bool bIsVowel = (Lower == TEXT('a') || Lower == TEXT('e') || Lower == TEXT('i')
				|| Lower == TEXT('o') || Lower == TEXT('u'));
			return bIsVowel ? TEXT("를") : TEXT("을");
		}

		return TEXT("을");
	}
}

ABVCityBase::ABVCityBase()
{
	// 거점은 메인 기지가 아니라 점령 가능 구조물 → "Base Under Attack" 어나운스 비활성.
	// 점령 직후 잔여 적 투사체가 맞더라도 오인성 경고 음성이 재생되지 않게 함.
	bPlayBaseUnderAttackAnnouncer = false;

	// 바닥 VFX 컴포넌트 — 루트에 attached, 액터 원점(=메시 바닥 중앙)에서 재생.
	// Asset/Scale은 BeginPlay에서 DA로부터 주입.
	BottomVFXComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("BottomVFX"));
	BottomVFXComponent->SetupAttachment(RootComponent);
	BottomVFXComponent->SetRelativeLocation(FVector::ZeroVector); // 메시 바닥 중앙 = (0,0,0)
	BottomVFXComponent->SetRelativeScale3D(FVector(1.f));
	BottomVFXComponent->SetAutoActivate(false); // 팀이 정해지기 전엔 재생 안 함

	// 빌드 반경 데칼 — 평소엔 숨김. 빌드 모드 진입 시 SetBuildRadiusVisualVisible(true)로 토글.
	// DecalSize는 BeginPlay에서 DA의 BuildRadius를 읽고 (BuildRadius, BuildRadius, ThinDepth)로 세팅.
	// -90 pitch = 데칼이 아래(지면) 방향으로 투사.
	BuildRadiusDecal = CreateDefaultSubobject<UDecalComponent>(TEXT("BuildRadiusDecal"));
	BuildRadiusDecal->SetupAttachment(RootComponent);
	// 부모(도시 액터)의 Scale3D를 무시한다. 도시 메시를 키우기 위해 액터 Scale을 수정해도
	// 빌드 반경은 DA의 BuildRadius(월드 cm)로만 결정되도록 absolute scale 사용.
	// Location/Rotation은 정상 상속되어 도시 위치/yaw는 그대로 따라간다.
	BuildRadiusDecal->SetUsingAbsoluteScale(true);
	BuildRadiusDecal->SetRelativeRotation(FRotator(-90.f, 0.f, 0.f));
	BuildRadiusDecal->SetRelativeLocation(FVector::ZeroVector);
	BuildRadiusDecal->DecalSize = FVector(64.f, 1500.f, 1500.f); // BeginPlay에서 덮어씀
	BuildRadiusDecal->SetVisibility(false);
	BuildRadiusDecal->SetHiddenInGame(true);
}

const UBVCityData* ABVCityBase::GetCityData() const
{
	return Cast<UBVCityData>(BuildingData);
}

void ABVCityBase::BeginPlay()
{
	// City 전용 세팅을 DA에서 먼저 로드.
	if (const UBVCityData* CityData = GetCityData())
	{
		PostCaptureInvulnSeconds = CityData->PostCaptureInvulnSeconds;

		// 팀별 emission 색 DA에서 읽기 (DA 없으면 클래스 기본값 유지).
		PlayerEmissionColor  = CityData->PlayerEmissionColor;
		EnemyEmissionColor   = CityData->EnemyEmissionColor;
		NeutralEmissionColor = CityData->NeutralEmissionColor;

		// 바닥 VFX 에셋/스케일/오프셋/지속시간/재생속도 DA에서 읽기.
		PlayerCaptureBottomVFX = CityData->PlayerCaptureBottomVFX;
		EnemyCaptureBottomVFX  = CityData->EnemyCaptureBottomVFX;
		BottomVFXScale         = CityData->BottomVFXScale;
		BottomVFXOffset        = CityData->BottomVFXOffset;
		BottomVFXDuration      = CityData->BottomVFXDuration;
		BottomVFXPlayRate      = CityData->BottomVFXPlayRate;

		// Build / Garrison 미러링.
		BuildRadius         = CityData->BuildRadius;
		BuildableBuildings  = CityData->BuildableBuildings;
		MaxGarrison         = CityData->MaxGarrison;
		DispatchThreshold   = CityData->DispatchThreshold;

		// 빌드 반경 데칼: 사이즈는 DA의 BuildRadius로 (X=Y=반경; -90 pitch 회전 기준 X가 깊이),
		// 머티리얼은 DA에서 지정된 것을 동기 로드 후 MID로 래핑.
		// -90 pitch 후의 데칼 박스 표기: X=투사 깊이, Y/Z=가로/세로 반경.
		if (BuildRadiusDecal)
		{
			constexpr float DecalProjectionDepth = 256.f; // 지면 굴곡 흡수용 여유 깊이.

			// 월드 스케일을 강제로 (1,1,1)로 박는다. SetUsingAbsoluteScale 방식은 BP의
			// RelativeScale3D가 (10,10,10)으로 저장돼 있으면 여전히 월드 10x가 나와서 실패.
			// SetWorldScale3D는 parent world scale을 역산해서 RelativeScale3D를 조정하므로
			// BP override와 무관하게 월드 기준 스케일이 확정된다.
			BuildRadiusDecal->SetWorldScale3D(FVector(1.f));

			// DecalSize.Y/Z는 박스의 half-extent (cm). 박스 full 한 변 = 2*BuildRadius.
			// 머티리얼의 TexCoord[0]는 이 박스 투사면을 [0,1]로 정규화한 값이므로,
			// ConstantBiasScale로 [-1,1] 변환한 Length=1 원은 박스 절반 = BuildRadius까지 덮인다.
			BuildRadiusDecal->DecalSize = FVector(DecalProjectionDepth, BuildRadius, BuildRadius);
			BuildRadiusDecal->MarkRenderStateDirty(); // DecalSize 변경 강제 반영.

			if (!CityData->BuildRadiusDecalMaterial.IsNull())
			{
				if (UMaterialInterface* DecalMat = CityData->BuildRadiusDecalMaterial.LoadSynchronous())
				{
					BuildRadiusMID = UMaterialInstanceDynamic::Create(DecalMat, this);
					if (BuildRadiusMID)
					{
						BuildRadiusDecal->SetDecalMaterial(BuildRadiusMID);
					}
				}
			}
		}
	}

	// 점령 저항력 = 체력. MaxHealth는 UBVBuildingData(Super DA) 쪽 필드.
	if (BuildingData)
	{
		CaptureDamageTotal = FMath::Max(1.f, BuildingData->MaxHealth);
	}

	// 거점의 초기 팀은 BuildingData(Team 카테고리)의 TeamType을 그대로 사용.
	// DA 하나의 TeamType으로 "중립으로 시작 / 아군 본진 / 적 본진" 전부 표현 가능.
	if (BuildingData)
	{
		TeamType = BuildingData->TeamType;
	}

	// Super::BeginPlay()가 InitDynamicMaterials()와 초기 SetIsProducing()을 이미 처리함.
	// (TeamType을 위에서 먼저 세팅했으니 Super 안에서 팀 기반 초기화도 일관됨.)
	Super::BeginPlay();

	// 초기 점령 진행도: 팀에 따라 설정 (중립이면 0, 이미 특정 팀이면 풀 점령 상태로).
	CaptureProgress =
		(TeamType == EBVTeam::Player) ? +1.f :
		(TeamType == EBVTeam::Enemy)  ? -1.f : 0.f;

	// 초기 emission 색도 팀에 맞춰 적용.
	UpdateEmissionColorForCurrentTeam();

	// 초기 바닥 VFX도 팀에 맞춰 적용.
	UpdateBottomVFXForCurrentTeam();

	// 거점 전용 캡처 위젯이 오버헤드에 붙어 있으면 바인딩.
	// (DA의 OverheadWidgetClass를 UBVCityCaptureWidget 계열로 지정했을 경우)
	if (OverheadWidgetComponent)
	{
		if (UUserWidget* UserWidget = OverheadWidgetComponent->GetUserWidgetObject())
		{
			if (UBVCityCaptureWidget* CaptureWidget = Cast<UBVCityCaptureWidget>(UserWidget))
			{
				CaptureWidget->InitWithCity(this);
			}
		}
	}

	// InitWithCity가 초기 진행도를 받아 게이지에 반영. 일반 구독자도 있으면 알림.
	OnCaptureProgressChanged.Broadcast(CaptureProgress);

	// 팀 상태 기준으로 스폰 타이머와 펄스를 최종 정리(Neutral이면 둘 다 off).
	ResetSpawnTimerForCurrentTeam();

	// 시작부터 발견된 도시(플레이어 본진 등)는 공지 없이 바로 발견 상태로.
	if (bStartDiscovered && !bDiscoveredByPlayer)
	{
		bDiscoveredByPlayer = true;
		OnCityDiscovered.Broadcast(this);
	}

	// 게리슨 RallyPoint 기본값 = 도시 액터 위치 (UI에서 옮기기 전까지 유닛은 도시 중앙으로 모임).
	RallyPoint = GetActorLocation();

	// 게리슨 인원수 변경 시 자동 출격 평가가 돌도록 self-bind.
	// (AddToGarrison/RemoveFromGarrison/PruneGarrison/Dispatch 모두 OnGarrisonChanged를 쏨)
	OnGarrisonChanged.AddDynamic(this, &ABVCityBase::HandleGarrisonChangedForAutoDispatch);
}

void ABVCityBase::SetBuildRadiusVisualVisible(bool bVisible)
{
	if (!BuildRadiusDecal) return;

	// SetVisibility는 에디터 미리보기, SetHiddenInGame은 인게임 렌더링 게이트.
	// 두 개 다 토글해서 PIE/Standalone 모두에서 일관되게 보이고/숨겨지도록.
	BuildRadiusDecal->SetVisibility(bVisible);
	BuildRadiusDecal->SetHiddenInGame(!bVisible);
}

bool ABVCityBase::IsLocationWithinBuildRadius(const FVector& WorldLoc) const
{
	// XY 평면 거리만 체크 (지형 높이 차이는 무시).
	const FVector CityLoc = GetActorLocation();
	const float DX = WorldLoc.X - CityLoc.X;
	const float DY = WorldLoc.Y - CityLoc.Y;
	const float DistSq = DX * DX + DY * DY;
	return DistSq <= (BuildRadius * BuildRadius);
}

void ABVCityBase::MarkDiscoveredByPlayer()
{
	if (bDiscoveredByPlayer) return;
	bDiscoveredByPlayer = true;

	BroadcastDiscoveryAnnouncement();
	OnCityDiscovered.Broadcast(this);
}

void ABVCityBase::BroadcastDiscoveryAnnouncement() const
{
	// BroadcastCaptureAnnouncement와 동일한 이름 폴백 규칙.
	FText CityName = BuildingName;
	if (CityName.IsEmpty() && BuildingData)
	{
		CityName = BuildingData->BuildingName;
	}
	if (CityName.IsEmpty())
	{
		CityName = FText::FromString(TEXT("거점"));
	}

	const FString Name = CityName.ToString();
	const FString Particle = GetObjectParticle(Name);
	const FString MessageStr = FString::Printf(TEXT("거점 [%s]%s 발견했습니다."), *Name, *Particle);

	if (ABVPlayerController* BVPC = Cast<ABVPlayerController>(UGameplayStatics::GetPlayerController(GetWorld(), 0)))
	{
		BVPC->ShowCaptureAnnouncement(FText::FromString(MessageStr));
	}
}

void ABVCityBase::HandleHealthDepleted()
{
	// 새 점령 시스템(CaptureProgress tug-of-war)에선 HP=0은 점령과 무관.
	// 그냥 풀피로 복구해 파괴 로직을 돌지 않게 함.
	RefillHealthToMax();
	PreviousHealthRatio = 1.f;
}

void ABVCityBase::HandleDamageReceived(const AActor* Attacker, float DamageAmount)
{
	// Super가 RecordDamageFrom(Attacker)를 호출해 LastDamagerTeam 갱신.
	Super::HandleDamageReceived(Attacker, DamageAmount);

	if (DamageAmount <= 0.f) return;

	ApplyCaptureDelta(LastDamagerTeam, DamageAmount);
}

void ABVCityBase::ApplyCaptureDelta(EBVTeam AttackerTeam, float DamageAmount)
{
	if (CaptureDamageTotal <= 0.f) return;

	// 가해자 팀에 따라 진행도 방향 결정. 그 외 팀(None/Neutral)은 점령 영향 없음.
	float Delta = DamageAmount / CaptureDamageTotal;
	if (AttackerTeam == EBVTeam::Player)
	{
		// +방향 (초록)
	}
	else if (AttackerTeam == EBVTeam::Enemy)
	{
		Delta = -Delta;
	}
	else
	{
		return;
	}

	const float OldProgress = CaptureProgress;
	CaptureProgress = FMath::Clamp(CaptureProgress + Delta, -1.f, 1.f);

	// 팀 전환 판정. 현재 소유 상태에 따라 임계치가 다름:
	//  - Neutral: 진행도가 ±1에 도달해야 해당 팀으로 최초 점령
	//  - Player 소유 (progress > 0): Enemy 공격으로 progress가 0 이하로 떨어지면 즉시 Enemy 점령
	//  - Enemy  소유 (progress < 0): Player 공격으로 progress가 0 이상으로 올라오면 즉시 Player 점령
	// 즉, 재점령은 "한 번의 HP 총량 소진"만으로 이뤄짐.
	EBVTeam NewOwner = TeamType;

	switch (TeamType)
	{
	case EBVTeam::Neutral:
		if      (CaptureProgress >=  1.f - KINDA_SMALL_NUMBER) NewOwner = EBVTeam::Player;
		else if (CaptureProgress <= -1.f + KINDA_SMALL_NUMBER) NewOwner = EBVTeam::Enemy;
		break;

	case EBVTeam::Player:
		// Enemy가 공격해서 progress를 0 이하로 끌어내리면 Enemy로 바로 전환.
		if (CaptureProgress <= KINDA_SMALL_NUMBER) NewOwner = EBVTeam::Enemy;
		break;

	case EBVTeam::Enemy:
		// Player가 공격해서 progress를 0 이상으로 끌어올리면 Player로 바로 전환.
		if (CaptureProgress >= -KINDA_SMALL_NUMBER) NewOwner = EBVTeam::Player;
		break;

	default:
		break;
	}

	const bool bTeamChanged = (NewOwner != TeamType);
	const EBVTeam PrevTeam = TeamType;

	if (bTeamChanged)
	{
		TeamType = NewOwner;

		// 새 소유 팀의 "완전 점령" 상태로 progress를 고정.
		// (다음 재점령은 반대편이 이 값에서 0까지 끌어와야 함 = MaxHealth 데미지 1회분)
		CaptureProgress =
			(NewOwner == EBVTeam::Player) ? +1.f :
			(NewOwner == EBVTeam::Enemy)  ? -1.f : 0.f;

		// 풀피 복구(시각적 리셋 + 혹시 HP가 낮았던 경우 대비).
		RefillHealthToMax();
		PreviousHealthRatio = 1.f;

		ResetSpawnTimerForCurrentTeam();

		// 점령 팀에 맞게 emission 색 + 바닥 VFX 전환.
		UpdateEmissionColorForCurrentTeam();
		UpdateBottomVFXForCurrentTeam();

		// 화면 중앙 점령 메시지.
		BroadcastCaptureAnnouncement(PrevTeam, NewOwner);

		// PlayerController의 거점 슬롯 트래킹(Slot 2 = 최근 점령) 갱신.
		if (ABVPlayerController* BVPC = Cast<ABVPlayerController>(UGameplayStatics::GetPlayerController(GetWorld(), 0)))
		{
			BVPC->NotifyCityOwnershipChanged(this, PrevTeam, NewOwner);
		}

		// 이 도시를 OwningCity로 가리키는 건물들에게 팀 전환 알림.
		OnCityTeamChanged.Broadcast(PrevTeam, NewOwner);

		OnCaptured(NewOwner);
	}

	// 진행도 변화 브로드캐스트: 팀 전환이 일어났든 아니든,
	// 값이 달라졌으면 위젯(UBVCityDetailWidget 등)에게 알림.
	// 팀 전환 시엔 OwnerText 등이 새 TeamType을 읽도록 반드시 재방송 필요.
	if (bTeamChanged || !FMath::IsNearlyEqual(OldProgress, CaptureProgress))
	{
		OnCaptureProgressChanged.Broadcast(CaptureProgress);
	}
}

void ABVCityBase::SpawnUnit()
{
	// 중립 상태에선 스폰 금지.
	if (TeamType == EBVTeam::Neutral) return;
	if (!SpawnUnitClass) return;

	// MaxGarrison 도달 — 생산 일시정지 (다음 타이머 틱에서 또 체크).
	if (IsGarrisonFull()) return;

	UWorld* World = GetWorld();
	if (!World) return;

	// 스폰 반경: 건물 메시에 끼지 않도록 BoxComponent extent + 여유.
	float SpawnRadius = 200.f;
	if (BoxComponent)
	{
		const float BoxRadius = BoxComponent->GetScaledBoxExtent().X;
		SpawnRadius = BoxRadius + 100.f;
	}

	const FVector CityLoc = GetActorLocation();

	// 스폰 방향: RallyPoint가 도시 외곽이면 그쪽으로 밀어내고, 도시와 거의 같으면 forward 사용.
	FVector ToRally = FVector(RallyPoint.X - CityLoc.X, RallyPoint.Y - CityLoc.Y, 0.f);
	FVector SpawnDir2D = ToRally.IsNearlyZero(1.f)
		? FVector(GetActorForwardVector().X, GetActorForwardVector().Y, 0.f).GetSafeNormal()
		: ToRally.GetSafeNormal();
	if (SpawnDir2D.IsNearlyZero())
	{
		SpawnDir2D = FVector::ForwardVector;
	}

	FVector SpawnLocation = CityLoc + SpawnDir2D * SpawnRadius;
	SpawnLocation.Z += 50.f; // 지면 클립 방지 (부모와 동일 패턴)

	const FRotator SpawnRotation = SpawnDir2D.Rotation();
	const FTransform SpawnTransform(SpawnRotation, SpawnLocation);

	ABVAutobotBase* NewUnit = World->SpawnActorDeferred<ABVAutobotBase>(
		SpawnUnitClass,
		SpawnTransform,
		this,
		nullptr,
		ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);

	if (!NewUnit) return;

	// 팀 + Rally 정보를 빙의 전에 주입 (AI OnPossess가 바로 BB에 반영).
	NewUnit->SetGenericTeamId(GetGenericTeamId());
	NewUnit->AssignedLane = nullptr; // City는 lane을 쓰지 않음.
	NewUnit->bHasRallyDestination = true;
	NewUnit->RallyDestination = RallyPoint;

	NewUnit->FinishSpawning(SpawnTransform);

	// 스폰 직후 게리슨에 등록 (en-route 인원도 MaxGarrison 산정에 포함).
	AddToGarrison(NewUnit);
}

FVector ABVCityBase::SetRallyPoint(const FVector& NewWorldLoc)
{
	// BuildRadius 안에 들어오면 그대로, 밖이면 도시 중심에서 반경 경계로 클램프.
	FVector Clamped = NewWorldLoc;
	const FVector CityLoc = GetActorLocation();
	const float DX = NewWorldLoc.X - CityLoc.X;
	const float DY = NewWorldLoc.Y - CityLoc.Y;
	const float DistSq = DX * DX + DY * DY;
	const float MaxR = FMath::Max(1.f, BuildRadius);

	if (DistSq > MaxR * MaxR)
	{
		const float Dist = FMath::Sqrt(DistSq);
		const float Scale = MaxR / Dist;
		Clamped.X = CityLoc.X + DX * Scale;
		Clamped.Y = CityLoc.Y + DY * Scale;
		// Z는 입력 그대로 유지 (지형 높이는 호출측이 결정).
	}

	if (RallyPoint.Equals(Clamped, 1.f))
	{
		return RallyPoint; // 변화 없음 — 재방송/재이동 생략.
	}

	RallyPoint = Clamped;

	// 살아있는 게리슨 유닛에게 새 rally point으로 재이동 명령.
	for (const TWeakObjectPtr<ABVAutobotBase>& Weak : Garrison)
	{
		if (ABVAutobotBase* Unit = Weak.Get())
		{
			Unit->SetRallyDestination(RallyPoint);
		}
	}

	OnRallyPointChanged.Broadcast(RallyPoint);
	return RallyPoint;
}

int32 ABVCityBase::GetGarrisonCount()
{
	PruneGarrison();
	return Garrison.Num();
}

bool ABVCityBase::AddToGarrison(ABVAutobotBase* Unit)
{
	if (!Unit || Unit->bIsDead || !IsValid(Unit)) return false;

	// 팀 가드 — 적팀 도시에 잘못 합류하는 사고 방지.
	// City가 Neutral인 동안엔 누구도 합류 불가 (게리슨은 점령된 도시 전용 개념).
	if (TeamType == EBVTeam::Neutral) return false;
	if (Unit->GetGenericTeamId() != GetGenericTeamId()) return false;

	// 중복 등록 방지 — 멱등하게 true 반환.
	for (const TWeakObjectPtr<ABVAutobotBase>& Weak : Garrison)
	{
		if (Weak.Get() == Unit) return true;
	}

	// 가득 찼으면 거부. (PruneGarrison으로 죽은 슬롯 회수 후 재판정.)
	if (GetGarrisonCount() >= MaxGarrison) return false;

	Garrison.Add(Unit);
	OnGarrisonChanged.Broadcast(Garrison.Num());
	return true;
}

void ABVCityBase::RemoveFromGarrison(ABVAutobotBase* Unit)
{
	if (!Unit) return;

	const int32 RemovedCount = Garrison.RemoveAll(
		[Unit](const TWeakObjectPtr<ABVAutobotBase>& Weak) { return Weak.Get() == Unit; });

	if (RemovedCount > 0)
	{
		OnGarrisonChanged.Broadcast(Garrison.Num());
	}
}

void ABVCityBase::PruneGarrison()
{
	const int32 BeforeCount = Garrison.Num();

	Garrison.RemoveAll([](const TWeakObjectPtr<ABVAutobotBase>& Weak)
	{
		const ABVAutobotBase* Unit = Weak.Get();
		return !Unit || Unit->bIsDead || !IsValid(Unit);
	});

	if (Garrison.Num() != BeforeCount)
	{
		OnGarrisonChanged.Broadcast(Garrison.Num());
	}
}

void ABVCityBase::SetDispatchTargetCity(ABVCityBase* NewTargetCity)
{
	// 자기 자신 또는 nullptr은 해제로 간주 (무한 출격 루프 방지).
	if (!NewTargetCity || NewTargetCity == this)
	{
		ClearDispatchTargetCity();
		return;
	}

	ABVCityBase* OldTarget = DispatchTargetCity.Get();
	if (OldTarget == NewTargetCity)
	{
		// 같은 타겟 재지정 — 평가만 다시 돌려서 Threshold가 충족이면 발사.
		EvaluateAutoDispatch();
		return;
	}

	DispatchTargetCity = NewTargetCity;
	OnDispatchTargetChanged.Broadcast(NewTargetCity);

	// 이미 게리슨이 Threshold 이상이면 즉시 발사.
	EvaluateAutoDispatch();
}

void ABVCityBase::ClearDispatchTargetCity()
{
	if (!DispatchTargetCity.IsValid()) return;

	DispatchTargetCity.Reset();
	OnDispatchTargetChanged.Broadcast(nullptr);
}

void ABVCityBase::Dispatch()
{
	ABVCityBase* TargetCity = DispatchTargetCity.Get();
	if (!TargetCity || TargetCity == this) return;

	// 죽은/만료된 약참조 정리. (이 호출 안에서 OnGarrisonChanged가 한 번 더 쏠 수 있음 — OK.)
	PruneGarrison();
	if (Garrison.Num() == 0) return;

	const FVector TargetLoc = TargetCity->GetActorLocation();

	// 게리슨 사본을 만들어 순회 — SetRallyDestination이 BB/MoveTo를 건드릴 때
	// 컨트롤러가 다시 City API를 콜백으로 부르더라도 안전하게 끝내기 위해.
	TArray<TWeakObjectPtr<ABVAutobotBase>> Pending = Garrison;

	// 본 도시 게리슨은 즉시 비움 (Threshold 재평가 무한 루프 방지).
	Garrison.Empty();

	for (const TWeakObjectPtr<ABVAutobotBase>& Weak : Pending)
	{
		ABVAutobotBase* Unit = Weak.Get();
		if (!Unit || Unit->bIsDead) continue;

		Unit->DispatchedToCity = TargetCity;
		Unit->SetRallyDestination(TargetLoc);
	}

	// 인원수 0 알림 (UI 갱신용). 자기 자신의 자동 평가도 트리거되지만 Garrison.Num()==0이라 no-op.
	OnGarrisonChanged.Broadcast(0);

	OnDispatchFired.Broadcast(this, TargetCity);
}

void ABVCityBase::EvaluateAutoDispatch()
{
	if (DispatchThreshold <= 0) return;
	if (GetGarrisonCount() < DispatchThreshold) return;

	// MarchDirection이 있으면 우선 — 자유 각도로 진격 (DispatchTargetCity는 무시).
	if (bHasMarchDirection)
	{
		MarchNow();
		return;
	}

	// Fallback: 도시 타겟 기반 기존 출격.
	ABVCityBase* TargetCity = DispatchTargetCity.Get();
	if (!TargetCity || TargetCity == this) return;

	Dispatch();
}

void ABVCityBase::SetMarchDirection(float AngleDeg)
{
	// 0~360 범위로 정규화 (음수/초과 입력 허용).
	const float Normalized = FRotator::ClampAxis(AngleDeg);

	const bool bChanged = (!bHasMarchDirection) || !FMath::IsNearlyEqual(MarchAngleDeg, Normalized, 0.01f);

	MarchAngleDeg = Normalized;
	bHasMarchDirection = true;

	if (bChanged)
	{
		OnMarchDirectionChanged.Broadcast(MarchAngleDeg, true);
	}

	// 이미 게리슨이 Threshold 이상이면 즉시 발사.
	EvaluateAutoDispatch();
}

void ABVCityBase::ClearMarchDirection()
{
	if (!bHasMarchDirection) return;

	bHasMarchDirection = false;
	OnMarchDirectionChanged.Broadcast(MarchAngleDeg, false);
}

void ABVCityBase::MarchNow()
{
	if (!bHasMarchDirection) return;

	PruneGarrison();
	if (Garrison.Num() == 0) return;

	// 진군 목적지: 도시 위치에서 MarchDirection 방향으로 MarchDistance 떨어진 지점.
	// Yaw만 사용하고 Z는 도시 Z 그대로 (지형 높낮이는 내비메시/MoveTo가 알아서 처리).
	const FVector CityLoc = GetActorLocation();
	const FVector Dir2D = FRotator(0.f, MarchAngleDeg, 0.f).Vector();
	const FVector MarchDest(
		CityLoc.X + Dir2D.X * MarchDistance,
		CityLoc.Y + Dir2D.Y * MarchDistance,
		CityLoc.Z);

	// Dispatch()와 동일 패턴 — 사본 순회, 게리슨 먼저 비우기(재평가 루프 방지).
	TArray<TWeakObjectPtr<ABVAutobotBase>> Pending = Garrison;
	Garrison.Empty();

	for (const TWeakObjectPtr<ABVAutobotBase>& Weak : Pending)
	{
		ABVAutobotBase* Unit = Weak.Get();
		if (!Unit || Unit->bIsDead) continue;

		// 도시 타겟이 아니므로 DispatchedToCity는 비움 — TickDispatchArrival이 no-op 되도록.
		Unit->DispatchedToCity = nullptr;
		Unit->SetRallyDestination(MarchDest);
	}

	OnGarrisonChanged.Broadcast(0);

	// 기존 OnDispatchFired 채널을 재사용 (ToCity=nullptr로 "방향 진격"을 표현).
	OnDispatchFired.Broadcast(this, nullptr);
}

void ABVCityBase::HandleGarrisonChangedForAutoDispatch(int32 /*NewGarrisonCount*/)
{
	EvaluateAutoDispatch();
}

void ABVCityBase::UpdateEmissionColorForCurrentTeam()
{
	FLinearColor Target;
	switch (TeamType)
	{
	case EBVTeam::Player: Target = PlayerEmissionColor;  break;
	case EBVTeam::Enemy:  Target = EnemyEmissionColor;   break;
	default:              Target = NeutralEmissionColor; break;
	}

	SetEmissionColor(Target);
}

void ABVCityBase::UpdateBottomVFXForCurrentTeam()
{
	if (!BottomVFXComponent) return;

	// 이전 자동 정지 타이머는 반드시 클리어 (전환 중간에 옛 타이머가 새 VFX를 끄지 않도록).
	GetWorldTimerManager().ClearTimer(BottomVFXStopTimerHandle);

	// 현재 팀에 해당하는 Niagara 에셋 선택.
	UNiagaraSystem* NewAsset = nullptr;
	switch (TeamType)
	{
	case EBVTeam::Player: NewAsset = PlayerCaptureBottomVFX; break;
	case EBVTeam::Enemy:  NewAsset = EnemyCaptureBottomVFX;  break;
	default:              NewAsset = nullptr;                break; // 중립은 VFX 없음
	}

	if (!NewAsset)
	{
		// 에셋 없으면 즉시 비활성화 (잔여 파티클 포함).
		BottomVFXComponent->DeactivateImmediate();
		BottomVFXComponent->SetAsset(nullptr);
		return;
	}

	// 에셋이 바뀐 경우에만 재설정 (같으면 재시작 방지).
	// Niagara: 활성 중인 컴포넌트에 SetAsset 후 Activate(true)를 호출해도 새 에셋이
	// 깨끗이 시작되지 않는 경우가 있다 (이전 팀 VFX의 Duration이 아직 안 끝났을 때 등).
	// 안전하게 DeactivateImmediate로 비운 뒤 교체한다.
	if (BottomVFXComponent->GetAsset() != NewAsset)
	{
		BottomVFXComponent->DeactivateImmediate();
		BottomVFXComponent->SetAsset(NewAsset);
	}

	// 위치는 매번 덮어쓰기 (DA 값 변경 즉시 반영).
	// 액터 원점 = 메시 바닥 중앙 (OnConstruction의 피벗 보정과 일치).
	BottomVFXComponent->SetRelativeLocation(BottomVFXOffset);

	// 스케일은 컴포넌트 transform이 아니라 Niagara User Parameter로 전달.
	// (Component scale은 내부 에미터 오프셋까지 같이 스케일해서 위치 이상으로 보임)
	// Niagara Asset의 User Parameters에 Float "Scale" 추가 후 사이즈 계산에 곱해서 쓰면 됨.
	BottomVFXComponent->SetVariableFloat(TEXT("Scale"), BottomVFXScale);

	// 스케일 관련 컴포넌트 transform은 항상 1로 고정 (위치 왜곡 방지).
	BottomVFXComponent->SetRelativeScale3D(FVector(1.f));

	// 재생 속도(시뮬레이션 타임 스케일). 1.0=원속, 2.0=2배 빠름, 0.5=절반.
	BottomVFXComponent->SetCustomTimeDilation(BottomVFXPlayRate > 0.f ? BottomVFXPlayRate : 1.f);

	BottomVFXComponent->Activate(true); // true = 리셋해서 처음부터 재생

	// Duration > 0이면 해당 시간 후 자동 정지 (지속 재생 대신 원샷 느낌).
	// DeactivateImmediate()로 기존 파티클 포함 즉시 중단
	// (Deactivate()는 새 스폰만 멈추고 루프/잔여 파티클은 계속 재생됨).
	if (BottomVFXDuration > 0.f)
	{
		GetWorldTimerManager().SetTimer(
			BottomVFXStopTimerHandle,
			FTimerDelegate::CreateWeakLambda(this, [this]()
			{
				if (BottomVFXComponent)
				{
					BottomVFXComponent->DeactivateImmediate();
				}
			}),
			BottomVFXDuration,
			false);
	}
}

void ABVCityBase::ResetSpawnTimerForCurrentTeam()
{
	FTimerManager& TimerManager = GetWorldTimerManager();
	TimerManager.ClearTimer(SpawnTimerHandle);
	ElapsedTime = 0.f;

	const bool bWillProduce =
		(TeamType != EBVTeam::Neutral) && SpawnUnitClass && RespawnInterval > 0.f;

	// 생산 상태를 머티리얼에도 반영(펄스 on/off).
	SetIsProducing(bWillProduce);

	if (!bWillProduce) return;

	TimerManager.SetTimer(
		SpawnTimerHandle,
		this,
		&ABVBuildingBase::SpawnUnit,
		RespawnInterval,
		true,
		RespawnInterval);
}

void ABVCityBase::RefillHealthToMax()
{
	if (!ASC || !CombatAttributes) return;

	const float MaxHP = CombatAttributes->GetMaxHealth();
	if (MaxHP <= 0.f) return;

	ASC->SetNumericAttributeBase(UCombatAttributeSet::GetHealthAttribute(), MaxHP);
}

void ABVCityBase::BroadcastCaptureAnnouncement(EBVTeam PrevTeam, EBVTeam NewTeam) const
{
	// 거점 이름은 BuildingName 우선, 비어있으면 DA 것, 그것도 없으면 "거점".
	FText CityName = BuildingName;
	if (CityName.IsEmpty() && BuildingData)
	{
		CityName = BuildingData->BuildingName;
	}
	if (CityName.IsEmpty())
	{
		CityName = FText::FromString(TEXT("거점"));
	}

	// 전환 종류별 문구 (플레이어 체감 기준의 카피).
	FString MessageStr;
	const FString Name = CityName.ToString();
	const FString Particle = GetObjectParticle(Name); // 이름 끝음절 기준으로 "을" / "를" 선택.

	if (PrevTeam == EBVTeam::Neutral && NewTeam == EBVTeam::Player)
	{
		// 중립 → 아군: 최초 점령
		MessageStr = FString::Printf(TEXT("아군이 거점 [%s]%s 최초로 점령하였습니다."), *Name, *Particle);
	}
	else if (PrevTeam == EBVTeam::Neutral && NewTeam == EBVTeam::Enemy)
	{
		// 중립 → 적군: 최초 점령
		MessageStr = FString::Printf(TEXT("적군이 거점 [%s]%s 최초로 점령하였습니다."), *Name, *Particle);
	}
	else if (PrevTeam == EBVTeam::Enemy && NewTeam == EBVTeam::Player)
	{
		// 적 소유 → 아군 탈환
		MessageStr = FString::Printf(TEXT("아군이 거점 [%s]%s 탈환하였습니다."), *Name, *Particle);
	}
	else if (PrevTeam == EBVTeam::Player && NewTeam == EBVTeam::Enemy)
	{
		// 아군 소유 → 적군에게 상실 (플레이어 시점의 경고성 카피)
		MessageStr = FString::Printf(TEXT("적군의 공격으로 거점 [%s]%s 상실하였습니다."), *Name, *Particle);
	}
	else
	{
		return; // 그 외 전환은 메시지 없음
	}

	const FText MessageText = FText::FromString(MessageStr);

	if (ABVPlayerController* BVPC = Cast<ABVPlayerController>(UGameplayStatics::GetPlayerController(GetWorld(), 0)))
	{
		BVPC->ShowCaptureAnnouncement(MessageText);
	}
}

