// Fill out your copyright notice in the Description page of Project Settings.


#include "Widget/BVCityCaptureWidget.h"

#include "Buildings/BVCityBase.h"
#include "Components/ProgressBar.h"

void UBVCityCaptureWidget::InitWithCity(ABVCityBase* InCity)
{
	if (!InCity) return;

	CityRef = InCity;

	// 델리게이트 구독 (중복 바인딩 방지를 위해 일단 언바인드 후 재바인드).
	InCity->OnCaptureProgressChanged.RemoveDynamic(this, &UBVCityCaptureWidget::HandleCaptureProgressChanged);
	InCity->OnCaptureProgressChanged.AddDynamic(this, &UBVCityCaptureWidget::HandleCaptureProgressChanged);

	// 현재 진행도로 초기 상태 반영.
	HandleCaptureProgressChanged(InCity->GetCaptureProgress());
}

void UBVCityCaptureWidget::HandleCaptureProgressChanged(float NewProgress)
{
	if (!CaptureBar) return;

	// 진행도 절대값을 Percent로 사용 (0~1).
	const float AbsProgress = FMath::Abs(NewProgress);
	CaptureBar->SetPercent(AbsProgress);

	// 부호에 따라 색상 결정.
	FLinearColor FillColor;
	if (NewProgress > KINDA_SMALL_NUMBER)
	{
		FillColor = PlayerCaptureColor;
	}
	else if (NewProgress < -KINDA_SMALL_NUMBER)
	{
		FillColor = EnemyCaptureColor;
	}
	else
	{
		FillColor = NeutralColor;
	}

	CaptureBar->SetFillColorAndOpacity(FillColor);
}
