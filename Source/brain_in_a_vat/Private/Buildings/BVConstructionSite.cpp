// Fill out your copyright notice in the Description page of Project Settings.


#include "Buildings/BVConstructionSite.h"

#include "BVPlayerController.h"
#include "Buildings/BVBuildingBase.h"
#include "Collision/BVCollision.h"
#include "Components/BoxComponent.h"
#include "Components/BVHealthComponent.h"
#include "Components/WidgetComponent.h"
#include "GAS/CombatAttributeSet.h"
#include "Perception/AIPerceptionStimuliSourceComponent.h"
#include "Perception/AISense_Sight.h"
#include "Widget/BVConstructionWidget.h"
#include "Data/BVBuildingData.h"
#include "Perception/AIPerceptionStimuliSourceComponent.h"
#include "Perception/AISense_Sight.h"
#include "Components/BVHealthComponent.h"
#include "GAS/CombatAttributeSet.h"


// Sets default values
ABVConstructionSite::ABVConstructionSite()
{
	PrimaryActorTick.bCanEverTick = true;

	// Components
	SceneRootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRootComponent;

	BoxComponent = CreateDefaultSubobject<UBoxComponent>(TEXT("BoxComponent"));
	BoxComponent->SetupAttachment(RootComponent);
	BoxComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	BoxComponent->SetGenerateOverlapEvents(true);
	BoxComponent->SetCollisionObjectType(ECC_Building);

	BoxComponent->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);       
	BoxComponent->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block); 
	BoxComponent->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);  
	BoxComponent->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);   
	BoxComponent->SetCollisionResponseToChannel(ECC_MouseHover, ECR_Block);
	BoxComponent->SetCollisionResponseToChannel(ECC_Player, ECR_Block);

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	MeshComponent->SetupAttachment(RootComponent);
	MeshComponent->SetCollisionProfileName(TEXT("NoCollision"));

	GhostMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GhostMeshComponent"));
	GhostMeshComponent->SetupAttachment(RootComponent);
	GhostMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GhostMeshComponent->SetCastShadow(false);

	// GAS
	ASC = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("ASC"));
	ASC->SetIsReplicated(true);

	CombatAttributes = CreateDefaultSubobject<UCombatAttributeSet>(TEXT("CombatAttributes"));
	HealthComponent = CreateDefaultSubobject<UBVHealthComponent>(TEXT("HealthComponent"));

	// AI
	StimuliSourceComponent = CreateDefaultSubobject<UAIPerceptionStimuliSourceComponent>(TEXT("StimuliSource"));

	// Widget
	ProgressWidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("ProgressWidgetComponent"));
	ProgressWidgetComponent->SetupAttachment(RootComponent);
	ProgressWidgetComponent->SetWidgetSpace(EWidgetSpace::Screen);
	ProgressWidgetComponent->SetDrawSize(FVector2D(150.f, 20.f));
	
}

// Called when the game starts or when spawned
void ABVConstructionSite::BeginPlay()
{
	Super::BeginPlay();

	// AI stimuli source initialization
	if (StimuliSourceComponent)
	{
		StimuliSourceComponent->RegisterForSense(UAISense_Sight::StaticClass());
		StimuliSourceComponent->RegisterWithPerceptionSystem();
	}

	// GAS
	if (ASC && CombatAttributes)
	{
		ASC->InitAbilityActorInfo(this, this);
		HealthComponent->InitFromGAS(ASC, CombatAttributes);
		
		if (TargetBuildingClass)
		{
			ABVBuildingBase* BuildingCDO = TargetBuildingClass->GetDefaultObject<ABVBuildingBase>();
	
			if (BuildingCDO && BuildingCDO->BuildingData)
			{
				MaxHealth = BuildingCDO->BuildingData->MaxHealth;
			}
		}

		CombatAttributes->SetMaxHealth(MaxHealth);
		CombatAttributes->SetHealth(FMath::FloorToFloat(MaxHealth * 0.01f) + 1); 
	}

	// Overhead Widget Size
	if (ProgressWidgetComponent)
	{
		const FBoxSphereBounds Bounds = MeshComponent->CalcBounds(MeshComponent->GetComponentTransform());
		const float TopZ = Bounds.Origin.Z + Bounds.BoxExtent.Z;
		
		ProgressWidgetComponent->SetPivot(FVector2D(0.5f, 1.0f));
		FVector WidgetLoc = Bounds.Origin;
		WidgetLoc.Z = TopZ + 100.0f;
		ProgressWidgetComponent->SetWorldLocation(WidgetLoc);
	}
	
}

// Called every frame
void ABVConstructionSite::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bIsDestroyed) return;

	if (bIsBuilding)
	{
		CurrentProgress += DeltaTime;
		float ProgressRatio = FMath::Clamp(CurrentProgress / ConstructionTime, 0.0f, 1.0f);

		// Health
		if (ASC && CombatAttributes)
		{
			float CurrentHealth = CombatAttributes->GetHealth();
			float HealAmount = (MaxHealth / ConstructionTime) * DeltaTime;
			
			float NewHealth = FMath::Clamp(CurrentHealth + HealAmount, 0.0f, MaxHealth);
			CombatAttributes->SetHealth(NewHealth);
		}

		// Progress Widget
		if (ProgressWidgetComponent)
		{
			UBVConstructionWidget* ProgressWidget = Cast<UBVConstructionWidget>(ProgressWidgetComponent->GetUserWidgetObject());
			if (ProgressWidget)
			{
				ProgressWidget->SetProgress(ProgressRatio);
			}

			if (HealthComponent)
			{
				ProgressWidget->SetHealth(HealthComponent->GetHealthRatio());
			}
		}

		// Opacity UI
		float NewOpacity = FMath::Lerp(1.0f, 0.0f, ProgressRatio);

		for (UMaterialInstanceDynamic* DMI : RealMeshDMI)
		{
			if (DMI)
			{
				DMI->SetScalarParameterValue(TEXT("Opacity"), 1-NewOpacity);
			}
		}

		if (GhostMaterialDMI)
		{
			GhostMaterialDMI->SetScalarParameterValue(TEXT("Opacity"), NewOpacity);
			// GhostMaterialDMI->SetScalarParameterValue(TEXT("EffectPhase"), Ratio); 
		}

		if (CurrentProgress >= ConstructionTime)
		{
			SpawnRealBuilding();
		}
	}
}

void ABVConstructionSite::InitConstruction(TSubclassOf<ABVBuildingBase> InBuildingClass, FGenericTeamId InTeamId, float InConstructionTime)
{
	TargetBuildingClass = InBuildingClass;
	TeamId = InTeamId;
	ConstructionTime = InConstructionTime;

	if (TargetBuildingClass)
	{
		ABVBuildingBase* DefaultBuilding = Cast<ABVBuildingBase>(TargetBuildingClass->GetDefaultObject());
		if (DefaultBuilding)
		{
			if (DefaultBuilding->GetStaticMeshComponent())
			{
				UStaticMesh* TargetMesh = DefaultBuilding->GetStaticMeshComponent()->GetStaticMesh();
				if (TargetMesh && MeshComponent)
				{
					MeshComponent->SetStaticMesh(TargetMesh);
					MeshComponent->SetRelativeScale3D(DefaultBuilding->GetStaticMeshComponent()->GetRelativeScale3D());
					MeshComponent->SetRelativeLocationAndRotation(
						DefaultBuilding->GetStaticMeshComponent()->GetRelativeLocation(),
						DefaultBuilding->GetStaticMeshComponent()->GetRelativeRotation()
					);

					RealMeshDMI.Empty();
					int32 NumMaterials = MeshComponent->GetNumMaterials();
					for (int32 i=0; i<NumMaterials; ++i)
					{
						UMaterialInstanceDynamic* DMI = MeshComponent->CreateAndSetMaterialInstanceDynamic(i);
						if (DMI)
						{
							RealMeshDMI.Add(DMI);
							DMI->SetScalarParameterValue(TEXT("Opacity"), 0.0f);
						}
					}

					if (GhostMeshComponent)
					{
						GhostMeshComponent->SetStaticMesh(TargetMesh);
						GhostMeshComponent->SetRelativeTransform(DefaultBuilding->GetStaticMeshComponent()->GetRelativeTransform());
						GhostMeshComponent->SetRelativeScale3D(MeshComponent->GetRelativeScale3D());

						if (GhostMaterialBase)
						{
							GhostMaterialDMI = UMaterialInstanceDynamic::Create(GhostMaterialBase, this);
							if (GhostMaterialDMI)
							{
								int32 MatNum = GhostMeshComponent->GetNumMaterials();
								for (int32 i = 0; i < MatNum; ++i)
								{
									GhostMeshComponent->SetMaterial(i, GhostMaterialDMI);
								}

								GhostMaterialDMI->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.5f, 0.7f, 1.0f, 1.0f));
							}
						}
					}
				}
			}

			if (DefaultBuilding->GetBoxComponent() && BoxComponent)
			{
				FVector TargetExtent = DefaultBuilding->GetBoxComponent()->GetUnscaledBoxExtent();
				BoxComponent->SetBoxExtent(TargetExtent);
				BoxComponent->SetRelativeLocation(DefaultBuilding->GetBoxComponent()->GetRelativeLocation());
			}

			if (DefaultBuilding->GetSceneRootComponent())
			{
				USceneComponent* TargetSceneRoot = DefaultBuilding->GetSceneRootComponent();
				SceneRootComponent->SetRelativeScale3D(TargetSceneRoot->GetRelativeScale3D());
			}
		}
	}

	bIsBuilding = true;

}

void ABVConstructionSite::SpawnRealBuilding()
{
	if (!TargetBuildingClass || bIsDestroyed) return;

	bIsBuilding = false;

	float FinalHealthRatio = 1.0f;
	if (HealthComponent)
	{
		FinalHealthRatio = HealthComponent->GetHealthRatio();
	}

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Params.Owner = GetOwner();

	ABVBuildingBase* NewBuilding = GetWorld()->SpawnActor<ABVBuildingBase>(
		TargetBuildingClass,
		GetActorLocation(),
		GetActorRotation(),
		Params);

	if (NewBuilding)
	{
		// New Building TeamId
		if (IGenericTeamAgentInterface* NewBuildingTeam = Cast<IGenericTeamAgentInterface>(NewBuilding))
		{
			NewBuildingTeam->SetGenericTeamId(TeamId);
		}

		// New Builing Health Value
		if (NewBuilding->GetCombatAttributeSet())
		{
			float SetupMaxHealth = NewBuilding->GetCombatAttributeSet()->GetMaxHealth();
			NewBuilding->GetCombatAttributeSet()->SetHealth(SetupMaxHealth * FinalHealthRatio);
		}

		if (TeamId.GetId() == 1) 
		{
			if (ABVPlayerController* BVPC = Cast<ABVPlayerController>(GetWorld()->GetFirstPlayerController()))
			{
				BVPC->PlayAnnouncerVoice(EBVAnnouncerEvent::ConstructionCompleted);
			}
		}
	}

	Destroy();
}

