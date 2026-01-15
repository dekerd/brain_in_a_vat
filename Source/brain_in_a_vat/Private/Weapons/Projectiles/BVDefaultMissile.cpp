// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapons/Projectiles/BVDefaultMissile.h"


// Sets default values
ABVDefaultMissile::ABVDefaultMissile()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

// Called when the game starts or when spawned
void ABVDefaultMissile::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void ABVDefaultMissile::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

