// Fill out your copyright notice in the Description page of Project Settings.


#include "Widget/BVHealthBarWidget.h"
#include "Components/ProgressBar.h"
#include "Components/BVHealthComponent.h"

void UBVHealthBarWidget::InitWithHealthComponent(UBVHealthComponent* InHealthComponent)
{
	HealthComponent = InHealthComponent;
	if (!HealthComponent || !HealthProgressBar) return;

	HealthComponent->OnHealthChangedUI.AddDynamic(this, &UBVHealthBarWidget::HandleHealthChanged);
	
	// Initialization
	const float Ratio = HealthComponent->GetHealthRatio();
	HealthProgressBar->SetPercent(Ratio);

	// Apply different colors by team info

	if (HealthComponent->GetOwnerTeamFlag() == 1)
	{
		HealthProgressBar->SetFillColorAndOpacity(FLinearColor::Green);
	}
	else if (HealthComponent->GetOwnerTeamFlag() == 2)
	{
		HealthProgressBar->SetFillColorAndOpacity(FLinearColor::Red);
	}

}

void UBVHealthBarWidget::HandleHealthChanged(float NewRatio)
{
	if (HealthProgressBar)
	{
		HealthProgressBar->SetPercent(NewRatio);
	}
}
