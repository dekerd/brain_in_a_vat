// Fill out your copyright notice in the Description page of Project Settings.


#include "Buildings/BVBuildingBase.h"
#include "Autobots/BVAutobotBase.h"
#include "Components/WidgetComponent.h"
#include "Components/BoxComponent.h"
#include "Components/BVHealthComponent.h"
#include "Perception/AIPerceptionStimuliSourceComponent.h"
#include "Widget/BVSpawnCooltimeBar.h"
#include "Widget/BVNameWidget.h"
#include "DrawDebugHelpers.h"
#include "Widget/BVHealthBarWidget.h"
#include "GAS/CombatAttributeSet.h"
#include "Perception/AISense_Sight.h"
#include "Collision/BVCollision.h"
#include "Data/UnitStats.h"
#include "Widget/BVNameWidget.h"

// Sets default values
ABVBuildingBase::ABVBuildingBase()
{
	PrimaryActorTick.bCanEverTick = true;

	// <--------------- Components ----------------> //
	// Root Component
	BoxComponent = CreateDefaultSubobject<UBoxComponent>(TEXT("Box"));
	RootComponent = BoxComponent;
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
	
	// HealthBar Widget
	HealthBarWidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("HealthBarWidget"));

	static ConstructorHelpers::FClassFinder<UBVHealthBarWidget> HealthBarWidgetRef(TEXT("/Script/UMGEditor.WidgetBlueprint'/Game/HUD/Widget/WBP_HealthBar.WBP_HealthBar_C'"));
	if (HealthBarWidgetRef.Succeeded())
	{
		HealthBarWidgetClass = HealthBarWidgetRef.Class;
		HealthBarWidgetComponent->SetWidgetClass(HealthBarWidgetClass);
	}
	
	HealthBarWidgetComponent->SetupAttachment(RootComponent);
	HealthBarWidgetComponent->SetWidgetSpace(EWidgetSpace::Screen);
	
	// Respawn Cooltime Widget
	RespawnWidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("RespawnWidget"));

	static ConstructorHelpers::FClassFinder<UBVSpawnCooltimeBar> SpawnWidgetRef(TEXT("/Script/UMGEditor.WidgetBlueprint'/Game/HUD/Widget/WBP_SpawnCooltimeBar.WBP_SpawnCooltimeBar_C'"));
	if (SpawnWidgetRef.Class != nullptr)
	{
		RespawnWidgetClass = SpawnWidgetRef.Class;
		RespawnWidgetComponent->SetWidgetClass(RespawnWidgetClass);
	}
	
	RespawnWidgetComponent->SetupAttachment(RootComponent);
	RespawnWidgetComponent->SetWidgetSpace(EWidgetSpace::Screen);

	// Name Widget
	NameWidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("NameWidget"));

	static ConstructorHelpers::FClassFinder<UBVNameWidget> NameWidgetRef(TEXT("/Script/UMGEditor.WidgetBlueprint'/Game/HUD/Widget/WBP_NameWidget.WBP_NameWidget_C'"));
	if (NameWidgetRef.Class != nullptr)
	{
		NameWidgetClass = NameWidgetRef.Class;
		NameWidgetComponent->SetWidgetClass(NameWidgetClass);
	}
	
	NameWidgetComponent->SetupAttachment(RootComponent);
	NameWidgetComponent->SetWidgetSpace(EWidgetSpace::Screen);
	
}

void ABVBuildingBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// If destroyed, do nothing
	if (bIsDestroyed) return;

	if (RespawnInterval <= 0.f) return;

	ElapsedTime += DeltaTime;

	float CycleTime = FMath::Fmod(ElapsedTime, RespawnInterval);
	float Percent = CycleTime / RespawnInterval;

	if (RespawnWidget)
	{
		RespawnWidget->SetProgress(Percent);
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
	if (RespawnWidgetComponent)
		RespawnWidgetComponent->SetVisibility(false);
	if (HealthBarWidgetComponent)
		HealthBarWidgetComponent->SetVisibility(false);

	// Destroy
	SetLifeSpan(1.0f);
	
}

FGenericTeamId ABVBuildingBase::GetGenericTeamId() const
{
	return FGenericTeamId(TeamFlag);
}

UAbilitySystemComponent* ABVBuildingBase::GetAbilitySystemComponent() const
{
	return ASC;	
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

	RespawnWidgetComponent->SetWorldLocation(FVector(Bounds.Origin.X, Bounds.Origin.Y, TopZ + 50.f));
	RespawnWidgetComponent->SetDrawSize(FVector2D(200.f, 10.f));
	
	HealthBarWidgetComponent->SetWorldLocation(FVector(Bounds.Origin.X, Bounds.Origin.Y, TopZ + 70.f));
	HealthBarWidgetComponent->SetDrawSize(FVector2D(200.f, 10.f));

	// [GAS] Initialize ASC

	if (ASC && CombatAttributes)
	{
		ASC->InitAbilityActorInfo(this, this);

		// Initialize Health Component
		if (HealthComponent)
		{
			HealthComponent->InitFromGAS(ASC, CombatAttributes);
		}

		ApplyInitStatFromDataTable();
	}

	// Add StimuliSource
	StimuliSourceComponent->RegisterForSense(UAISense_Sight::StaticClass());
	StimuliSourceComponent->RegisterWithPerceptionSystem();

	// Health Bar Widget
	if (HealthBarWidgetComponent)
	{
		if (UUserWidget* Widget = HealthBarWidgetComponent->GetUserWidgetObject())
		{
			if (UBVHealthBarWidget* HealthBar = Cast<UBVHealthBarWidget>(Widget))
			{
				HealthBar->InitWithHealthComponent(HealthComponent);
			}
		}
	}
	
	// Respawn Cooltime Widget
	if (UUserWidget* UserWidget = RespawnWidgetComponent->GetUserWidgetObject())
	{
		RespawnWidget = Cast<UBVSpawnCooltimeBar>(UserWidget);
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

	// Name Widget
	if (NameWidgetComponent)
	{
		UUserWidget* WidgetObject = NameWidgetComponent->GetUserWidgetObject();
		if (WidgetObject)
		{
			UBVNameWidget* NameWidget = Cast<UBVNameWidget>(WidgetObject);
			if (NameWidget)
			{
				NameWidget->SetBuildingName(BuildingName);
			}
		}
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

	ASC->ApplyGameplayEffectSpecToSelf(*GESpec.Data.Get());
	
}

void ABVBuildingBase::SpawnUnit()
{
	
	UE_LOG(LogTemp, Warning, TEXT("SpawnUnit called!"))
	if (!SpawnUnitClass) return;

	UWorld* World = GetWorld();
	if (!World) return;

	// Choosing spawn location
	FVector SpawnLocation = GetActorLocation() + GetActorForwardVector()*200.f;
	const FRotator SpawnRotation = FRotator::ZeroRotator;
	
	DrawDebugSphere(GetWorld(), SpawnLocation, 25.0f, 12, FColor::Blue, false, 1.0f);

	FActorSpawnParameters Params;
	Params.Owner = this;

	ABVAutobotBase* NewSpawnUnit = World->SpawnActor<ABVAutobotBase>(
		SpawnUnitClass,
		SpawnLocation,
		SpawnRotation,
		Params
		);

	if (NewSpawnUnit)
	{
		// Initialization
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
			const bool bIsEnemy = (TeamFlag != 1);
			Stencil = bIsEnemy ? 2 : 1;
		}

		StaticMeshComponent->SetRenderCustomDepth(bIsHovered);
		StaticMeshComponent->SetCustomDepthStencilValue(Stencil);

		// FString DebugMsg = FString::Printf(TEXT("[%s] is hovered! Stencil : %d"), *GetName(), Stencil);
		// GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Orange, DebugMsg);
	}
}
