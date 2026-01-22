// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "BVUnitNameWidget.generated.h"

/**
 * 
 */
UCLASS()
class BRAIN_IN_A_VAT_API UBVUnitNameWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	
	UPROPERTY(meta = (BindWidget))
	class UTextBlock* UnitNameText;

	void SetUnitName(FText NewName);
};
