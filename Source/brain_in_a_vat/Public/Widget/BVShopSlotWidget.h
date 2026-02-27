// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "BVShopSlotWidget.generated.h"

/**
 * 
 */
UCLASS()
class BRAIN_IN_A_VAT_API UBVShopSlotWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
	void UpdateSlot(class UBVItemData* InItemData);

protected:
	virtual void NativeConstruct() override;

	UFUNCTION()
	void OnItemButtonClicked();

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<class UImage> ItemIconImage;
	
	UPROPERTY()
	TObjectPtr<class UBVItemData> ItemData;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<class UButton> ItemButton;

	UPROPERTY(EditDefaultsOnly, Category = "Tooltip")
	TSubclassOf<class UBVShopTooltipWidget> TooltipClass;
};
