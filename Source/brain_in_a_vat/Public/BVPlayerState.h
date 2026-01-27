// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "BVPlayerState.generated.h"

/**
 * 
 */
UCLASS()
class BRAIN_IN_A_VAT_API ABVPlayerState : public APlayerState
{
	GENERATED_BODY()
public:
	ABVPlayerState();

	void AddRewards(int32 InGold, float InExp);

	UFUNCTION(BlueprintCallable, Category = "Resources")
	int32 GetGold() const {return Gold;}

	UFUNCTION(BlueprintCallable, Category = "Resources")
	float GetExperience() const {return Experience;}

protected:

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Resources")
	int32 Gold;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Resources")
	float Experience;
};
