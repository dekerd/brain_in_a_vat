// Fill out your copyright notice in the Description page of Project Settings.


#include "Buildings/Details/BVBaseCamp.h"


// Sets default values
ABVBaseCamp::ABVBaseCamp()
{
	PrimaryActorTick.bCanEverTick = true;

	StaticMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMesh"));
	StaticMeshComponent->SetupAttachment(RootComponent);
	StaticMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	
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
