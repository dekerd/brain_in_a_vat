// Fill out your copyright notice in the Description page of Project Settings.


#include "Widget/BVInventorySlotWidget.h"

#include "BVPlayerController.h"
#include "MainCharacter.h"
#include "Components/Image.h"
#include "Item/BVItemData.h"

void UBVInventorySlotWidget::UpdateSlot(class UBVItemData* InItemData, bool bInIsEquipped)
{
	ItemData = InItemData;
	bIsEquippedSlot = bInIsEquipped;
	
	if (ItemData && ItemIconImage)
	{
		ItemIconImage->SetBrushFromTexture(ItemData->ItemIcon);
	}
}

FReply UBVInventorySlotWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
	{
		ABVPlayerController* PC = Cast<ABVPlayerController>(GetOwningPlayer());
		if (PC && PC->HeroCharacter)
		{
			if (bIsEquippedSlot)
			{
				PC->HeroCharacter->UnequipItem(ItemData);
			}
			else
			{
				PC->HeroCharacter->EquipItem(ItemData);
			}
		}
		return FReply::Handled();
	}
	
	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}
