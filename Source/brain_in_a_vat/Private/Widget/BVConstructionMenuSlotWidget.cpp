// Fill out your copyright notice in the Description page of Project Settings.


#include "Widget/BVConstructionMenuSlotWidget.h"
#include "BVPlayerController.h"
#include "BVPlayerState.h"
#include "Buildings/BVBuildingBase.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"

class ABVPlayerState;

void UBVConstructionMenuSlotWidget::InitSlot(const FBuildingMenuData& InMenuData)
{
	SlotData = InMenuData;
	BuildingClass = InMenuData.BuildingClass;
	
	if (BuildingIcon && SlotData.BuildingIcon)
	{
		BuildingIcon->SetBrushFromTexture(SlotData.BuildingIcon);
	}
	
	if (BuildingNameText)
	{
		BuildingNameText->SetText(SlotData.BuildingName); 
	}

	if (CostText)
	{
		CostText->SetText(FText::AsNumber(SlotData.ConstructionCost));
	}
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
	if (!BuildingClass) return;

	ABVPlayerController* PC = Cast<ABVPlayerController>(GetOwningPlayer());
	if (!PC) return;

	ABVPlayerState* PS = PC->GetPlayerState<ABVPlayerState>();
	if (PS)
	{
		ABVBuildingBase* BuildingCDO = BuildingClass->GetDefaultObject<ABVBuildingBase>();
		if (BuildingCDO && PS->GetGold() >= SlotData.ConstructionCost)
		{

			PS->AddRewards(-SlotData.ConstructionCost, 0.0f);
			PC->EnterConstructionMode(BuildingClass, SlotData.ConstructionTime);
		}
		else
		{
			if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Red, TEXT("Not enough gold to build!"));
			PC->PlayAnnouncerVoice(EBVAnnouncerEvent::NotEnoughGold);
		}
	}
}
