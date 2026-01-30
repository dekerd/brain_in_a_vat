// Fill out your copyright notice in the Description page of Project Settings.


#include "Characters/BVNPCBase.h"


// Sets default values
ABVNPCBase::ABVNPCBase()
{
	PrimaryActorTick.bCanEverTick = false;

	StaticMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMeshComponent"));
	
}

// Called when the game starts or when spawned
void ABVNPCBase::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void ABVNPCBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}



