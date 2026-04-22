// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "BVCityCaptureWidget.generated.h"

class ABVCityBase;
class UProgressBar;

/**
 * 거점(ABVCityBase) 전용 오버헤드 위젯.
 *
 * 단일 ProgressBar의 Percent와 Fill Color를 동시에 조작:
 *   CaptureProgress = 0      → Percent 0        (회색 배경만 보임)
 *   CaptureProgress > 0      → Percent = +값,   FillColor = 초록 (Player 점령 진행)
 *   CaptureProgress < 0      → Percent = |값|,  FillColor = 빨강 (Enemy 점령 진행)
 *
 * WBP 구성 가이드:
 *   - UProgressBar "CaptureBar" 하나 배치
 *   - Style → Fill Image → Tint = 흰색(1,1,1,1)
 *     (틴트가 흰색이어야 C++의 SetFillColorAndOpacity가 제대로 적용됨)
 *   - Style → Background Image → Tint = 회색 (빈 상태 = 중립 색)
 *   - BarFillType = LeftToRight (기본)
 */
UCLASS()
class BRAIN_IN_A_VAT_API UBVCityCaptureWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	// 점령 게이지. Percent와 FillColor를 진행도에 따라 동적 조작.
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> CaptureBar;

	// 월드 두께(WBP에서 조정). ABVBuildingBase의 위젯 스케일 계산이 참조.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Overhead", meta = (ClampMin = "1.0"))
	float WorldThickness = 36.f;

	// 아군(Player) 점령 진행 중일 때 fill color.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Overhead|Capture")
	FLinearColor PlayerCaptureColor = FLinearColor(0.1f, 1.f, 0.1f, 1.f);

	// 적(Enemy) 점령 진행 중일 때 fill color.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Overhead|Capture")
	FLinearColor EnemyCaptureColor = FLinearColor(1.f, 0.1f, 0.1f, 1.f);

	// 중립(0) 상태의 fill color. 보통 Percent=0이라 보이지 않지만, 아주 작은 진행도에서도
	// 자연스러운 페이드를 위해 사용.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Overhead|Capture")
	FLinearColor NeutralColor = FLinearColor(0.5f, 0.5f, 0.5f, 1.f);

	// 거점과 바인딩. OnCaptureProgressChanged 델리게이트 구독 후 초기값 반영.
	void InitWithCity(ABVCityBase* InCity);

protected:

	UFUNCTION()
	void HandleCaptureProgressChanged(float NewProgress);

	UPROPERTY()
	TWeakObjectPtr<ABVCityBase> CityRef;
};
