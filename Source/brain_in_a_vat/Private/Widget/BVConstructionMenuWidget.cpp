// Fill out your copyright notice in the Description page of Project Settings.


#include "Widget/BVConstructionMenuWidget.h"
#include "Widget/BVConstructionMenuSlotWidget.h"
#include "Components/WrapBox.h"
#include "BVPlayerController.h"
#include "Buildings/BuildingMenuData.h"
#include "Buildings/BVBuildingBase.h"
#include "Data/BVBuildingData.h"

void UBVConstructionMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (!BuildingSlotContainer || !SlotWidgetClass) return;

	if (BuildingSlotContainer->HasAnyChildren())
	{
		return;
	}

	BuildingSlotContainer->ClearChildren();

	for (UBVBuildingData* Data : AvailableBuildingsData)
	{
		if (Data && Data->BuildingClass && Data->TeamType == EBVTeam::Player)
		{
			UBVConstructionMenuSlotWidget* NewSlot = CreateWidget<UBVConstructionMenuSlotWidget>(this, SlotWidgetClass);
			if (NewSlot)
			{
				NewSlot->InitSlot(Data);
				BuildingSlotContainer->AddChildToWrapBox(NewSlot);
			}
		}
	}
}
