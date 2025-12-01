// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "BVHealthBarWidget.generated.h"

class UProgressBar;
class ABVAutobotBase;

/**
 * 
 */
UCLASS()
class BRAIN_IN_A_VAT_API UBVHealthBarWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintCallable, Category = "UI")
	void InitWithOwner(ABVAutobotBase* InOwner);

protected:

	UPROPERTY(meta = (BindWidget))
	UProgressBar* HealthProgressBar;

	UFUNCTION()
	void HandleHealthChanged(float NewRatio);
	
	UPROPERTY()
	ABVAutobotBase* OwnerAutobot;
};
