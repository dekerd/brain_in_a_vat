// Fill out your copyright notice in the Description page of Project Settings.


#include "Buildings/BVCityBase.h"

#include "AbilitySystemComponent.h"
#include "BVPlayerController.h"
#include "Characters/BVAutobotBase.h"
#include "Components/WidgetComponent.h"
#include "Data/BVCityData.h"
#include "GAS/CombatAttributeSet.h"
#include "Kismet/GameplayStatics.h"
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
		bStartsNeutral = CityData->bStartsNeutral;
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
	}

	// 점령 저항력 = 체력. MaxHealth는 UBVBuildingData(Super DA) 쪽 필드.
	if (BuildingData)
	{
		CaptureDamageTotal = FMath::Max(1.f, BuildingData->MaxHealth);
	}

	// Super::BeginPlay()가 InitDynamicMaterials()와 초기 SetIsProducing()을 이미 처리함.
	Super::BeginPlay();

	// DA의 TeamType이 Player/Enemy로 설정돼 있어도 거점은 중립으로 시작하도록 강제.
	if (bStartsNeutral)
	{
		TeamType = EBVTeam::Neutral;
	}

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
				UE_LOG(LogTemp, Warning, TEXT("[City] %s: CaptureWidget bound."), *GetName());
			}
			else
			{
				UE_LOG(LogTemp, Warning,
					TEXT("[City] %s: Overhead widget is %s, not UBVCityCaptureWidget. Check WBP parent class."),
					*GetName(), *UserWidget->GetClass()->GetName());
			}
		}
		else
		{
			UE_LOG(LogTemp, Warning,
				TEXT("[City] %s: OverheadWidgetComponent has no UserWidget. Check DA's OverheadWidgetClass."),
				*GetName());
		}
	}

	// InitWithCity가 초기 진행도를 받아 게이지에 반영. 일반 구독자도 있으면 알림.
	OnCaptureProgressChanged.Broadcast(CaptureProgress);

	// 팀 상태 기준으로 스폰 타이머와 펄스를 최종 정리(Neutral이면 둘 다 off).
	ResetSpawnTimerForCurrentTeam();
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
	// 중립 상태에선 스폰 금지. 타이머가 어떤 경로로든 발화해도 안전.
	if (TeamType == EBVTeam::Neutral) return;

	// 현재 팀에 맞는 레인을 선택해서 AssignedLane에 세팅.
	// (base SpawnUnit이 AssignedLane을 사용해 스폰 위치/방향 + 유닛 이동 경로 결정)
	switch (TeamType)
	{
	case EBVTeam::Player: AssignedLane = PlayerTeamLane; break;
	case EBVTeam::Enemy:  AssignedLane = EnemyTeamLane;  break;
	default:              AssignedLane = nullptr;        break;
	}

	Super::SpawnUnit();
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
	if (BottomVFXComponent->GetAsset() != NewAsset)
	{
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

