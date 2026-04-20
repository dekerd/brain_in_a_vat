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
	TObjectPtr<UTexture2D> ItemFullImage;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item", meta = (MultiLine = "true"))
	FText ItemDescription;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	int32 ItemPrice = 100;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	TObjectPtr<UStaticMesh> PickupMesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	EItemType ItemType;
	
	// 무기용 투사체 DA. 스폰 시 BVProjectileBase 기본 클래스에 이 데이터를 주입해 구성한다.
	// 사거리/데미지/발사간격 등 무기 스펙은 모두 이 DA 안에서 설정.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	TObjectPtr<class UBVProjectileData> WeaponData;

};
