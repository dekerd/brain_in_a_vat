// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "BVShopWidget.generated.h"

class ABVNPCBase;
class UImage;
class UButton;
/**
 * 
 */
UCLASS()
class BRAIN_IN_A_VAT_API UBVShopWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Shop")
	void InitShop(ABVNPCBase* InNPC);

protected:
	
	// Called when the widget is constructed
	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> CloseButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> NPC_PortraitImage;

	UFUNCTION()
	void OnCloseButtonClicked();


};
