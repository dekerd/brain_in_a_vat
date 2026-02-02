// Fill out your copyright notice in the Description page of Project Settings.


#include "Widget/BVUnitOverheadWidget.h"

#include "GenericTeamAgentInterface.h"
#include "Components/BVHealthComponent.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"

void UBVUnitOverheadWidget::SetUnitName(FText NewName)
{
	if (UnitNameText) UnitNameText->SetText(NewName);
}

void UBVUnitOverheadWidget::InitWithHealthComponent(class UBVHealthComponent* InHealthComponent)
{
	if (!InHealthComponent || !HealthBar) return;

	InHealthComponent->OnHealthChangedUI.AddDynamic(this, &UBVUnitOverheadWidget::HandleHealthChanged);
	HealthBar->SetPercent(InHealthComponent->GetHealthRatio());

	APlayerController* PC = GetOwningPlayer();
	AActor* TargetActor = InHealthComponent->GetOwner();

	if (PC && TargetActor)
	{
		if (IGenericTeamAgentInterface* PlayerAgent = Cast<IGenericTeamAgentInterface>(PC))
		{
			ETeamAttitude::Type Attitude = PlayerAgent->GetTeamAttitudeTowards(*TargetActor);
			switch (Attitude)
			{
			case ETeamAttitude::Friendly:
				HealthBar->SetFillColorAndOpacity(FLinearColor::Green);
				break;
			case ETeamAttitude::Hostile:
				HealthBar->SetFillColorAndOpacity(FLinearColor::Red);
				break;
			case ETeamAttitude::Neutral:
				HealthBar->SetFillColorAndOpacity(FLinearColor::Gray);
				break;
			default:
				HealthBar->SetFillColorAndOpacity(FLinearColor::Gray);
				break;
			}
		}
	}
}

void UBVUnitOverheadWidget::HandleHealthChanged(float NewRatio)
{
	if (HealthBar) HealthBar->SetPercent(NewRatio);
}
