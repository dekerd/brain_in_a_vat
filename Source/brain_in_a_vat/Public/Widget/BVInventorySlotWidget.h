// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "BVInventorySlotWidget.generated.h"

/**
 * 
 */
UCLASS()
class BRAIN_IN_A_VAT_API UBVInventorySlotWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	void UpdateSlot(class UBVItemData* InItemData, bool bInIsEquipped);

protected:

	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

	UPROPERTY()
	bool bIsEquippedSlot = false;

	UPROPERTY()
	TObjectPtr<class UBVItemData> ItemData;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<class UImage> ItemIconImage;
};
