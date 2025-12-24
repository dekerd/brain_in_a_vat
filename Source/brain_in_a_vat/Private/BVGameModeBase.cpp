// Fill out your copyright notice in the Description page of Project Settings.


#include "BVGameModeBase.h"
#include "MainCharacter.h"
#include "BVPlayerController.h"
#include "Kismet/GameplayStatics.h"

ABVGameModeBase::ABVGameModeBase()
{
	DefaultPawnClass = AMainCharacter::StaticClass();
	PlayerControllerClass = ABVPlayerController::StaticClass();
}

void ABVGameModeBase::GameOver(uint8 LosingTeam)
{
	FString ResultMsg = (LosingTeam == 1) ? TEXT("DEFEAT") : TEXT("VICTORY");

	FString DebugMsg = FString::Printf(TEXT("Game Over : %s"), *ResultMsg);
	GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Red, DebugMsg);

	UGameplayStatics::SetGamePaused(GetWorld(), true);
}
