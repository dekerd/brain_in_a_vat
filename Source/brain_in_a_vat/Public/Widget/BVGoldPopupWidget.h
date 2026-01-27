// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "BVGoldPopupWidget.generated.h"

/**
 * 
 */
UCLASS()
class BRAIN_IN_A_VAT_API UBVGoldPopupWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetGoldAmount(int32 Amount);
	
protected:

	UPROPERTY(meta = (BindWidget))
	class UTextBlock* GoldAmountText;
	
};
