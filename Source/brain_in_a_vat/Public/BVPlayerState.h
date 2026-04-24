// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "BVPlayerState.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGoldChange, int32, NewGold);

/**
 * 
 */
UCLASS()
class BRAIN_IN_A_VAT_API ABVPlayerState : public APlayerState
{
	GENERATED_BODY()
public:
	ABVPlayerState();

	virtual void BeginPlay() override;

	void AddRewards(int32 InGold, float InExp);

	UFUNCTION(BlueprintCallable, Category = "Resources")
	int32 GetGold() const {return Gold;}

	UFUNCTION(BlueprintCallable, Category = "Resources")
	float GetExperience() const {return Experience;}

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnGoldChange OnGoldChange;

protected:

	// 레벨 시작 시 Gold의 초기값. BP_PlayerState 디테일 패널 > Resources 카테고리에서 설정.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Resources", meta = (ClampMin = "0"))
	int32 InitialGold = 0;

	// 레벨 시작 시 Experience의 초기값.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Resources", meta = (ClampMin = "0.0"))
	float InitialExperience = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Resources")
	int32 Gold;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Resources")
	float Experience;
};
