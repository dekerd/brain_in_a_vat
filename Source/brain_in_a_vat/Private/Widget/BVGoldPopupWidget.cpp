// Fill out your copyright notice in the Description page of Project Settings.


#include "Widget/BVGoldPopupWidget.h"
#include "Components/TextBlock.h"

void UBVGoldPopupWidget::InitPopup(int32 Amount, FVector WorldLocation)
{
	if (GoldAmountText)
	{
		FString GoldString = FString::Printf(TEXT("+%d"), Amount);
		GoldAmountText->SetText(FText::FromString(GoldString));
	}
	
	TargetWorldLocation = WorldLocation;

	FTimerHandle TimerHandle;
	GetWorld()->GetTimerManager().SetTimer(TimerHandle,
		this,
		&UBVGoldPopupWidget::RemoveFromParent,
		2.0f,
		false);
	
}

void UBVGoldPopupWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	APlayerController* PC = GetOwningPlayer();
	if (PC)
	{
		FVector2D ScreenPosition;
		if (PC->ProjectWorldLocationToScreen(TargetWorldLocation, ScreenPosition))
		{
			SetPositionInViewport(ScreenPosition);
		}
	}
	
}
