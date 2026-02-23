// Fill out your copyright notice in the Description page of Project Settings.


#include "Widget/BVConstructionWidget.h"

#include "Components/ProgressBar.h"

void UBVConstructionWidget::SetProgress(float InPercent)
{
	if (ConstructionProgressBar)
	{
		ConstructionProgressBar->SetPercent(InPercent);
	}
}

void UBVConstructionWidget::SetHealth(float InPercent)
{
	if (HealthBar)
	{
		HealthBar->SetPercent(InPercent);
	}
}
