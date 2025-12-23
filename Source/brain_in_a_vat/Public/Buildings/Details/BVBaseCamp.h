// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Buildings/BVBuildingBase.h"
#include "BVBaseCamp.generated.h"

UCLASS()
class BRAIN_IN_A_VAT_API ABVBaseCamp : public ABVBuildingBase
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ABVBaseCamp();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	virtual void OnConstruction(const FTransform& Transform) override;

};
