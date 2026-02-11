// Fill out your copyright notice in the Description page of Project Settings.


#include "Buildings/BVConstructionSite.h"

#include "Buildings/BVBuildingBase.h"
#include "Collision/BVCollision.h"
#include "Components/BoxComponent.h"
#include "Components/WidgetComponent.h"
#include "Widget/BVConstructionWidget.h"


// Sets default values
ABVConstructionSite::ABVConstructionSite()
{
	PrimaryActorTick.bCanEverTick = true;

	SceneRootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRootComponent;

	BoxComponent = CreateDefaultSubobject<UBoxComponent>(TEXT("BoxComponent"));
	BoxComponent->SetupAttachment(RootComponent);
	BoxComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	BoxComponent->SetGenerateOverlapEvents(true);
	BoxComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
	BoxComponent->SetCollisionObjectType(ECC_Building);

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	MeshComponent->SetupAttachment(RootComponent);
	MeshComponent->SetCollisionProfileName(TEXT("NoCollision"));

	GhostMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GhostMeshComponent"));
	GhostMeshComponent->SetupAttachment(RootComponent);
	GhostMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GhostMeshComponent->SetCastShadow(false);

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

	if (bIsBuilding)
	{
		CurrentProgress += DeltaTime;
		float ProgressRatio = FMath::Clamp(CurrentProgress / ConstructionTime, 0.0f, 1.0f);

		if (ProgressWidgetComponent)
		{
			UBVConstructionWidget* ProgressWidget = Cast<UBVConstructionWidget>(ProgressWidgetComponent->GetUserWidgetObject());
			if (ProgressWidget)
			{
				ProgressWidget->SetProgress(ProgressRatio);
			}
		}

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

void ABVConstructionSite::InitConstruction(TSubclassOf<ABVBuildingBase> InBuildingClass, FGenericTeamId InTeamId)
{
	TargetBuildingClass = InBuildingClass;
	TeamId = InTeamId;

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
	if (!TargetBuildingClass) return;

	bIsBuilding = false;

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
		if (IGenericTeamAgentInterface* NewBuildingTeam = Cast<IGenericTeamAgentInterface>(NewBuilding))
		{
			NewBuildingTeam->SetGenericTeamId(TeamId);
		}
	}

	Destroy();
}

