// Fill out your copyright notice in the Description page of Project Settings.


#include "BVGameModeBase.h"
#include "MainCharacter.h"
#include "BVPlayerController.h"

ABVGameModeBase::ABVGameModeBase()
{
	DefaultPawnClass = AMainCharacter::StaticClass();
	PlayerControllerClass = ABVPlayerController::StaticClass();
}
