// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "BVShopTooltipWidget.generated.h"

/**
 * 
 */
UCLASS()
class BRAIN_IN_A_VAT_API UBVShopTooltipWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// Set up the tooltip with item data
	void SetupTooltip(class UBVItemData* InItemData);

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<class UImage> TooltipIcon;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<class UTextBlock> ItemNameText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<class UTextBlock> ItemDescText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<class UTextBlock> ItemPriceText;
};
