// Fill out your copyright notice in the Description page of Project Settings.


#include "Widget/BVSpawnCooltimeBar.h"
#include "Components/ProgressBar.h"

void UBVSpawnCooltimeBar::SetProgress(float InPercent)
{
	if (ProgressBar)
	{
		ProgressBar->SetPercent(FMath::Clamp(InPercent, 0.0f, 1.0f));
	}
}
