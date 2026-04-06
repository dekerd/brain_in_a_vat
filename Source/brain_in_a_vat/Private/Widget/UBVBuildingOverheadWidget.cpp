// Fill out your copyright notice in the Description page of Project Settings.


#include "Widget/UBVBuildingOverheadWidget.h"

#include "Components/TextBlock.h"
#include "Components/ProgressBar.h"
#include "Components/BVHealthComponent.h"
#include "GenericTeamAgentInterface.h"
#include "Kismet/GameplayStatics.h"

void UUBVBuildingOverheadWidget::SetBuildingName(FText NewName)
{
	if (BuildingNameText) BuildingNameText->SetText(NewName);
}

void UUBVBuildingOverheadWidget::InitWithHealthComponent(UBVHealthComponent* InHealthComponent)
{

	if (!InHealthComponent || !HealthBar) return;

	InHealthComponent->OnHealthChangedUI.AddDynamic(this, &UUBVBuildingOverheadWidget::HandleHealthChanged);
	HealthBar->SetPercent(InHealthComponent->GetHealthRatio());

	APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	AActor* Owner = InHealthComponent->GetOwner();
	
	if (PC && Owner)
	{
if (IGenericTeamAgentInterface* PlayerAgent = Cast<IGenericTeamAgentInterface>(PC))
		{
			ETeamAttitude::Type Attitude = PlayerAgent->GetTeamAttitudeTowards(*Owner);
			
			if (Attitude == ETeamAttitude::Friendly)
			{
				HealthBar->SetFillColorAndOpacity(FLinearColor::Green);
				this->SetVisibility(ESlateVisibility::HitTestInvisible); // 보이기
			}
			else if (Attitude == ETeamAttitude::Hostile)
			{
				HealthBar->SetFillColorAndOpacity(FLinearColor::Red);
				this->SetVisibility(ESlateVisibility::HitTestInvisible); // 보이기
			}
			else
			{
				// 중립(Neutral)일 경우 위젯 자체를 숨깁니다.
				this->SetVisibility(ESlateVisibility::Hidden);
			}
		}
	}
}

void UUBVBuildingOverheadWidget::SetRespawnProgress(float Percent)
{
	if (RespawnBar) RespawnBar->SetPercent(Percent);
}

void UUBVBuildingOverheadWidget::HandleHealthChanged(float NewRatio)
{
	if (HealthBar) HealthBar->SetPercent(NewRatio);
}
