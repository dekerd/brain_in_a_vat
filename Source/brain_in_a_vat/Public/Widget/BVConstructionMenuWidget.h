// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "BVConstructionMenuWidget.generated.h"

class UWrapBox;
class UBVConstructionMenuSlotWidget;
class UDataTable;
class UBVBuildingCatalog;
class ABVCityBase;

/**
 *
 */
UCLASS()
class BRAIN_IN_A_VAT_API UBVConstructionMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 메뉴 슬롯을 재구성한다.
	//   - InCity == nullptr → 글로벌 Hero 카탈로그 (BuildingCatalog / AvailableBuildingsData).
	//   - InCity != nullptr → InCity->BuildableBuildings 기반. 슬롯 클릭 시 EnterCityBuildMode 경로로 라우팅.
	// 위젯이 이미 열려 있는 상태에서도 호출 가능 (컨테이너를 먼저 비우고 다시 채움).
	UFUNCTION(BlueprintCallable, Category = "Build")
	void Populate(ABVCityBase* InCity);

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

	// 현재 바인딩된 도시 (City 패널 → B키로 진입한 경우). null이면 글로벌 Hero 모드.
	TWeakObjectPtr<ABVCityBase> BoundCity;
};
