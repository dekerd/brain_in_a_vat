// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "BVUnitDetailWidget.generated.h"

class ABVAutobotBase;
class UImage;
class UTextBlock;
class UProgressBar;
class UBVHealthComponent;

/**
 * 유닛 상세 패널.
 * 클릭된 유닛의 아이콘 / 이름 / 체력 / 체력바 / 공격력 / 방어력 / 공격속도 /
 * 공격사거리 / 시야거리를 표시한다.
 *
 * UMG 바인드 이름 (BindWidget):
 *  - UnitIconImage        (UImage)
 *  - UnitNameText         (UTextBlock)
 *  - HealthText           (UTextBlock)
 *  - HealthBar            (UProgressBar)
 *  - DamageText           (UTextBlock)
 *  - DefenseText          (UTextBlock)
 *  - AttackSpeedText      (UTextBlock)
 *  - AttackRangeText      (UTextBlock)
 *  - VisionRangeText      (UTextBlock)
 */
UCLASS()
class BRAIN_IN_A_VAT_API UBVUnitDetailWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(meta = (BindWidget))
	UImage* UnitIconImage;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* UnitNameText;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* HealthText;

	UPROPERTY(meta = (BindWidget))
	UProgressBar* HealthBar;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* DamageText;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* DefenseText;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* AttackSpeedText;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* AttackRangeText;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* VisionRangeText;

public:
	// 유닛 데이터로 UI 초기화 + HealthComponent에 바인딩
	UFUNCTION(BlueprintCallable, Category = "Unit Detail")
	void SetFromUnit(ABVAutobotBase* InUnit);

	// 매 프레임 최신 스탯으로 갱신 (PlayerTick에서 호출)
	UFUNCTION(BlueprintCallable, Category = "Unit Detail")
	void RefreshLiveStats();

protected:
	UFUNCTION()
	void HandleHealthChanged(float NewRatio);

	void UpdateHealthTextFromComponent();

	TWeakObjectPtr<ABVAutobotBase> BoundUnit;
	TWeakObjectPtr<UBVHealthComponent> BoundHealth;
};
