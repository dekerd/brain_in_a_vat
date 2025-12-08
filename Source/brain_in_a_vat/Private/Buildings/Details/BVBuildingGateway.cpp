// Fill out your copyright notice in the Description page of Project Settings.


#include "Buildings/Details/BVBuildingGateway.h"
#include "UObject/ConstructorHelpers.h"
#include "Components/StaticMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "Autobots/BVAutobotBlue.h"


// Sets default values
ABVBuildingGateway::ABVBuildingGateway()
{
	PrimaryActorTick.bCanEverTick = true;

	CapsuleComponent = CreateDefaultSubobject<UCapsuleComponent>(TEXT("Capsule"));
	RootComponent = CapsuleComponent;
	CapsuleComponent->InitCapsuleSize(20.f, 30.f);
	CapsuleComponent->SetCollisionProfileName(TEXT("BlockAll"));
	
	StaticMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMesh"));
	StaticMeshComponent->SetupAttachment(RootComponent);
	StaticMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	
	static ConstructorHelpers::FObjectFinder<UStaticMesh> StaticMeshRef(TEXT("/Script/Engine.StaticMesh'/Game/buildings/Gateway/gateway.gateway'"));
	if (StaticMeshRef.Object != nullptr)
	{
		StaticMeshComponent->SetStaticMesh(StaticMeshRef.Object);
	}

	SpawnUnitClass = ABVAutobotBlue::StaticClass();
	RespawnInterval = 10.f;
	
}
