// Fill out your copyright notice in the Description page of Project Settings.


#include "Widget/BVConstructionMenuWidget.h"
#include "Widget/BVConstructionMenuSlotWidget.h"
#include "Components/WrapBox.h"
#include "BVPlayerController.h"
#include "Buildings/BVBuildingBase.h"

void UBVConstructionMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (!BuildingSlotContainer || !SlotWidgetClass) return;

	BuildingSlotContainer->ClearChildren();

	ABVPlayerController* PC = Cast<ABVPlayerController>(GetOwningPlayer());
	if (PC)
	{
		for (TSubclassOf<ABVBuildingBase> BuildingClass : PC->AvailableBuildings)
		{
			if (BuildingClass)
			{
				UBVConstructionMenuSlotWidget* NewSlot = CreateWidget<UBVConstructionMenuSlotWidget>(this, SlotWidgetClass);
				if (NewSlot)
				{
					NewSlot->InitSlot(BuildingClass);
					BuildingSlotContainer->AddChildToWrapBox(NewSlot);
				}
			}
		}
	}
}
