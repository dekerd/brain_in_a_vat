// Fill out your copyright notice in the Description page of Project Settings.


#include "Widget/BVInventoryWidget.h"

#include "MainCharacter.h"
#include "Components/WrapBox.h"
#include "Widget/BVInventorySlotWidget.h"

void UBVInventoryWidget::RefreshInventory()
{

	FString DebugMsg = FString::Printf(TEXT("RefreshInventory has been called!"));
	GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Orange, DebugMsg);
	
	if (!InventoryGrid || !SlotWidgetClass) return;

	InventoryGrid->ClearChildren();

	AMainCharacter* Character = Cast<AMainCharacter>(GetOwningPlayerPawn());
	if (!Character) return;

	const TArray<UBVItemData*>& Items = Character->GetInventoryItems();
	for (UBVItemData* Item : Items)
	{
		UBVInventorySlotWidget* NewSlot = CreateWidget<UBVInventorySlotWidget>(this, SlotWidgetClass);
		if (NewSlot)
		{
			NewSlot->UpdateSlot(Item);
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

	AMainCharacter* MainCharacter = Cast<AMainCharacter>(GetOwningPlayerPawn());
	if (MainCharacter)
	{
		MainCharacter->OnInventoryUpdated.AddDynamic(this, &UBVInventoryWidget::HandleOnAddItem);
	}
	
}
