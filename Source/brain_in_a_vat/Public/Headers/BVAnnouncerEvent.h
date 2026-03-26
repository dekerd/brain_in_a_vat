#pragma once

#include "CoreMinimal.h"
#include "BVAnnouncerEvent.generated.h"

UENUM(BlueprintType)
enum class EBVAnnouncerEvent : uint8
{
	NotEnoughGold UMETA(DisplayName = "Not Enough Gold"),
	ConstructionStarted UMETA(DisplayName = "Construction Started"),
	ConstructionCompleted UMETA(DisplayName = "Construction Completed"),
	BaseUnderAttack UMETA(DisplayName = "Base Under Attack")
};