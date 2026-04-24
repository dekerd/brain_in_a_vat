// Fill out your copyright notice in the Description page of Project Settings.


#include "Widget/BVConstructionMenuWidget.h"
#include "Widget/BVConstructionMenuSlotWidget.h"
#include "Components/WrapBox.h"
#include "BVPlayerController.h"
#include "Buildings/BuildingMenuData.h"
#include "Buildings/BVBuildingBase.h"
#include "Buildings/BVCityBase.h"
#include "Data/BVBuildingData.h"
#include "Data/BVBuildingCatalog.h"

void UBVConstructionMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// 초기 오픈 시엔 글로벌 Hero 카탈로그. City 컨텍스트로 오픈할 땐 외부에서 Populate(City)를 추가 호출.
	Populate(nullptr);
}

void UBVConstructionMenuWidget::Populate(ABVCityBase* InCity)
{
	BoundCity = InCity;

	if (!BuildingSlotContainer || !SlotWidgetClass) return;

	// 컨텍스트가 바뀔 수 있으므로 항상 비우고 재구성.
	BuildingSlotContainer->ClearChildren();

	// City-only 빌드: City가 없으면 슬롯을 만들지 않고 빈 메뉴를 보여준다.
	// (Hero 주도 글로벌 카탈로그 경로는 제거됨.)
	if (!InCity) return;

	for (const TSubclassOf<ABVBuildingBase>& BuildingClass : InCity->BuildableBuildings)
	{
		if (!BuildingClass) continue;

		const ABVBuildingBase* CDO = BuildingClass->GetDefaultObject<ABVBuildingBase>();
		UBVBuildingData* Data = CDO ? CDO->BuildingData : nullptr;
		if (!Data || !Data->BuildingClass) continue;

		UBVConstructionMenuSlotWidget* NewSlot = CreateWidget<UBVConstructionMenuSlotWidget>(this, SlotWidgetClass);
		if (!NewSlot) continue;

		NewSlot->InitSlotForCity(Data, InCity);
		BuildingSlotContainer->AddChildToWrapBox(NewSlot);
	}
}
