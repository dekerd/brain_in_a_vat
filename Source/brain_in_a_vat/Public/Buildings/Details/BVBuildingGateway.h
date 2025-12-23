// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Buildings/BVBuildingBase.h"
#include "BVBuildingGateway.generated.h"

UCLASS()
class BRAIN_IN_A_VAT_API ABVBuildingGateway : public ABVBuildingBase
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ABVBuildingGateway();

protected:
	virtual void OnConstruction(const FTransform& Transform) override;
	
};
