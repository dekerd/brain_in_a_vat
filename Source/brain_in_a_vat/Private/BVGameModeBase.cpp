// Fill out your copyright notice in the Description page of Project Settings.


#include "BVGameModeBase.h"
#include "MainCharacter.h"
#include "BVPlayerController.h"
#include "Kismet/GameplayStatics.h"

ABVGameModeBase::ABVGameModeBase()
{
	//DefaultPawnClass = AMainCharacter::StaticClass();
	//PlayerControllerClass = ABVPlayerController::StaticClass();
	static ConstructorHelpers::FClassFinder<ABVPlayerController> PlayerControllerClassRef(TEXT("/Script/Engine.Blueprint'/Game/PlayerController/BP_PlayerController.BP_PlayerController_C'"));
	if (PlayerControllerClassRef.Class != nullptr)
	{
		PlayerControllerClass = PlayerControllerClassRef.Class;
	}

	static ConstructorHelpers::FClassFinder<AMainCharacter> MainPlayerClassRef(TEXT("/Script/Engine.Blueprint'/Game/Player/BP_MainCharacter.BP_MainCharacter_C'"));
	if (MainPlayerClassRef.Class != nullptr)
	{
		DefaultPawnClass = MainPlayerClassRef.Class;
	}
	else
	{
		FString DebugMsg = FString::Printf(TEXT("Cannot find mainplay class"));
		GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Red, DebugMsg);
	}
}

void ABVGameModeBase::GameOver(uint8 LosingTeam)
{
	FString ResultMsg = (LosingTeam == 1) ? TEXT("DEFEAT") : TEXT("VICTORY");

	FString DebugMsg = FString::Printf(TEXT("Game Over : %s"), *ResultMsg);
	GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Red, DebugMsg);

	UGameplayStatics::SetGamePaused(GetWorld(), true);
}
