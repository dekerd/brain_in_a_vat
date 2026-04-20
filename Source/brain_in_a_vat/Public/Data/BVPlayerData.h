// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "BVPlayerData.generated.h"

/**
 * 플레이어 캐릭터의 기본 스탯 DA.
 * GAS CombatAttributeSet 초기값으로 주입된다.
 */
UCLASS(BlueprintType)
class BRAIN_IN_A_VAT_API UBVPlayerData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	// --- Health / Mana ---
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
	float MaxHealth = 500.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
	float HealthRegen = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
	float MaxMana = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
	float ManaRegen = 0.f;

	// --- Combat ---
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
	float Damage = 20.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
	float Defense = 5.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
	float AttackSpeed = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
	float AttackRange = 1500.f;

	// --- Movement ---
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
	float MovementSpeed = 600.f;

	// --- Vision ---
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
	float VisionRadius = 1500.f;

	// --- UI ---
	// 머리 위 체력바 위젯 (UBVUnitOverheadWidget 또는 그 파생 클래스)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI")
	TSubclassOf<class UUserWidget> OverheadWidgetClass;
};
