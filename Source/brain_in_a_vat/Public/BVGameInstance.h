// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "BVGameInstance.generated.h"

/**
 * 
 */
UCLASS()
class BRAIN_IN_A_VAT_API UBVGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	virtual void Init() override;
};
