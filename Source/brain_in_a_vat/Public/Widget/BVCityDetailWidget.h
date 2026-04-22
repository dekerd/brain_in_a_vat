// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "BVCityDetailWidget.generated.h"

class ABVCityBase;
class UImage;
class UTextBlock;
class UProgressBar;

/**
 * 거점(ABVCityBase) 전용 상세 패널.
 * 건물 상세와 달리 체력 대신 점령 진행도를 표시한다.
 *
 * UMG BindWidget 이름:
 *  - BuildingIconImage      (UImage)
 *  - BuildingNameText       (UTextBlock)
 *  - DescriptionText        (UTextBlock)
 *  - CaptureProgressBar     (UProgressBar)    진행도 |값| + 부호에 따라 색 변경
 *  - OwnerText              (UTextBlock)      * 선택, "중립" / "아군" / "적" 표시
 *  - CaptureProgressText    (UTextBlock)      * 선택, 퍼센트 텍스트
 *  - SpawnUnitIconImage     (UImage)          * 선택
 *  - SpawnUnitNameText      (UTextBlock)      * 선택
 *  - RespawnProgressBar     (UProgressBar)    * 선택
 */
UCLASS()
class BRAIN_IN_A_VAT_API UBVCityDetailWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// --- City info ---
	UPROPERTY(meta = (BindWidget))
	UImage* BuildingIconImage;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* BuildingNameText;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* DescriptionText;

	// --- Capture progress ---
	UPROPERTY(meta = (BindWidget))
	UProgressBar* CaptureProgressBar;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* OwnerText;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* CaptureProgressText;

	// --- Spawn Unit (optional) ---
	UPROPERTY(meta = (BindWidgetOptional))
	UImage* SpawnUnitIconImage;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* SpawnUnitNameText;

	UPROPERTY(meta = (BindWidgetOptional))
	UProgressBar* RespawnProgressBar;

	// --- 색상 파라미터 ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Capture")
	FLinearColor PlayerCaptureColor = FLinearColor(0.1f, 1.f, 0.1f, 1.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Capture")
	FLinearColor EnemyCaptureColor = FLinearColor(1.f, 0.1f, 0.1f, 1.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Capture")
	FLinearColor NeutralColor = FLinearColor(0.5f, 0.5f, 0.5f, 1.f);

public:
	// 거점 데이터로 UI 초기화 (아이콘, 이름, 설명, 점령 진행도, 스폰 유닛 등)
	UFUNCTION(BlueprintCallable, Category = "City Detail")
	void SetFromCity(ABVCityBase* InCity);

	// 리스폰 프로그레스 업데이트 (PlayerController Tick에서 호출)
	UFUNCTION(BlueprintCallable, Category = "City Detail")
	void SetRespawnProgress(float Ratio);

protected:
	UFUNCTION()
	void HandleCaptureProgressChanged(float NewProgress);

	void ApplyCaptureVisuals(float Progress);

	// 스폰 유닛 섹션(아이콘/이름/리스폰바) 표시 여부를 현재 상태 기준으로 갱신.
	// 중립이거나 스폰 유닛 미설정이면 전부 Collapsed.
	void UpdateSpawnUnitVisibility();

	TWeakObjectPtr<ABVCityBase> BoundCity;
};
