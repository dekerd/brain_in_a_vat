// Fill out your copyright notice in the Description page of Project Settings.


#include "Widget/BVGoldPopupWidget.h"
#include "Components/TextBlock.h"

void UBVGoldPopupWidget::SetGoldAmount(int32 Amount)
{
	if (GoldAmountText)
	{
		FString GoldString = FString::Printf(TEXT("+%d"), Amount);
		GoldAmountText->SetText(FText::FromString(GoldString));
	}
}
