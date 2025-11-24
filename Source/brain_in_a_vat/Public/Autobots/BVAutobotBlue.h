// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BVAutobotBase.h"
#include "BVAutobotBlue.generated.h"

UCLASS()
class BRAIN_IN_A_VAT_API ABVAutobotBlue : public ABVAutobotBase
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	ABVAutobotBlue();
	
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

};
