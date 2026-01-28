// Fill out your copyright notice in the Description page of Project Settings.


#include "Widget/BVResourceHUDWidget.h"

#include "BVPlayerState.h"
#include "Components/TextBlock.h"

void UBVResourceHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (APlayerController* PC = GetOwningPlayer())
	{
		if (ABVPlayerState* PS = PC->GetPlayerState<ABVPlayerState>())
		{
			PS->OnGoldChange.AddDynamic(this, &UBVResourceHUDWidget::UpdateGoldText);
			UpdateGoldText(PS->GetGold());
		}
	}
}

void UBVResourceHUDWidget::UpdateGoldText(int32 NewGold)
{
	if (GoldTotalText)
	{
		GoldTotalText->SetText(FText::AsNumber(NewGold));
	}
}
