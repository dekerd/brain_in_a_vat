// Fill out your copyright notice in the Description page of Project Settings.


#include "Widget/BVShopTooltipWidget.h"

#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Item/BVItemData.h"

void UBVShopTooltipWidget::SetupTooltip(class UBVItemData* InItemData)
{
	if (!InItemData) return;

	if (TooltipIcon)
	{
		UTexture2D* ImageToUse = InItemData->ItemFullImage ? InItemData->ItemFullImage : InItemData->ItemIcon;
		TooltipIcon->SetBrushFromTexture(ImageToUse);
	}

	if (ItemNameText)
	{
		ItemNameText->SetText(InItemData->ItemName);
	}

	if (ItemDescText)
	{
		ItemDescText->SetText(InItemData->ItemDescription);
	}

	if (ItemPriceText)
	{
		FString PriceString = FString::Printf(TEXT("%d G"), InItemData->ItemPrice);
		ItemPriceText->SetText(FText::FromString(PriceString));
	}
}
