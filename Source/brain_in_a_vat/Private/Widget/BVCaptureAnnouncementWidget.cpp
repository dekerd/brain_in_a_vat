// Fill out your copyright notice in the Description page of Project Settings.


#include "Widget/BVCaptureAnnouncementWidget.h"

#include "Animation/WidgetAnimation.h"
#include "Components/TextBlock.h"

void UBVCaptureAnnouncementWidget::SetMessage(const FText& InMessage)
{
	if (MessageText)
	{
		MessageText->SetText(InMessage);
	}

	if (FadeIn)
	{
		PlayAnimation(FadeIn);
	}
}

void UBVCaptureAnnouncementWidget::PlayFadeOut()
{
	if (FadeOut)
	{
		PlayAnimation(FadeOut);
		// 실제 숨김은 OnAnimationFinished에서 (애니메이션 다 끝난 후)
	}
	else
	{
		// 애니메이션 없으면 즉시 숨김.
		SetVisibility(ESlateVisibility::Hidden);
	}
}

void UBVCaptureAnnouncementWidget::OnAnimationFinished_Implementation(const UWidgetAnimation* Animation)
{
	Super::OnAnimationFinished_Implementation(Animation);

	if (Animation == FadeOut)
	{
		SetVisibility(ESlateVisibility::Hidden);
	}
}
