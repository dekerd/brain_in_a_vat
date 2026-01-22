// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "BVNameWidget.generated.h"

class UTextBlock;
/**
 * 
 */
UCLASS()
class BRAIN_IN_A_VAT_API UBVNameWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	UPROPERTY(meta = (BindWidget))
	UTextBlock* BuildingNameText;

	void SetBuildingName(FText NewName);
};
