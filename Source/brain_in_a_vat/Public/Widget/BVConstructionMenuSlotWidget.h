// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Buildings/BuildingMenuData.h"
#include "BVConstructionMenuSlotWidget.generated.h"

class ABVBuildingBase;
class UButton;
class UImage;
class UTextBlock;

/**
 * 
 */
UCLASS()
class BRAIN_IN_A_VAT_API UBVConstructionMenuSlotWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Build")
	void InitSlot(const FBuildingMenuData& InMenuData);

protected:
	virtual void NativeConstruct() override;

	UFUNCTION()
	void OnButtonClicked();

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> BuildButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> BuildingIcon;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> BuildingNameText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> CostText;

	UPROPERTY()
	TSubclassOf<ABVBuildingBase> BuildingClass;

	UPROPERTY()
	FBuildingMenuData SlotData;
};
