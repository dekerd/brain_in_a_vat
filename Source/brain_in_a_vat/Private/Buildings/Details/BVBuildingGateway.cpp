// Fill out your copyright notice in the Description page of Project Settings.


#include "Buildings/Details/BVBuildingGateway.h"
#include "UObject/ConstructorHelpers.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "Autobots/BVAutobotBlue.h"


// Sets default values
ABVBuildingGateway::ABVBuildingGateway()
{
	PrimaryActorTick.bCanEverTick = true;
	
	static ConstructorHelpers::FObjectFinder<UStaticMesh> StaticMeshRef(TEXT("/Script/Engine.StaticMesh'/Game/buildings/Gateway/gateway.gateway'"));
	if (StaticMeshRef.Object != nullptr)
	{
		StaticMeshComponent->SetStaticMesh(StaticMeshRef.Object);
	}

	SpawnUnitClass = ABVAutobotBlue::StaticClass();
	RespawnInterval = 10.f;

	TeamFlag = 1;
	StatRowName = TEXT("Gateway");
	
}

void ABVBuildingGateway::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	if (StaticMeshComponent && BoxComponent)
	{
		FBox MeshBounds = StaticMeshComponent->GetStaticMesh()->GetBoundingBox();
		BoxComponent->SetBoxExtent(MeshBounds.GetExtent());
	}
	
}
