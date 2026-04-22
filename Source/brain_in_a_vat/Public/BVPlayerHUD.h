// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "BVPlayerHUD.generated.h"

/**
 * 박스 셀렉션(드래그 사각형) 렌더링용 HUD.
 * PlayerController가 보관하는 드래그 상태를 읽어서 DrawHUD에서 사각형을 그린다.
 */
UCLASS()
class BRAIN_IN_A_VAT_API ABVPlayerHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;

	UPROPERTY(EditDefaultsOnly, Category = "BoxSelect")
	FLinearColor BoxFillColor = FLinearColor(0.2f, 0.8f, 0.2f, 0.15f);

	UPROPERTY(EditDefaultsOnly, Category = "BoxSelect")
	FLinearColor BoxBorderColor = FLinearColor(0.3f, 1.0f, 0.3f, 0.9f);

	UPROPERTY(EditDefaultsOnly, Category = "BoxSelect")
	float BoxBorderThickness = 2.0f;
};
