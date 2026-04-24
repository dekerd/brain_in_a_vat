// Fill out your copyright notice in the Description page of Project Settings.


#include "BVGameModeBase.h"
#include "MainCharacter.h"
#include "BVPlayerController.h"
#include "BVPlayerState.h"
#include "Kismet/GameplayStatics.h"

ABVGameModeBase::ABVGameModeBase()
{
	// PlayerState
	PlayerStateClass = ABVPlayerState::StaticClass();
}

void ABVGameModeBase::GameOver(uint8 LosingTeam)
{
	UGameplayStatics::SetGamePaused(GetWorld(), true);
}
