// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Buildings/BuildingMenuData.h"
#include "BVConstructionMenuSlotWidget.generated.h"

class ABVBuildingBase;
class ABVCityBase;
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
	void InitSlot(class UBVBuildingData* InData);

	// City 패널에서 호출하는 초기화. 슬롯 클릭 시 ABVPlayerController::EnterCityBuildMode 경로로 라우팅.
	// InOwningCity가 nullptr이면 일반 InitSlot과 동치(Hero 빌드 모드).
	UFUNCTION(BlueprintCallable, Category = "Build")
	void InitSlotForCity(class UBVBuildingData* InData, ABVCityBase* InOwningCity);

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
	TObjectPtr<class UBVBuildingData> SlotData;

	// 이 슬롯이 속한 도시(있으면 city build 경로). 일반 빌드 메뉴에선 비어있음.
	TWeakObjectPtr<ABVCityBase> OwningCity;
};
