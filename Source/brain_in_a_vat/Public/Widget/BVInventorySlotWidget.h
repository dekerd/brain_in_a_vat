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

	void UpdateSlot(class UBVItemData* ItemData);

protected:

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<class UImage> ItemIconImage;
};
