// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "BVSpawnCooltimeBar.generated.h"

/**
 * 
 */
UCLASS()
class BRAIN_IN_A_VAT_API UBVSpawnCooltimeBar : public UUserWidget
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintCallable)
	void SetProgress(float InPercent);

protected:

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<class UProgressBar> ProgressBar;
};
