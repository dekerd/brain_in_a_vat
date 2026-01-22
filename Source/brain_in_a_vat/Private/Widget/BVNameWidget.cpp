// Fill out your copyright notice in the Description page of Project Settings.


#include "Widget/BVNameWidget.h"
#include "Components/TextBlock.h"

void UBVNameWidget::SetBuildingName(FText NewName)
{
	if (BuildingNameText)
	{
		BuildingNameText->SetText(NewName);
	}
}
