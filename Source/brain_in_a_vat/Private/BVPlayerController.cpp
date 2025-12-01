// Fill out your copyright notice in the Description page of Project Settings.


#include "BVPlayerController.h"

ABVPlayerController::ABVPlayerController()
{
	bShowMouseCursor = true;
	DefaultMouseCursor = EMouseCursor::Default;
}

void ABVPlayerController::BeginPlay()
{
	Super::BeginPlay();

	// Blue Team by default
	TeamID = FGenericTeamId(1);

	// Use Mouse Cursor
	FInputModeGameAndUI InputMode;
	InputMode.SetHideCursorDuringCapture(false);
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(InputMode);
	
}
