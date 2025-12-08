// Fill out your copyright notice in the Description page of Project Settings.


#include "Buildings/Details/ABVBuildingBarrack.h"
#include "UObject/ConstructorHelpers.h"
#include "Components/StaticMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "Autobots/BVAutobotRed.h"

// Sets default values
AABVBuildingBarrack::AABVBuildingBarrack()
{

	PrimaryActorTick.bCanEverTick = true;

	CapsuleComponent = CreateDefaultSubobject<UCapsuleComponent>(TEXT("Capsule"));
	RootComponent = CapsuleComponent;
	CapsuleComponent->InitCapsuleSize(20.f, 30.f);
	CapsuleComponent->SetCollisionProfileName(TEXT("BlockAll"));
	
	StaticMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMesh"));
	StaticMeshComponent->SetupAttachment(RootComponent);
	StaticMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	
	static ConstructorHelpers::FObjectFinder<UStaticMesh> StaticMeshRef(TEXT("/Script/Engine.StaticMesh'/Game/buildings/barrack/barrack.barrack'"));
	if (StaticMeshRef.Object != nullptr)
	{
		StaticMeshComponent->SetStaticMesh(StaticMeshRef.Object);
	}

	SpawnUnitClass = ABVAutobotRed::StaticClass();
	RespawnInterval = 10.f;

}
