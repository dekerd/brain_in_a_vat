// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "BVInventoryWidget.generated.h"

/**
 * 
 */
UCLASS()
class BRAIN_IN_A_VAT_API UBVInventoryWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintCallable)
	void RefreshInventory();

	UFUNCTION()
	void HandleOnAddItem();

	void NativeConstruct() override;

protected:

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<class UWrapBox> InventoryGrid;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<class UBVInventorySlotWidget> SlotWidgetClass;
	
};
