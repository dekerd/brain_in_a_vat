// Fill out your copyright notice in the Description page of Project Settings.


#include "Buildings/BVConstructionSite.h"

#include "Buildings/BVBuildingBase.h"
#include "Collision/BVCollision.h"
#include "Components/BoxComponent.h"


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
	
}

// Called when the game starts or when spawned
void ABVConstructionSite::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void ABVConstructionSite::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bIsBuilding)
	{
		CurrentProgress += DeltaTime;

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
		if (DefaultBuilding && DefaultBuilding->GetStaticMeshComponent())
		{
			UStaticMesh* TargetMesh = DefaultBuilding->GetStaticMeshComponent()->GetStaticMesh();
			if (TargetMesh)
			{
				MeshComponent->SetStaticMesh(TargetMesh);
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

