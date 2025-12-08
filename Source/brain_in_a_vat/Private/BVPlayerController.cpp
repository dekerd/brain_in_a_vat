// Fill out your copyright notice in the Description page of Project Settings.


#include "BVPlayerController.h"
#include "Autobots/BVAutobotBase.h"
#include "Collision/BVCollision.h"

ABVPlayerController::ABVPlayerController()
{
	bShowMouseCursor = true;
	DefaultMouseCursor = EMouseCursor::Default;

	HoveredUnit = nullptr;
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

void ABVPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);

	FHitResult Hit;
	bool bHit = GetHitResultUnderCursor(ECC_MouseHover, false, Hit);

	ABVAutobotBase* NewHoveredUnit = nullptr;
	if (bHit)
	{
		AActor* HitActor = Hit.GetActor();
		if (HitActor)
		{
			UE_LOG(LogTemp, Warning, TEXT("CursorHit : %s"), *HitActor->GetName());
			NewHoveredUnit = Cast<ABVAutobotBase>(HitActor);
		}
	}

	if (NewHoveredUnit != HoveredUnit)
	{
		if (HoveredUnit) HoveredUnit->SetHovered(false);
	}

	HoveredUnit = NewHoveredUnit;
	if (HoveredUnit) HoveredUnit->SetHovered(true);
}
