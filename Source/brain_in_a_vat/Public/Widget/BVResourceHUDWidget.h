// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "BVResourceHUDWidget.generated.h"

/**
 * 
 */
UCLASS()
class BRAIN_IN_A_VAT_API UBVResourceHUDWidget : public UUserWidget
{
	GENERATED_BODY()
public:

	virtual void NativeConstruct() override;

	UFUNCTION()
	void UpdateGoldText(int32 NewGold);

protected:

	UPROPERTY(meta = (BindWidget))
	class UTextBlock* GoldTotalText;
};
