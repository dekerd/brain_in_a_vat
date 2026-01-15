// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BVProjectileBase.h"
#include "BVDefaultMissile.generated.h"

UCLASS()
class BRAIN_IN_A_VAT_API ABVDefaultMissile : public ABVProjectileBase
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ABVDefaultMissile();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;
};
