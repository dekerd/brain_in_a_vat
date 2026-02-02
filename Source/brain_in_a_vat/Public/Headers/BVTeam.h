#pragma once

#include "CoreMinimal.h"
#include "GenericTeamAgentInterface.h"
#include "BVTeam.generated.h"

UENUM(BlueprintType)
enum class EBVTeam : uint8
{
	None = 0 UMETA(DisplayName = "None"),
	Player = 1 UMETA(DisplayName = "Player"),
	Enemy = 2 UMETA(DisplayName = "Enemy"),
	Neutral = 255 UMETA(DisplayName = "Neutral")
};
