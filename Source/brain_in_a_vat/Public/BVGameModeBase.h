// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "BVGameModeBase.generated.h"


/**
 * 
 */
UCLASS()
class BRAIN_IN_A_VAT_API ABVGameModeBase : public AGameModeBase
{
	GENERATED_BODY()

public:

	ABVGameModeBase();

	void GameOver(uint8 LosingTeam);
};
