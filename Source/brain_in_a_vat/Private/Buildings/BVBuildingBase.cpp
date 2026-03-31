// Fill out your copyright notice in the Description page of Project Settings.


#include "Buildings/BVBuildingBase.h"
#include "AI/BVLane.h"

#include "BVPlayerController.h"
#include "Characters/BVAutobotBase.h"
#include "Components/WidgetComponent.h"
#include "Components/BoxComponent.h"
#include "Components/BVHealthComponent.h"
#include "Perception/AIPerceptionStimuliSourceComponent.h"
#include "Widget/BVSpawnCooltimeBar.h"
#include "DrawDebugHelpers.h"
#include "GAS/CombatAttributeSet.h"
#include "Perception/AISense_Sight.h"
#include "Collision/BVCollision.h"
#include "Components/BoxComponent.h"
#include "Data/UnitStats.h"
#include "Kismet/GameplayStatics.h"
#include "Widget/UBVBuildingOverheadWidget.h"
#include "GAS/GASTags.h"
#include "Weapons/Projectiles/BVLaserBeamBase.h"

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
	BoxComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	BoxComponent->SetGenerateOverlapEvents(true);
	BoxComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
	BoxComponent->SetCollisionObjectType(ECC_Building);

	BoxComponent->SetCollisionResponseToChannel(ECC_MouseHover, ECR_Block);
	BoxComponent->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Overlap);
	BoxComponent->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
	BoxComponent->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

	StaticMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMesh"));
	StaticMeshComponent->SetupAttachment(RootComponent);
	StaticMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	StaticMeshComponent->SetCollisionProfileName(TEXT("Hoverable"));

	// [GAS] ASC & Attributes

	ASC = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("ASC"));
	ASC->SetIsReplicated(true);

	CombatAttributes = CreateDefaultSubobject<UCombatAttributeSet>(TEXT("CombatAttributes"));

	// Stimuli Source Component
	StimuliSourceComponent = CreateDefaultSubobject<UAIPerceptionStimuliSourceComponent>(TEXT("StimuliSource"));

	// Health Component
	HealthComponent = CreateDefaultSubobject<UBVHealthComponent>(TEXT("HealthComponent"));

	

	

	// <--------------- Assets ----------------> //
	// Unit Stats
	static ConstructorHelpers::FObjectFinder<UDataTable> DT_UnitStats(TEXT("/Script/Engine.DataTable'/Game/Data/UnitStats.UnitStats'"));
	if (DT_UnitStats.Succeeded())
	{
		StatTable = DT_UnitStats.Object;
	}

	static ConstructorHelpers::FClassFinder<UGameplayEffect> InitStatGEClass(TEXT("/Script/Engine.Blueprint'/Game/GAS/GE/GE_InitStat.GE_InitStat_C'"));
	if (InitStatGEClass.Succeeded())
	{
		InitStatsEffect = InitStatGEClass.Class;
	}

	// <--------------- Widgets ----------------> //
	
	OverheadWidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("OverheadWidget"));
	OverheadWidgetComponent->SetupAttachment(RootComponent);
	OverheadWidgetComponent->SetWidgetSpace(EWidgetSpace::Screen);
	OverheadWidgetComponent->SetDrawSize(FVector2D(150.f, 20.f));
	
}

void ABVBuildingBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// If destroyed, do nothing
	if (bIsDestroyed) return;

	if (RespawnInterval <= 0.f) return;

	ElapsedTime += DeltaTime;
	
	float Percent = FMath::Fmod(ElapsedTime, RespawnInterval) / RespawnInterval;

	if (OverheadWidget)
	{
		OverheadWidget->SetRespawnProgress(Percent);
	}
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

	// Hide Widget
	if (OverheadWidgetComponent)
		OverheadWidgetComponent->SetVisibility(false);

	// Destroy
	SetLifeSpan(1.0f);
	
}

void ABVBuildingBase::FinishConstruction()
{
	
}

UAbilitySystemComponent* ABVBuildingBase::GetAbilitySystemComponent() const
{
	return ASC;	
}

void ABVBuildingBase::HandleHealthChangedForAudio(float NewHealthRatio)
{
	if (NewHealthRatio < PreviousHealthRatio)
	{
		if (TeamType == EBVTeam::Player)
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

	if (StaticMeshComponent && StaticMeshComponent->GetStaticMesh() && BoxComponent)
	{
		FBox MeshBounds = StaticMeshComponent->GetStaticMesh()->GetBoundingBox();
		BoxComponent->SetBoxExtent(MeshBounds.GetExtent());
	}
	
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

	// Get actual actor size
	const FBoxSphereBounds Bounds = StaticMeshComponent->CalcBounds(StaticMeshComponent->GetComponentTransform());
	const float TopZ = Bounds.Origin.Z + Bounds.BoxExtent.Z;

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
	
	// Building Overhead Widget
	if (OverheadWidgetComponent)
	{
		OverheadWidgetComponent->SetPivot(FVector2D(0.5f, 1.0f));
		FVector WidgetLoc = Bounds.Origin;
		WidgetLoc.Z = TopZ + 100.0f;
		OverheadWidgetComponent->SetWorldLocation(WidgetLoc);
		
		if (UUserWidget* UserWidget = OverheadWidgetComponent->GetUserWidgetObject())
		{
			OverheadWidget = Cast<UUBVBuildingOverheadWidget>(UserWidget);
			if (OverheadWidget)
			{
				OverheadWidget->SetBuildingName(BuildingName);
				OverheadWidget->InitWithHealthComponent(HealthComponent);
			}
		}
	}


	ElapsedTime = 0.0f;

	if (SpawnUnitClass && RespawnInterval > 0.f)
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

const FUnitStats* ABVBuildingBase::GetStats() const
{
	if (!StatTable || StatRowName.IsNone()) return nullptr;
	return StatTable->FindRow<FUnitStats>(StatRowName, TEXT("StatLookup"));
}

void ABVBuildingBase::ApplyInitStatFromDataTable()
{

	if (!ASC) return;
	if (!InitStatsEffect) return;

	const FUnitStats* Stats = GetStats();
	if (!Stats) return;

	FGameplayEffectContextHandle GEContext = ASC->MakeEffectContext();
	GEContext.AddSourceObject(this);

	FGameplayEffectSpecHandle GESpec = ASC->MakeOutgoingSpec(InitStatsEffect, 1.f, GEContext);
	if (!GESpec.IsValid()) return;

	GESpec.Data->SetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag(TEXT("Data.MaxHealth")), Stats->MaxHealth);
	GESpec.Data->SetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag(TEXT("Data.Health")), Stats->MaxHealth);
	GESpec.Data->SetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag(TEXT("Data.Damage")), Stats->Damage);
	
	ASC->ApplyGameplayEffectSpecToSelf(*GESpec.Data.Get());
	
}

void ABVBuildingBase::SpawnUnit()
{
	
	UE_LOG(LogTemp, Warning, TEXT("SpawnUnit called!"))
	if (!SpawnUnitClass) return;

	UWorld* World = GetWorld();
	if (!World) return;

	// Choosing spawn location
	float SpawnDistance = 200.f;

	if (BoxComponent)
	{
		float BoxRadius = BoxComponent->GetScaledBoxExtent().X;
		SpawnDistance = BoxRadius + 100.f;
	}
	
	FVector SpawnLocation = GetActorLocation() + (GetActorForwardVector() * SpawnDistance);
	SpawnLocation.Z += 50.0f;
	const FRotator SpawnRotation = FRotator::ZeroRotator;
	FTransform SpawnTransform(SpawnRotation, SpawnLocation);

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
		// 2. AI가 빙의하기 전에, 유닛의 호주머니에 레인 정보를 쏙 넣어줍니다!
		if (AssignedLane)
		{
			NewSpawnUnit->AssignedLane = AssignedLane;
		}

		// 3. 자 이제 멈춰뒀던 스폰을 완료해라! (이때 AI가 빙의하면서 레인 정보를 성공적으로 읽음)
		NewSpawnUnit->FinishSpawning(SpawnTransform);
	}
}

void ABVBuildingBase::SetHovered_Implementation(bool bInHovered)
{
	bIsHovered = bInHovered;
	if (StaticMeshComponent)
	{
		uint8 Stencil = 0;

		if (bIsHovered)
		{

			APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
			IGenericTeamAgentInterface* TeamAgentPC = Cast<IGenericTeamAgentInterface>(PC);

			if (TeamAgentPC)
			{
				ETeamAttitude::Type Attitude = TeamAgentPC->GetTeamAttitudeTowards((*this));

				switch (Attitude)
				{
				case ETeamAttitude::Friendly:
					Stencil = 1;
					break;
				case ETeamAttitude::Hostile:
					Stencil = 2;
					break;
				case ETeamAttitude::Neutral:
					Stencil = 3;
					break;
				default:
					Stencil = 0;
					break;
				}
			}

		}
		
		StaticMeshComponent->SetRenderCustomDepth(bIsHovered);
		StaticMeshComponent->SetCustomDepthStencilValue(Stencil);
		
		// FString DebugMsg = FString::Printf(TEXT("[%s] is hovered! Stencil : %d"), *GetName(), Stencil);
		// GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Orange, DebugMsg);
	}
}
