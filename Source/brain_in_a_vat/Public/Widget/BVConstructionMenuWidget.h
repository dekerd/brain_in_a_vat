// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "BVConstructionMenuWidget.generated.h"

class UWrapBox;
class UBVConstructionMenuSlotWidget;
class UDataTable;
class UBVBuildingCatalog;

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

	// 건설 가능한 건물 카탈로그 (DA). 여기에 설정된 건물들이 메뉴 슬롯으로 자동 생성됨.
	UPROPERTY(EditDefaultsOnly, Category = "Data")
	TObjectPtr<UBVBuildingCatalog> BuildingCatalog;

	// Deprecated: 이전 방식의 직접 배열. 카탈로그가 없을 때 fallback으로만 사용.
	UPROPERTY(EditDefaultsOnly, Category = "Data")
	TArray<TObjectPtr<class UBVBuildingData>> AvailableBuildingsData;
};
