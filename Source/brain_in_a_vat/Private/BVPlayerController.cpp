// Fill out your copyright notice in the Description page of Project Settings.


#include "BVPlayerController.h"

void ABVPlayerController::BeginPlay()
{
	Super::BeginPlay();

	// Blue Team by default
	TeamID = FGenericTeamId(1);
	
}
