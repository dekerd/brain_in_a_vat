// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "UnitStats.generated.h"

USTRUCT(BlueprintType)
struct FUnitStats : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float MaxHealth = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float Damage = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float Defence = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float AttackSpeed = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float AttackRange = 20.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 GoldReward = 10;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float ExpReward = 100.f;
	
};