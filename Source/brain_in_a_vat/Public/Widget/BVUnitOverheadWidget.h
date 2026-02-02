// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "BVUnitOverheadWidget.generated.h"

/**
 * 
 */
UCLASS()
class BRAIN_IN_A_VAT_API UBVUnitOverheadWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(meta = (BindWidget))
	class UTextBlock* UnitNameText;
	
	UPROPERTY(meta = (BindWidget))
	class UProgressBar* HealthBar;

	void SetUnitName(FText NewName);
	void InitWithHealthComponent(class UBVHealthComponent* InHealthComponent);

protected:

	UFUNCTION()
	void HandleHealthChanged(float NewRatio);
};
