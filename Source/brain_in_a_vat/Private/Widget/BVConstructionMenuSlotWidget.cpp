// Fill out your copyright notice in the Description page of Project Settings.


#include "Widget/BVConstructionMenuSlotWidget.h"

#include "BVPlayerController.h"
#include "BVPlayerState.h"
#include "Buildings/BVBuildingBase.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"

class ABVPlayerState;

void UBVConstructionMenuSlotWidget::InitSlot(TSubclassOf<ABVBuildingBase> InBuildingClass)
{
	if (!InBuildingClass) return;
	
	BuildingClass = InBuildingClass;
	
	ABVBuildingBase* BuildingCDO = BuildingClass->GetDefaultObject<ABVBuildingBase>();
	if (BuildingCDO)
	{
		if (BuildingIcon && BuildingCDO->BuildingIcon)
		{
			BuildingIcon->SetBrushFromTexture(BuildingCDO->BuildingIcon);
		}
		
		if (BuildingNameText)
		{
			BuildingNameText->SetText(BuildingCDO->BuildingName); 
		}

		if (CostText)
		{
			CostText->SetText(FText::AsNumber(BuildingCDO->ConstructionCost));
		}
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
		if (BuildingCDO && PS->GetGold() >= BuildingCDO->ConstructionCost)
		{

			PS->AddRewards(-BuildingCDO->ConstructionCost, 0.0f);
			PC->EnterConstructionMode(BuildingClass);
		}
		else
		{
			if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Red, TEXT("Not enough gold to build!"));
			PC->PlayAnnouncerVoice(EBVAnnouncerEvent::NotEnoughGold);
		}
	}
}
