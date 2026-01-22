// Fill out your copyright notice in the Description page of Project Settings.


#include "Widget/BVUnitNameWidget.h"
#include "Components/TextBlock.h"

void UBVUnitNameWidget::SetUnitName(FText NewName)
{
	if (UnitNameText)
	{
		UnitNameText->SetText(NewName);
	}
}
