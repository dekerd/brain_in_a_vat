// Fill out your copyright notice in the Description page of Project Settings.


#include "Widget/BVConstructionMenuWidget.h"
#include "Widget/BVConstructionMenuSlotWidget.h"
#include "Components/WrapBox.h"
#include "BVPlayerController.h"
#include "Buildings/BuildingMenuData.h"
#include "Buildings/BVBuildingBase.h"
#include "Data/BVBuildingData.h"
#include "Data/BVBuildingCatalog.h"

void UBVConstructionMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (!BuildingSlotContainer || !SlotWidgetClass) return;

	if (BuildingSlotContainer->HasAnyChildren())
	{
		return;
	}

	BuildingSlotContainer->ClearChildren();

	// 카탈로그가 설정돼 있으면 그걸 우선 사용, 아니면 레거시 배열 fallback
	const TArray<TObjectPtr<UBVBuildingData>>& SourceList =
		(BuildingCatalog && BuildingCatalog->Buildings.Num() > 0)
			? BuildingCatalog->Buildings
			: AvailableBuildingsData;

	UE_LOG(LogTemp, Warning, TEXT("[ConstructionMenu] Catalog=%s, CatalogNum=%d, FallbackNum=%d, UsingNum=%d"),
		BuildingCatalog ? *BuildingCatalog->GetName() : TEXT("NULL"),
		BuildingCatalog ? BuildingCatalog->Buildings.Num() : -1,
		AvailableBuildingsData.Num(),
		SourceList.Num());

	for (UBVBuildingData* Data : SourceList)
	{
		if (!Data)
		{
			UE_LOG(LogTemp, Warning, TEXT("[ConstructionMenu] Skipped: Data is NULL"));
			continue;
		}
		UE_LOG(LogTemp, Warning, TEXT("[ConstructionMenu] Data=%s, BuildingClass=%s, TeamType=%d"),
			*Data->GetName(),
			Data->BuildingClass ? *Data->BuildingClass->GetName() : TEXT("NULL"),
			(int32)Data->TeamType);

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
