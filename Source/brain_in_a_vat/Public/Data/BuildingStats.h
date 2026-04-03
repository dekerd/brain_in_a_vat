#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "BuildingStats.generated.h"

USTRUCT(BlueprintType)
struct FBuildingStats : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FText BuildingName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float MaxHealth = 500.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float Defense = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float Damage = 20.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float AttackSpeed = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float AttackRange = 800.f;
};