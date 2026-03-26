// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "BVConstructionMenuWidget.generated.h"

class UWrapBox;
class UBVConstructionMenuSlotWidget;

/**
 * 
 */
UCLASS()
class BRAIN_IN_A_VAT_API UBVConstructionMenuWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWrapBox> BuildingSlotContainer;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UBVConstructionMenuSlotWidget> SlotWidgetClass;
};
