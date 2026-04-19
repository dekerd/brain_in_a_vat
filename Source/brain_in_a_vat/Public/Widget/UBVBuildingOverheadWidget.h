// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UBVBuildingOverheadWidget.generated.h"

class UBVHealthComponent;
class UProgressBar;

/**
 * 건물 오버헤드 위젯 — 체력바만 표시.
 */
UCLASS()
class BRAIN_IN_A_VAT_API UUBVBuildingOverheadWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	UPROPERTY(meta = (BindWidget))
	UProgressBar* HealthBar;

	// 체력바의 월드 두께(월드유닛). WBP에서 바로 수정 가능.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Overhead", meta = (ClampMin = "1.0"))
	float WorldThickness = 36.f;

	void InitWithHealthComponent(UBVHealthComponent* InHealthComponent);

protected:

	UFUNCTION()
	void HandleHealthChanged(float NewRatio);
};
