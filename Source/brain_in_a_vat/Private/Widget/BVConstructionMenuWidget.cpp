// Fill out your copyright notice in the Description page of Project Settings.


#include "Widget/BVConstructionMenuWidget.h"
#include "Widget/BVConstructionMenuSlotWidget.h"
#include "Components/WrapBox.h"
#include "BVPlayerController.h"
#include "Buildings/BuildingMenuData.h"
#include "Buildings/BVBuildingBase.h"

void UBVConstructionMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (!BuildingSlotContainer || !SlotWidgetClass) return;

	if (BuildingSlotContainer->HasAnyChildren())
	{
		return;
	}

	BuildingSlotContainer->ClearChildren();

	if (BuildingMenuDataTable)
	{
		static const FString ContextString(TEXT("Building Menu Context"));
		TArray<FBuildingMenuData*> MenuRows;
		
		BuildingMenuDataTable->GetAllRows<FBuildingMenuData>(ContextString, MenuRows);

		for (FBuildingMenuData* RowData : MenuRows)
		{
			if (RowData && RowData->BuildingClass)
			{
				UBVConstructionMenuSlotWidget* NewSlot = CreateWidget<UBVConstructionMenuSlotWidget>(this, SlotWidgetClass);
				if (NewSlot)
				{
					NewSlot->InitSlot(*RowData);
					BuildingSlotContainer->AddChildToWrapBox(NewSlot);
				}
			}
		}
	}
}
