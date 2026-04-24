// Fill out your copyright notice in the Description page of Project Settings.


#include "Buildings/BVBuildingBase.h"
#include "AI/BVLane.h"
#include "Buildings/BVCityBase.h"
#include "EngineUtils.h"

#include "BVPlayerController.h"
#include "Characters/BVAutobotBase.h"
#include "Components/WidgetComponent.h"
#include "Components/BoxComponent.h"
#include "Components/BVHealthComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Perception/AIPerceptionStimuliSourceComponent.h"
#include "Widget/BVSpawnCooltimeBar.h"
#include "GAS/CombatAttributeSet.h"
#include "Perception/AISense_Sight.h"
#include "Collision/BVCollision.h"
#include "Components/BoxComponent.h"
#include "Data/BVBuildingData.h"
#include "Data/UnitStats.h"
#include "Kismet/GameplayStatics.h"
#include "Widget/UBVBuildingOverheadWidget.h"
#include "GAS/GASTags.h"
#include "Weapons/Projectiles/BVLaserBeamBase.h"
#include "Components/DecalComponent.h"
#include "Components/BVStaticHoverRingComponent.h"
#include "Materials/MaterialInterface.h"

// Sets default values
ABVBuildingBase::ABVBuildingBase()
{
	PrimaryActorTick.bCanEverTick = true;

	// <--------------- Components ----------------> //
	// Root Component
	SceneRootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRootComponent;
	
	BoxComponent = CreateDefaultSubobject<UBoxComponent>(TEXT("Box"));
	BoxComponent->SetupAttachment(RootComponent);
	BoxComponent->InitBoxExtent(FVector(30.f, 30.f, 30.f));
	BoxComponent->SetCollisionProfileName(TEXT("Building"));
	BoxComponent->SetGenerateOverlapEvents(true);

	StaticMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMesh"));
	StaticMeshComponent->SetupAttachment(RootComponent);
	// 마우스 호버 전용. 실루엣을 정확히 잡으려고 메시 자체의 콜리전 사용.
	// 게임플레이 충돌(Projectile/Pawn/Vision 등)은 BoxComponent가 담당.
	StaticMeshComponent->SetCollisionProfileName(TEXT("Hoverable"));
	// 다른 건물/거점의 호버 링이 이 메시 위에 투영되지 않도록 차단.
	StaticMeshComponent->SetReceivesDecals(false);

	// [GAS] ASC & Attributes

	ASC = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("ASC"));
	ASC->SetIsReplicated(true);

	CombatAttributes = CreateDefaultSubobject<UCombatAttributeSet>(TEXT("CombatAttributes"));

	// Stimuli Source Component
	StimuliSourceComponent = CreateDefaultSubobject<UAIPerceptionStimuliSourceComponent>(TEXT("StimuliSource"));

	// Health Component
	HealthComponent = CreateDefaultSubobject<UBVHealthComponent>(TEXT("HealthComponent"));

	// <--------------- Assets ----------------> //

	static ConstructorHelpers::FClassFinder<UGameplayEffect> InitStatGEClass(TEXT("/Script/Engine.Blueprint'/Game/GAS/GE/GE_InitStat.GE_InitStat_C'"));
	if (InitStatGEClass.Succeeded())
	{
		InitStatsEffect = InitStatGEClass.Class;
	}

	// <--------------- Widgets ----------------> //
	
	OverheadWidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("OverheadWidget"));
	OverheadWidgetComponent->SetupAttachment(RootComponent);
	OverheadWidgetComponent->SetWidgetSpace(EWidgetSpace::World);
	OverheadWidgetComponent->SetDrawSize(FVector2D(120.f, 12.f));
	OverheadWidgetComponent->SetRelativeScale3D(FVector(1.f));
	OverheadWidgetComponent->SetUsingAbsoluteRotation(true);

	// <--------------- Hover Ring Decal ----------------> //
	// 바닥에 투영되는 호버 링. 머티리얼은 아래에서 기본 에셋을 자동 할당, 런타임에 MID로 래핑.
	HoverRingDecal = CreateDefaultSubobject<UDecalComponent>(TEXT("HoverRingDecal"));
	HoverRingDecal->SetupAttachment(RootComponent);
	// -90 Pitch: 데칼 박스의 +X(투영 방향)가 아래를 향함 → 바닥에 투영.
	HoverRingDecal->SetRelativeRotation(FRotator(-90.f, 0.f, 0.f));
	HoverRingDecal->SetVisibility(false);
	HoverRingDecal->SetHiddenInGame(true);

	// 기본 링 머티리얼(소프트 참조). 에셋 없으면 BeginPlay에서 조용히 스킵.
	HoverRingMaterial = TSoftObjectPtr<UMaterialInterface>(
		FSoftObjectPath(TEXT("/Game/Materials/M_HoverRingDecal.M_HoverRingDecal")));

	// 새 StaticMesh 기반 링 — 실제 렌더링 담당. 위 HoverRingDecal은 BP 호환성을 위해 남아있지만 미사용.
	StaticHoverRingComponent = CreateDefaultSubobject<UBVStaticHoverRingComponent>(TEXT("StaticHoverRingComponent"));
	StaticHoverRingComponent->SetupAttachment(RootComponent);
}

void ABVBuildingBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// 호버 페이드(메시 MID + 오버레이 + 스텐실)는 파괴 여부와 무관하게 갱신.
	UpdateHoverAnim(DeltaTime);

	// If destroyed, do nothing
	if (bIsDestroyed) return;

	if (RespawnInterval <= 0.f) return;

	ElapsedTime += DeltaTime;
	
	float Percent = FMath::Fmod(ElapsedTime, RespawnInterval) / RespawnInterval;

	// 리스폰 프로그레스는 빌딩 상세 패널(BVBuildingDetailWidget)에서 표시.
}

void ABVBuildingBase::DestroyBuilding()
{
	bIsDestroyed = true;

	// Disable collisions
	if (BoxComponent)
		BoxComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	if (StaticMeshComponent)
		StaticMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// Stop Respawn Timer
	GetWorldTimerManager().ClearTimer(SpawnTimerHandle);
	ElapsedTime = 0.0f;

	// 생산 중 펄스 off
	SetIsProducing(false);

	// Hide Widget
	if (OverheadWidgetComponent)
		OverheadWidgetComponent->SetVisibility(false);

	// Destroy
	SetLifeSpan(1.0f);

}

void ABVBuildingBase::FinishConstruction()
{

}

void ABVBuildingBase::HandleHealthDepleted()
{
	DestroyBuilding();
}

void ABVBuildingBase::HandleDamageReceived(const AActor* Attacker, float DamageAmount)
{
	// 기본 동작은 가해자 팀을 기록하는 것까지.
	// 파생 클래스(거점 등)는 이후 Super 호출 후 점령 로직을 더함.
	RecordDamageFrom(Attacker);
}

void ABVBuildingBase::RecordDamageFrom(const AActor* Attacker)
{
	if (!Attacker) return;

	if (const IGenericTeamAgentInterface* TeamAgent = Cast<const IGenericTeamAgentInterface>(Attacker))
	{
		const FGenericTeamId Id = TeamAgent->GetGenericTeamId();
		LastDamagerTeam = static_cast<EBVTeam>(Id.GetId());
		return;
	}

	// Pawn이면 컨트롤러에서도 확인
	if (const APawn* Pawn = Cast<APawn>(Attacker))
	{
		if (const AController* Controller = Pawn->GetController())
		{
			if (const IGenericTeamAgentInterface* ControllerAgent = Cast<const IGenericTeamAgentInterface>(Controller))
			{
				const FGenericTeamId Id = ControllerAgent->GetGenericTeamId();
				LastDamagerTeam = static_cast<EBVTeam>(Id.GetId());
			}
		}
	}
}

void ABVBuildingBase::InitDynamicMaterials()
{
	DynamicMaterials.Reset();

	if (!StaticMeshComponent) return;

	const int32 NumMaterials = StaticMeshComponent->GetNumMaterials();
	DynamicMaterials.Reserve(NumMaterials);

	// 인스턴스별 위상 오프셋. 머티리얼이 PhaseOffset 파라미터를 쓰면 펄스가 건물마다 어긋남.
	// 0~TwoPi 범위: sin/cos 기반 펄스에 그대로 더해 쓰기 좋음.
	const float InstancePhaseOffset = FMath::FRandRange(0.f, UE_TWO_PI);

	for (int32 Index = 0; Index < NumMaterials; ++Index)
	{
		UMaterialInterface* SourceMat = StaticMeshComponent->GetMaterial(Index);
		if (!SourceMat) continue;

		UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(SourceMat, this);
		if (!MID) continue;

		MID->SetScalarParameterValue(PhaseOffsetParamName, InstancePhaseOffset);

		StaticMeshComponent->SetMaterial(Index, MID);
		DynamicMaterials.Add(MID);
	}
}

void ABVBuildingBase::SetIsProducing(bool bIsProducing)
{
	const float Value = bIsProducing ? 1.f : 0.f;
	for (UMaterialInstanceDynamic* MID : DynamicMaterials)
	{
		if (MID)
		{
			MID->SetScalarParameterValue(IsProducingParamName, Value);
		}
	}
}

void ABVBuildingBase::SetEmissionColor(const FLinearColor& InColor)
{
	for (UMaterialInstanceDynamic* MID : DynamicMaterials)
	{
		if (MID)
		{
			MID->SetVectorParameterValue(EmissionColorParamName, InColor);
		}
	}
}

UAbilitySystemComponent* ABVBuildingBase::GetAbilitySystemComponent() const
{
	return ASC;	
}

void ABVBuildingBase::HandleHealthChangedForAudio(float NewHealthRatio)
{
	if (NewHealthRatio < PreviousHealthRatio)
	{
		// 거점은 기지가 아니라 점령 가능한 구조물 → 어나운스 비활성 (bPlayBaseUnderAttackAnnouncer=false).
		// 또한 팀 전환 직후 날아오던 잔여 투사체가 맞는 엣지 케이스도 이 플래그로 억제됨.
		if (bPlayBaseUnderAttackAnnouncer && TeamType == EBVTeam::Player)
		{
			if (ABVPlayerController* BVPC = Cast<ABVPlayerController>(GetWorld()->GetFirstPlayerController()))
			{
				BVPC->PlayAnnouncerVoice(EBVAnnouncerEvent::BaseUnderAttack);
			}
		}
	}

	PreviousHealthRatio = NewHealthRatio;
}

void ABVBuildingBase::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	// 데이터 에셋이 할당되어 있고, 그 안에 건물 메시(BuildingMesh)가 설정되어 있다면
	if (BuildingData && StaticMeshComponent)
	{
		if (BuildingData->BuildingMesh)
		{
			StaticMeshComponent->SetStaticMesh(BuildingData->BuildingMesh);

			// 메시 바운딩 박스를 읽어서 피벗을 바닥 중앙으로 보정.
			// X/Y는 메시 중앙, Z는 바닥면이 Actor 원점(0,0,0)에 오도록 오프셋.
			const FBox MeshBounds = BuildingData->BuildingMesh->GetBoundingBox();
			const FVector MeshCenter = MeshBounds.GetCenter();
			StaticMeshComponent->SetRelativeLocation(FVector(-MeshCenter.X, -MeshCenter.Y, -MeshBounds.Min.Z));
		}

		// DA에서 Yaw 회전 오프셋 적용 (메시 방향 보정)
		StaticMeshComponent->SetRelativeRotation(FRotator(0.f, BuildingData->BuildingYaw, 0.f));

		// DA에서 균일 스케일 적용
		const float S = BuildingData->BuildingScale;
		SetActorScale3D(FVector(S));

		// 아이콘 정보도 동기화 (UI에서 바로 쓸 수 있도록)
		if (BuildingData->BuildingIcon)
		{
			BuildingIcon = BuildingData->BuildingIcon;
		}
	}

	if (StaticMeshComponent && StaticMeshComponent->GetStaticMesh() && BoxComponent)
	{
		const FBox MeshBounds = StaticMeshComponent->GetStaticMesh()->GetBoundingBox();
		const FVector MeshCenter = MeshBounds.GetCenter();
		BoxComponent->SetBoxExtent(MeshBounds.GetExtent());
		// 메시와 동일하게 바닥+중앙 피벗 보정
		BoxComponent->SetRelativeLocation(FVector(-MeshCenter.X, -MeshCenter.Y, -MeshBounds.Min.Z));
		// 메시와 동일한 Yaw 회전 적용
		const float YawOffset = BuildingData ? BuildingData->BuildingYaw : 0.f;
		BoxComponent->SetRelativeRotation(FRotator(0.f, YawOffset, 0.f));
	}
	
}

void ABVBuildingBase::FixPivotToBottom()
{
	if (!StaticMeshComponent || !StaticMeshComponent->GetStaticMesh()) return;

	const FBox MeshBounds = StaticMeshComponent->GetStaticMesh()->GetBoundingBox();
	const FVector MeshCenter = MeshBounds.GetCenter();
	const FVector Offset(-MeshCenter.X, -MeshCenter.Y, -MeshBounds.Min.Z);

	StaticMeshComponent->SetRelativeLocation(Offset);

	// DA Yaw 오프셋도 함께 반영
	const float YawOffset = BuildingData ? BuildingData->BuildingYaw : 0.f;
	StaticMeshComponent->SetRelativeRotation(FRotator(0.f, YawOffset, 0.f));

	if (BoxComponent)
	{
		BoxComponent->SetBoxExtent(MeshBounds.GetExtent());
		BoxComponent->SetRelativeLocation(Offset);
		BoxComponent->SetRelativeRotation(FRotator(0.f, YawOffset, 0.f));
	}

#if WITH_EDITOR
	// 에디터에서 변경 사항이 dirty로 마크되어 저장 시 반영되도록.
	Modify();
	if (StaticMeshComponent) StaticMeshComponent->Modify();
	if (BoxComponent) BoxComponent->Modify();
#endif
}

FGenericTeamId ABVBuildingBase::GetTeamId_Implementation() const
{
	return GetGenericTeamId();
}

bool ABVBuildingBase::IsDestroyed_Implementation() const
{
	return bIsDestroyed;
}

// Called when the game starts or when spawned
void ABVBuildingBase::BeginPlay()
{
	Super::BeginPlay();

	// 메시 로컬 바운딩 박스의 Top 중앙점을 월드로 변환 → 메시가 시각적으로 그려지는 위치 위
	// (BoxComponent나 RelativeLocation 상태에 의존하지 않음)
	FVector MeshTopCenterWorld = GetActorLocation();
	FVector MeshWorldExtent(50.f);
	if (StaticMeshComponent && StaticMeshComponent->GetStaticMesh())
	{
		const FBox LocalBox = StaticMeshComponent->GetStaticMesh()->GetBoundingBox();
		const FVector LocalTopCenter(LocalBox.GetCenter().X, LocalBox.GetCenter().Y, LocalBox.Max.Z);
		MeshTopCenterWorld = StaticMeshComponent->GetComponentTransform().TransformPosition(LocalTopCenter);
		MeshWorldExtent = StaticMeshComponent->Bounds.BoxExtent;
	}
	const FVector BoundsCenter = MeshTopCenterWorld;
	const float TopZ = MeshTopCenterWorld.Z;

	// [GAS] Initialize ASC

	if (ASC && CombatAttributes)
	{
		ASC->InitAbilityActorInfo(this, this);

		// Initialize Health Component
		if (HealthComponent)
		{
			HealthComponent->InitFromGAS(ASC, CombatAttributes);
			HealthComponent->OnHealthChangedUI.AddDynamic(this, &ABVBuildingBase::HandleHealthChangedForAudio);
			PreviousHealthRatio = HealthComponent->GetHealthRatio();
		}

		ApplyInitStatFromDataTable();
	}

	// Add StimuliSource
	StimuliSourceComponent->RegisterForSense(UAISense_Sight::StaticClass());
	StimuliSourceComponent->RegisterWithPerceptionSystem();

	// DA에 위젯 클래스가 지정되어 있으면 그걸 우선 사용
	if (BuildingData && BuildingData->OverheadWidgetClass)
	{
		OverheadWidgetClass = BuildingData->OverheadWidgetClass;
	}

	if (OverheadWidgetClass && OverheadWidgetComponent)
	{
		OverheadWidgetComponent->SetWidgetClass(OverheadWidgetClass);
	}
	
	// Building Overhead Widget
	if (OverheadWidgetComponent)
	{
		// 두께는 WBP의 WorldThickness 프로퍼티로 결정, 길이만 풋프린트에 비례.
		const FVector2D WidgetDesignSize(120.f, 12.f);

		// 먼저 UserWidget을 가져와서 WorldThickness 읽기 (없으면 기본 36)
		float WorldThickness = 36.f;
		if (UUserWidget* UserWidget = OverheadWidgetComponent->GetUserWidgetObject())
		{
			if (UUBVBuildingOverheadWidget* BuildingWidget = Cast<UUBVBuildingOverheadWidget>(UserWidget))
			{
				WorldThickness = BuildingWidget->WorldThickness;
			}
		}
		const float BaseScale = WorldThickness / WidgetDesignSize.Y; // DrawSize.Y(=12) × scale = WorldThickness

		const float BuildingFootprint = FMath::Max(MeshWorldExtent.X, MeshWorldExtent.Y) * 2.0f;
		const float TargetWorldWidth  = FMath::Max(BuildingFootprint * 1.0f, 40.f);
		const float TargetDrawWidth   = TargetWorldWidth / BaseScale;
		OverheadWidgetComponent->SetDrawSize(FVector2D(TargetDrawWidth, WidgetDesignSize.Y));
		OverheadWidgetComponent->SetWorldScale3D(FVector(BaseScale));

		OverheadWidgetComponent->SetPivot(FVector2D(0.5f, 1.0f));
		FVector WidgetLoc = BoundsCenter;
		WidgetLoc.Z = TopZ + 120.0f;  // 건물 위로 좀 더 띄우기
		OverheadWidgetComponent->SetWorldLocation(WidgetLoc);
		OverheadWidgetComponent->SetWorldRotation(FRotator(65.f, 180.f, 0.f));
		
		if (UUserWidget* UserWidget = OverheadWidgetComponent->GetUserWidgetObject())
		{
			OverheadWidget = Cast<UUBVBuildingOverheadWidget>(UserWidget);
			if (OverheadWidget)
			{
				OverheadWidget->InitWithHealthComponent(HealthComponent);
			}
		}

		// Tab으로 전역 off 상태라면 스폰 즉시 위젯 숨기기
		if (ABVPlayerController* BVPC = Cast<ABVPlayerController>(GetWorld()->GetFirstPlayerController()))
		{
			if (!BVPC->AreOverheadWidgetsVisible())
			{
				OverheadWidgetComponent->SetVisibility(false);
			}
		}
	}


	ElapsedTime = 0.0f;

	// 런타임에 IsProducing 파라미터를 조작하려면 MID 필요.
	InitDynamicMaterials();

	// 건물 전체가 다른 호버 링 데칼을 받지 않도록 일괄 차단.
	TArray<UPrimitiveComponent*> AllPrims;
	GetComponents<UPrimitiveComponent>(AllPrims);
	for (UPrimitiveComponent* Prim : AllPrims)
	{
		if (Prim)
		{
			Prim->SetReceivesDecals(false);
		}
	}

	// 호버 링 데칼 세팅: 메시 풋프린트 기준으로 크기 자동 계산 + MID 초기화.
	if (HoverRingDecal)
	{
		// 캘리브레이션 계수: 메시 바운딩 박스는 여백을 포함해서 실제 풋프린트보다 큼.
		// 시각적으로 맞는 기본값이 0.65라서 Padding에 곱함.
		constexpr float CalibrationFactor = 0.65f;

		// 메시 로컬 바운딩의 가로/세로 중 큰 쪽을 풋프린트로 사용.
		float LocalDiameter = 256.f;
		if (StaticMeshComponent && StaticMeshComponent->GetStaticMesh())
		{
			const FBox LocalBox = StaticMeshComponent->GetStaticMesh()->GetBoundingBox();
			const FVector Extent = LocalBox.GetExtent();
			const float Footprint = FMath::Max(Extent.X, Extent.Y) * 2.f;
			LocalDiameter = Footprint * HoverRingPadding * CalibrationFactor;
		}

		HoverRingDecal->DecalSize = FVector(HoverRingDepth, LocalDiameter, LocalDiameter);
		HoverRingDecal->SetRelativeLocation(HoverRingOffset);

		// 월드 지름 = 로컬 지름 × 액터 스케일. UV 두께 = 월드 두께 / 월드 지름 → 크기 무관 일정.
		const float ActorScale = GetActorScale3D().GetMax();
		const float WorldDiameter = LocalDiameter * FMath::Max(ActorScale, KINDA_SMALL_NUMBER);
		const float UVThickness = FMath::Clamp(HoverRingThickness / WorldDiameter, 0.002f, 0.3f);

		if (!HoverRingMaterial.IsNull())
		{
			if (UMaterialInterface* Src = HoverRingMaterial.LoadSynchronous())
			{
				HoverRingMID = UMaterialInstanceDynamic::Create(Src, this);
				HoverRingDecal->SetDecalMaterial(HoverRingMID);
				if (HoverRingMID)
				{
					HoverRingMID->SetScalarParameterValue(RingIntensityParamName, 0.f);
					HoverRingMID->SetVectorParameterValue(RingTintParamName, FLinearColor::White);
					HoverRingMID->SetScalarParameterValue(RingThicknessParamName, UVThickness);
				}
			}
		}
	}

	// City 반경 안에 들어와 있으면 OwningCity로 캐싱 — SpawnUnit이 이걸 보고 게리슨 라우팅 결정.
	// (도시 자신은 ResolveOwningCity 내부에서 자기 자신을 owning city로 갖지 않도록 가드.)
	ResolveOwningCity();

	const bool bWillProduce = SpawnUnitClass && RespawnInterval > 0.f;

	// 생산 중 펄스 on/off 초기 상태 반영 (머티리얼에 IsProducing 파라미터 없어도 안전).
	SetIsProducing(bWillProduce);

	if (bWillProduce)
	{
		GetWorldTimerManager().SetTimer(
			SpawnTimerHandle,
			this,
			&ABVBuildingBase::SpawnUnit,
			RespawnInterval,
			true,
			RespawnInterval
			);
	}

}

void ABVBuildingBase::ResolveOwningCity()
{
	OwningCity.Reset();

	// 도시 자신은 owning city를 갖지 않는다 (자기 자신을 게리슨으로 잡으면 무한 루프 위험).
	if (Cast<ABVCityBase>(this)) return;

	UWorld* World = GetWorld();
	if (!World) return;

	const FVector MyLoc = GetActorLocation();

	// 자기 위치를 BuildRadius 안에 포함하는 도시 중 가장 가까운 것을 선택.
	// (반경이 겹치는 경우 발생 가능 — 가장 가까운 도시 = 가장 자연스러운 owner.)
	ABVCityBase* BestCity = nullptr;
	float BestDistSq = TNumericLimits<float>::Max();

	for (TActorIterator<ABVCityBase> It(World); It; ++It)
	{
		ABVCityBase* City = *It;
		if (!City) continue;
		if (!City->IsLocationWithinBuildRadius(MyLoc)) continue;

		const float DistSq = FVector::DistSquared2D(MyLoc, City->GetActorLocation());
		if (DistSq < BestDistSq)
		{
			BestDistSq = DistSq;
			BestCity = City;
		}
	}

	if (BestCity)
	{
		OwningCity = BestCity;
	}
}


void ABVBuildingBase::ApplyInitStatFromDataTable()
{

	if (!ASC || !InitStatsEffect || !BuildingData) return;

	BuildingName = BuildingData->BuildingName;

	if (TeamType == EBVTeam::Neutral)
	{
		TeamType = BuildingData->TeamType;
	}

	// 스폰 관련 필드는 bCanSpawnUnits가 true일 때만 DA 값 적용.
	// false면 DA의 잔존 데이터로 덮어쓰지 않는다.
	if (BuildingData->bCanSpawnUnits)
	{
		if (BuildingData->SpawnUnitClass)
		{
			SpawnUnitClass = BuildingData->SpawnUnitClass;
		}

		if (BuildingData->SpawnInterval > 0.f)
		{
			RespawnInterval = BuildingData->SpawnInterval;
		}
	}

	FGameplayEffectContextHandle GEContext = ASC->MakeEffectContext();
	GEContext.AddSourceObject(this);

	FGameplayEffectSpecHandle GESpec = ASC->MakeOutgoingSpec(InitStatsEffect, 1.f, GEContext);
	if (!GESpec.IsValid()) return;

	GESpec.Data->SetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag(TEXT("Data.MaxHealth")), BuildingData->MaxHealth);
	GESpec.Data->SetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag(TEXT("Data.Health")), BuildingData->MaxHealth);

	// 공격력은 bCanAttack이 true일 때만 적용.
	if (BuildingData->bCanAttack)
	{
		GESpec.Data->SetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag(TEXT("Data.Damage")), BuildingData->Damage);
	}

	ASC->ApplyGameplayEffectSpecToSelf(*GESpec.Data.Get());

}

void ABVBuildingBase::SpawnUnit()
{
	if (!SpawnUnitClass) return;

	UWorld* World = GetWorld();
	if (!World) return;

	// City 산하 생산건물(barracks/factory 등): 도시 게리슨이 가득이면 생산 일시정지.
	// 다음 타이머 틱에서 다시 평가. 게리슨이 줄어들면 자연스럽게 재개.
	ABVCityBase* RoutedCity = OwningCity.Get();
	if (RoutedCity && RoutedCity->IsGarrisonFull()) return;

	// 스폰 반경 (건물 중심으로부터 허용되는 최대 거리)
	float SpawnRadius = 200.f;
	if (BoxComponent)
	{
		const float BoxRadius = BoxComponent->GetScaledBoxExtent().X;
		SpawnRadius = BoxRadius + 100.f;
	}

	const FVector BuildingLoc = GetActorLocation();

	// 스폰 위치: 기본은 Forward 방향으로 반경만큼 이동한 지점.
	// City 산하면 lane 무시(도시 rally로 가야 하니까), 아니면 lane 우선.
	FVector SpawnLocation    = BuildingLoc + GetActorForwardVector() * SpawnRadius;
	FVector SpawnDirection   = GetActorForwardVector();

	if (AssignedLane && !RoutedCity)
	{
		const FVector FootOnLane         = AssignedLane->GetPerpendicularFoot(BuildingLoc);
		const FVector BuildingToFoot     = FVector(FootOnLane.X - BuildingLoc.X, FootOnLane.Y - BuildingLoc.Y, 0.f);
		const float   DistanceToLane     = BuildingToFoot.Size();

		if (DistanceToLane <= SpawnRadius)
		{
			// 레인이 건물 반경 안/내부를 지나감.
			// 수선의 발에 그대로 스폰하면 건물 안쪽에 꽂혀서 낙하 버그 발생.
			// → 수선의 발에서 "적 베이스 방향"으로 SpawnRadius만큼 밀어서 건물 밖에 배치.
			FVector LaneDir =
				(AssignedLane->GetEnemyBaseLocation() - AssignedLane->GetFriendlyBaseLocation()).GetSafeNormal2D();
			if (LaneDir.IsNearlyZero())
			{
				LaneDir = FVector(GetActorForwardVector().X, GetActorForwardVector().Y, 0.f).GetSafeNormal();
			}
			SpawnLocation = FVector(FootOnLane.X, FootOnLane.Y, BuildingLoc.Z) + LaneDir * SpawnRadius;
		}
		else if (!BuildingToFoot.IsNearlyZero())
		{
			// 레인이 반경 밖 -> 반경 경계에서 레인 쪽으로 가장 가까운 점
			SpawnLocation = BuildingLoc + BuildingToFoot.GetSafeNormal() * SpawnRadius;
		}

		// 스폰 방향은 적 베이스 쪽으로
		const FVector ToEnemy = (AssignedLane->GetEnemyBaseLocation() - SpawnLocation).GetSafeNormal2D();
		if (!ToEnemy.IsNearlyZero())
		{
			SpawnDirection = ToEnemy;
		}
	}

	SpawnLocation.Z += 50.0f;
	const FRotator  SpawnRotation = SpawnDirection.Rotation();
	const FTransform SpawnTransform(SpawnRotation, SpawnLocation);

	// 1. 지연 스폰 (태어나기 직전, 메모리에만 올라간 일시정지 상태)
	ABVAutobotBase* NewSpawnUnit = World->SpawnActorDeferred<ABVAutobotBase>(
		SpawnUnitClass,
		SpawnTransform,
		this,
		nullptr,
		ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn
	);

	if (NewSpawnUnit)
	{
		// 2. 건물의 팀 정보를 유닛에게 그대로 물려줍니다!
		NewSpawnUnit->SetGenericTeamId(GetGenericTeamId());

		// 3. AI가 빙의하기 전에, 게리슨/레인 정보를 쏙 넣어줍니다.
		//    City 산하 생산건물은 lane 대신 rally point으로 라우팅.
		if (RoutedCity)
		{
			NewSpawnUnit->AssignedLane = nullptr;
			NewSpawnUnit->bHasRallyDestination = true;
			NewSpawnUnit->RallyDestination = RoutedCity->RallyPoint;
		}
		else if (AssignedLane)
		{
			NewSpawnUnit->AssignedLane = AssignedLane;
		}

		// 4. 자 이제 멈춰뒀던 스폰을 완료해라! (이때 AI가 빙의하면서 정보를 성공적으로 읽음)
		NewSpawnUnit->FinishSpawning(SpawnTransform);

		// 5. 도시 게리슨 등록은 FinishSpawning 후 (en-route 인원도 cap 산정에 포함).
		if (RoutedCity)
		{
			RoutedCity->AddToGarrison(NewSpawnUnit);
		}
	}
}

void ABVBuildingBase::SetHovered_Implementation(bool bInHovered)
{
	bIsHovered = bInHovered;
	HoverTargetValue = bInHovered ? 1.f : 0.f;

	// 새 StaticMesh 버전 hover ring 호출 (BoxComponent 기반 AutoSize + Project Settings 값 사용).
	if (StaticHoverRingComponent)
	{
		StaticHoverRingComponent->SetHovered(bInHovered, TeamType);
	}

	if (!bInHovered) return;

	// 호버 시작 시점에 팀 색을 고정. attitude가 아닌 실제 팀 ID로 판정
	// (attitude는 점령용으로 중립을 적대로 취급해서 색 오인 유발).
	EBVTeam ViewerTeam = EBVTeam::None;
	if (APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0))
	{
		if (IGenericTeamAgentInterface* TeamAgentPC = Cast<IGenericTeamAgentInterface>(PC))
		{
			ViewerTeam = static_cast<EBVTeam>(TeamAgentPC->GetGenericTeamId().GetId());
		}
	}

	// 아군=녹, 적=적, 중립=청.
	if (TeamType == EBVTeam::Neutral)
	{
		HoverTintColor = FLinearColor(0.2f, 0.6f, 1.f);
	}
	else if (TeamType == ViewerTeam)
	{
		HoverTintColor = FLinearColor(0.2f, 1.f, 0.4f);
	}
	else
	{
		HoverTintColor = FLinearColor(1.f, 0.25f, 0.25f);
	}

	// 첫 프레임에 MID가 준비되어 있지 않았다면(호버 직전까지 로드 지연) 지금 만들어둠.
	if (!HoverRingMID && HoverRingDecal && !HoverRingMaterial.IsNull())
	{
		if (UMaterialInterface* Src = HoverRingMaterial.LoadSynchronous())
		{
			HoverRingMID = UMaterialInstanceDynamic::Create(Src, this);
			HoverRingDecal->SetDecalMaterial(HoverRingMID);
		}
	}
}

void ABVBuildingBase::UpdateHoverAnim(float DeltaTime)
{
	// Hover ring 렌더링은 StaticHoverRingComponent가 전담. 여기서는 interp 값만 유지.
	HoverCurrentValue = FMath::FInterpTo(HoverCurrentValue, HoverTargetValue, DeltaTime, HoverEaseSpeed);

	// 기존 Decal(HoverRingDecal)은 BP 호환성 유지용으로 남아있지만 visibility를 항상 꺼둠.
	if (HoverRingDecal && HoverRingDecal->IsVisible())
	{
		HoverRingDecal->SetVisibility(false);
		HoverRingDecal->SetHiddenInGame(true);
	}
}
