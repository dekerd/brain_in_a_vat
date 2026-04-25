// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "BVMainHUDWidget.generated.h"

class UBVCityDetailWidget;

/**
 * 메인 HUD (화면 하단 프레임 + 자식 패널들).
 *
 * UMG BindWidget 이름:
 *  - CityDetailPanel  (UBVCityDetailWidget)  * 선택, 하단 중앙 프레임 안에 박혀있는 거점 상세 패널
 */
UCLASS()
class BRAIN_IN_A_VAT_API UBVMainHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(meta = (BindWidgetOptional))
	UBVCityDetailWidget* CityDetailPanel;
};
