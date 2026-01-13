// Fill out your copyright notice in the Description page of Project Settings.


#include "Widget/BVInventorySlotWidget.h"
#include "Components/Image.h"
#include "Item/BVItemData.h"

void UBVInventorySlotWidget::UpdateSlot(class UBVItemData* ItemData)
{
	if (ItemData && ItemIconImage)
	{
		ItemIconImage->SetBrushFromTexture(ItemData->ItemIcon);
	}
}
