// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UBVBuildingOverheadWidget.generated.h"

class UBVHealthComponent;
class UProgressBar;
class UTextBlock;
/**
 * 
 */
UCLASS()
class BRAIN_IN_A_VAT_API UUBVBuildingOverheadWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	UPROPERTY(meta = (BindWidget))
	UTextBlock* BuildingNameText;

	UPROPERTY(meta = (BindWidget))
	UProgressBar* HealthBar;

	UPROPERTY(meta = (BindWidget))
	UProgressBar* RespawnBar;

	void SetBuildingName(FText NewName);
	void InitWithHealthComponent(UBVHealthComponent* InHealthComponent);
	void SetRespawnProgress(float Percent);

protected:

	UFUNCTION()
	void HandleHealthChanged(float NewRatio);
};
