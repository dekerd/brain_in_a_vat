// Fill out your copyright notice in the Description page of Project Settings.

#include "BVPlayerHUD.h"
#include "BVPlayerController.h"
#include "Engine/Canvas.h"

void ABVPlayerHUD::DrawHUD()
{
	Super::DrawHUD();

	ABVPlayerController* PC = Cast<ABVPlayerController>(PlayerOwner);
	if (!PC || !PC->IsBoxSelectActive()) return;

	const FVector2D Start = PC->GetBoxSelectStart();
	const FVector2D End   = PC->GetBoxSelectCurrent();

	const float X = FMath::Min(Start.X, End.X);
	const float Y = FMath::Min(Start.Y, End.Y);
	const float W = FMath::Abs(End.X - Start.X);
	const float H = FMath::Abs(End.Y - Start.Y);

	if (W < 1.f || H < 1.f) return;

	// 내부 채움 (반투명)
	DrawRect(BoxFillColor, X, Y, W, H);

	// 테두리 4줄
	const float T = BoxBorderThickness;
	DrawRect(BoxBorderColor, X,       Y,       W, T); // top
	DrawRect(BoxBorderColor, X,       Y + H-T, W, T); // bottom
	DrawRect(BoxBorderColor, X,       Y,       T, H); // left
	DrawRect(BoxBorderColor, X + W-T, Y,       T, H); // right
}
