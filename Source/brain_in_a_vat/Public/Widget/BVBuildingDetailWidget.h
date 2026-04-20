// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "BVBuildingDetailWidget.generated.h"

class ABVBuildingBase;
class UImage;
class UTextBlock;
class UProgressBar;
class UBVHealthComponent;

/**
 * 건물 상세 패널.
 * 클릭된 건물의 아이콘 / 이름 / 설명 / 체력 / 체력바 /
 * 생성 중인 유닛 아이콘 + 리스폰 프로그레스 바를 표시한다.
 * 스폰 유닛이 없으면 스폰 관련 위젯들은 숨긴다.
 *
 * UMG 바인드 이름은 아래 BindWidget 멤버명과 정확히 일치해야 한다.
 *  - BuildingIconImage      (UImage)
 *  - BuildingNameText       (UTextBlock)
 *  - DescriptionText        (UTextBlock)
 *  - HealthText             (UTextBlock)        * 선택, OptionalBindWidget
 *  - HealthBar              (UProgressBar)      * 선택, OptionalBindWidget
 *  - SpawnUnitIconImage     (UImage)            * 선택, OptionalBindWidget
 *  - SpawnUnitNameText      (UTextBlock)        * 선택, OptionalBindWidget
 *  - RespawnProgressBar     (UProgressBar)      * 선택, OptionalBindWidget
 */
UCLASS()
class BRAIN_IN_A_VAT_API UBVBuildingDetailWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// --- Building info ---
	UPROPERTY(meta = (BindWidget))
	UImage* BuildingIconImage;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* BuildingNameText;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* DescriptionText;

	// --- Health (optional) ---
	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* HealthText;

	UPROPERTY(meta = (BindWidgetOptional))
	UProgressBar* HealthBar;

	// --- Spawn Unit (optional) ---
	UPROPERTY(meta = (BindWidgetOptional))
	UImage* SpawnUnitIconImage;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* SpawnUnitNameText;

	UPROPERTY(meta = (BindWidgetOptional))
	UProgressBar* RespawnProgressBar;

public:
	// 건물 데이터로 UI 초기화 (아이콘, 이름, 설명, 스폰 유닛 정보)
	UFUNCTION(BlueprintCallable, Category = "Building Detail")
	void SetFromBuilding(ABVBuildingBase* InBuilding);

	// 리스폰 프로그레스 업데이트 (0~1)
	UFUNCTION(BlueprintCallable, Category = "Building Detail")
	void SetRespawnProgress(float Ratio);

protected:
	UFUNCTION()
	void HandleHealthChanged(float NewRatio);

	void UpdateHealthTextFromComponent();

	// 현재 바인딩된 건물 (약한 참조로, destroy 되더라도 안전)
	TWeakObjectPtr<ABVBuildingBase> BoundBuilding;
	TWeakObjectPtr<UBVHealthComponent> BoundHealth;
};
