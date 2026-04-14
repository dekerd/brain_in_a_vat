// Fill out your copyright notice in the Description page of Project Settings.


#include "Widget/BVInventoryWidget.h"
#include "BVPlayerController.h"
#include "MainCharacter.h"
#include "Components/WrapBox.h"
#include "Widget/BVInventorySlotWidget.h"

void UBVInventoryWidget::RefreshInventory()
{

	if (!InventoryGrid || !EquippedGrid || !SlotWidgetClass) return;

	InventoryGrid->ClearChildren();
	EquippedGrid->ClearChildren();

	ABVPlayerController* PC = Cast<ABVPlayerController>(GetOwningPlayer());
	if (!PC || !PC->HeroCharacter) return;

	AMainCharacter* Character = PC->HeroCharacter;

	for (UBVItemData* Item : Character->GetEquippedWeapons())
	{
		UBVInventorySlotWidget* NewSlot = CreateWidget<UBVInventorySlotWidget>(this, SlotWidgetClass);
		if (NewSlot)
		{
			NewSlot->UpdateSlot(Item, true);
			EquippedGrid->AddChildToWrapBox(NewSlot);
		}
	}

	for (UBVItemData* Item : Character->GetInventoryItems())
	{
		UBVInventorySlotWidget* NewSlot = CreateWidget<UBVInventorySlotWidget>(this, SlotWidgetClass);
		if (NewSlot)
		{
			NewSlot->UpdateSlot(Item, false);
			InventoryGrid->AddChildToWrapBox(NewSlot);
		}
	}
}

void UBVInventoryWidget::HandleOnAddItem()
{
	RefreshInventory();
}

void UBVInventoryWidget::NativeConstruct()
{
	Super::NativeConstruct();

	ABVPlayerController* PC = Cast<ABVPlayerController>(GetOwningPlayer());
	if (PC && PC->HeroCharacter)
	{
		PC->HeroCharacter->OnInventoryUpdated.AddDynamic(this, &UBVInventoryWidget::HandleOnAddItem);
	}
	
}
