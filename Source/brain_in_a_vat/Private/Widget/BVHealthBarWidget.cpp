// Fill out your copyright notice in the Description page of Project Settings.


#include "Widget/BVHealthBarWidget.h"
#include "Components/ProgressBar.h"
#include "Autobots/BVAutobotBase.h"

void UBVHealthBarWidget::InitWithOwner(ABVAutobotBase* InOwner)
{
	OwnerAutobot = InOwner;
	if (!OwnerAutobot) return;

	OwnerAutobot->OnHealthChangedUI.AddDynamic(this, &UBVHealthBarWidget::HandleHealthChanged);
	
	// Initialization
	if (HealthProgressBar)
	{
		const float Health = OwnerAutobot->GetCombatAttributeSet()->GetHealth();
		const float MaxHealth = OwnerAutobot->GetCombatAttributeSet()->GetMaxHealth();
		const float Ratio = (MaxHealth > 0.f) ? (Health / MaxHealth) : 0.f;

		if (OwnerAutobot->TeamFlag == 1)
		{
			HealthProgressBar->SetFillColorAndOpacity(FLinearColor::Green);
		}
		else if (OwnerAutobot->TeamFlag == 2)
		{
			HealthProgressBar->SetFillColorAndOpacity(FLinearColor::Red);
		}
		
		HealthProgressBar->SetPercent(Ratio);
	}
}

void UBVHealthBarWidget::HandleHealthChanged(float NewRatio)
{
	if (HealthProgressBar)
	{
		HealthProgressBar->SetPercent(NewRatio);
	}
}
