// Fill out your copyright notice in the Description page of Project Settings.


#include "Widget/BVUnitOverheadWidget.h"

#include "GenericTeamAgentInterface.h"
#include "Components/BVHealthComponent.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Kismet/GameplayStatics.h"

void UBVUnitOverheadWidget::SetUnitName(FText NewName)
{
	if (UnitNameText) UnitNameText->SetText(NewName);
}

void UBVUnitOverheadWidget::InitWithHealthComponent(class UBVHealthComponent* InHealthComponent)
{
	if (!InHealthComponent || !HealthBar) return;

	InHealthComponent->OnHealthChangedUI.AddDynamic(this, &UBVUnitOverheadWidget::HandleHealthChanged);
	HealthBar->SetPercent(InHealthComponent->GetHealthRatio());

	APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	AActor* TargetActor = InHealthComponent->GetOwner();

	if (PC && TargetActor)
	{
		if (IGenericTeamAgentInterface* PlayerAgent = Cast<IGenericTeamAgentInterface>(PC))
		{
			ETeamAttitude::Type Attitude = PlayerAgent->GetTeamAttitudeTowards(*TargetActor);
			
			if (Attitude == ETeamAttitude::Friendly)
			{
				HealthBar->SetFillColorAndOpacity(FLinearColor::Green);
				this->SetVisibility(ESlateVisibility::HitTestInvisible);
			}
			else if (Attitude == ETeamAttitude::Hostile)
			{
				HealthBar->SetFillColorAndOpacity(FLinearColor::Red);
				this->SetVisibility(ESlateVisibility::HitTestInvisible);
			}
			else
			{
				this->SetVisibility(ESlateVisibility::Hidden);
			}
		}
	}
}

void UBVUnitOverheadWidget::HandleHealthChanged(float NewRatio)
{
	if (HealthBar) HealthBar->SetPercent(NewRatio);
}
