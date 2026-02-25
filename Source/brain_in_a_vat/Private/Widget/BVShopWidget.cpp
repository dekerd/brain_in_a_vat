// Fill out your copyright notice in the Description page of Project Settings.


#include "Widget/BVShopWidget.h"
#include "Characters/BVNPCBase.h"
#include "BVPlayerController.h"
#include "Components/Button.h"
#include "Components/Image.h"

void UBVShopWidget::InitShop(ABVNPCBase* InNPC)
{
	if (InNPC && NPC_PortraitImage)
	{
		NPC_PortraitImage->SetBrushFromTexture(InNPC->NPCPortrait);
	}
}

void UBVShopWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// Bind the click event to the CloseButton
	if (CloseButton)
	{
		CloseButton->OnClicked.AddDynamic(this, &UBVShopWidget::OnCloseButtonClicked);
	}
}

void UBVShopWidget::OnCloseButtonClicked()
{
	// Get the player controller and call CloseShopUI
	if (ABVPlayerController* PC = Cast<ABVPlayerController>(GetOwningPlayer()))
	{
		PC->CloseShopUI();
	}
}
