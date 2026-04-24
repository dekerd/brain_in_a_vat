// Fill out your copyright notice in the Description page of Project Settings.


#include "Widget/BVConstructionMenuSlotWidget.h"
#include "BVPlayerController.h"
#include "BVPlayerState.h"
#include "Buildings/BVBuildingBase.h"
#include "Buildings/BVCityBase.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Data/BVBuildingData.h"

class ABVPlayerState;

void UBVConstructionMenuSlotWidget::InitSlot(UBVBuildingData* InData)
{
	OwningCity.Reset(); // 일반 빌드 모드 = 도시 없음.

	SlotData = InData;
	if (!SlotData) return;

	BuildingClass = SlotData->BuildingClass;

	if (BuildingIcon && SlotData->BuildingIcon)
		BuildingIcon->SetBrushFromTexture(SlotData->BuildingIcon);

	if (BuildingNameText)
		BuildingNameText->SetText(SlotData->BuildingName);

	if (CostText)
		CostText->SetText(FText::AsNumber(SlotData->ConstructionCost));
}

void UBVConstructionMenuSlotWidget::InitSlotForCity(UBVBuildingData* InData, ABVCityBase* InOwningCity)
{
	// 일반 InitSlot의 시각 세팅을 그대로 재사용한 뒤 City 컨텍스트만 덮어씀.
	InitSlot(InData);
	OwningCity = InOwningCity;
}

void UBVConstructionMenuSlotWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (BuildButton)
	{
		BuildButton->OnClicked.AddDynamic(this, &UBVConstructionMenuSlotWidget::OnButtonClicked);
	}
}

void UBVConstructionMenuSlotWidget::OnButtonClicked()
{
	if (!BuildingClass || !SlotData) return;

	ABVPlayerController* PC = Cast<ABVPlayerController>(GetOwningPlayer());
	if (!PC) return;

	ABVPlayerState* PS = PC->GetPlayerState<ABVPlayerState>();
	if (!PS) return;

	ABVBuildingBase* BuildingCDO = BuildingClass->GetDefaultObject<ABVBuildingBase>();
	if (!BuildingCDO) return;

	// City-only 빌드: OwningCity가 없으면 이 슬롯은 동작하지 않는다.
	// (Hero 주도 빌드 경로는 제거됨 — 글로벌 카탈로그를 쓰던 슬롯은 이 체크로 차단된다.)
	ABVCityBase* City = OwningCity.Get();
	if (!City)
	{
		return;
	}

	if (PS->GetGold() < SlotData->ConstructionCost)
	{
		PC->PlayAnnouncerVoice(EBVAnnouncerEvent::NotEnoughGold);
		return;
	}

	PS->AddRewards(-SlotData->ConstructionCost, 0.0f);

	PC->EnterCityBuildMode(City, BuildingClass, SlotData->ConstructionTime);
}
