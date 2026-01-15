// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "BVItemData.generated.h"

/**
 * 
 */

UENUM(BlueprintType)
enum class EItemType : uint8 {Weapon, Armor, Consumable};


UCLASS()
class BRAIN_IN_A_VAT_API UBVItemData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	FText ItemName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	TObjectPtr<UTexture2D> ItemIcon;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	TObjectPtr<UStaticMesh> PickupMesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	EItemType ItemType;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	TSubclassOf<class ABVProjectileBase> ProjectileClass;

	UPROPERTY(EditAnywhere, Category = "Weapon")
	float FireInterval = 1.0f;

	UPROPERTY(EditAnywhere, Category = "Weapon")
	float DamageAmount = 20.f;

	UPROPERTY(EditAnywhere, Category = "Weapon")
	float FireRange = 100.f;
	
};
