// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "BVConstructionWidget.generated.h"

class UProgressBar;
/**
 * 
 */
UCLASS()
class BRAIN_IN_A_VAT_API UBVConstructionWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetProgress(float InPercent);
	void SetHealth(float InPercent);

protected:

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> ConstructionProgressBar;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> HealthBar;
};
