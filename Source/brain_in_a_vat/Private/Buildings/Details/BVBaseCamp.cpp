// Fill out your copyright notice in the Description page of Project Settings.


#include "Buildings/Details/BVBaseCamp.h"
#include "Components/BoxComponent.h"
#include "BVGameModeBase.h"


// Sets default values
ABVBaseCamp::ABVBaseCamp()
{
	PrimaryActorTick.bCanEverTick = true;
	
	static ConstructorHelpers::FObjectFinder<UStaticMesh> StaticMeshRef(TEXT("/Script/Engine.StaticMesh'/Game/buildings/first_building/low_poly/base_building_low_poly.base_building_low_poly'"));
	if (StaticMeshRef.Object != nullptr)
	{
		StaticMeshComponent->SetStaticMesh(StaticMeshRef.Object);
	}

	TeamFlag = 1;
	StatRowName = TEXT("Basecamp");
	
}

// Called when the game starts or when spawned
void ABVBaseCamp::BeginPlay()
{
	Super::BeginPlay();
	
}

void ABVBaseCamp::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	/*
	if (StaticMeshComponent && BoxComponent)
	{
		FBox MeshBounds = StaticMeshComponent->GetStaticMesh()->GetBoundingBox();
		BoxComponent->SetBoxExtent(MeshBounds.GetExtent());
	}
	*/
}

void ABVBaseCamp::DestroyBuilding()
{
	// Defeat condition
	Super::DestroyBuilding();

	if (UWorld* World = GetWorld())
	{
		if (ABVGameModeBase* GameMode = Cast<ABVGameModeBase>(World->GetAuthGameMode()))
		{
			GameMode->GameOver(TeamFlag);
		}
	}
	
}
