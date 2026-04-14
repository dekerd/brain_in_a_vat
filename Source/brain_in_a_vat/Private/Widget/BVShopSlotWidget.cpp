// Fill out your copyright notice in the Description page of Project Settings.


#include "Widget/BVShopSlotWidget.h"

#include "BVPlayerController.h"
#include "BVPlayerState.h"
#include "MainCharacter.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Item/BVItemData.h"
#include "Widget/BVShopTooltipWidget.h"

class AMainCharacter;
class ABVPlayerState;

void UBVShopSlotWidget::UpdateSlot(class UBVItemData* InItemData)
{
	ItemData = InItemData;

	if (ItemData && ItemIconImage)
	{
		ItemIconImage->SetBrushFromTexture(ItemData->ItemIcon);

		if (TooltipClass)
		{
			UBVShopTooltipWidget* TooltipWidget = CreateWidget<UBVShopTooltipWidget>(this, TooltipClass);
			if (TooltipWidget)
			{
				TooltipWidget->SetupTooltip(ItemData);
				ItemButton->SetToolTip(TooltipWidget); 
			}
		}
	}
}

void UBVShopSlotWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (ItemButton)
	{
		// Bind the click event
		ItemButton->OnClicked.AddDynamic(this, &UBVShopSlotWidget::OnItemButtonClicked);
	}
}

void UBVShopSlotWidget::OnItemButtonClicked()
{
	if (!ItemData) return;

	ABVPlayerController* PC = Cast<ABVPlayerController>(GetOwningPlayer());
	if (!PC) return;

	ABVPlayerState* PS = PC->GetPlayerState<ABVPlayerState>();
	AMainCharacter* Character = PC->HeroCharacter;

	if (PS && Character)
	{
		if (PS->GetGold() >= ItemData->ItemPrice)
		{
			// Deduct gold and add item to inventory
			PS->AddRewards(-ItemData->ItemPrice, 0.0f);
			Character->AddItemToInventory(ItemData);

			if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Green, TEXT("Purchase Successful!"));
		}
		else
		{
			if (ABVPlayerController* BVPC = Cast<ABVPlayerController>(GetOwningPlayer()))
			{
				BVPC->PlayAnnouncerVoice(EBVAnnouncerEvent::NotEnoughGold);
			}
			if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Red, TEXT("Not enough gold!"));
		}
	}
}
