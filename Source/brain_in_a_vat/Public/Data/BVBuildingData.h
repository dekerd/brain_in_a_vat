#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Headers/BVTeam.h"
#include "BVBuildingData.generated.h"

class ABVBuildingBase;

UCLASS(BlueprintType)
class BRAIN_IN_A_VAT_API UBVBuildingData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:

	// Team Setting
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Team")
	EBVTeam TeamType = EBVTeam::Neutral;
	
	// Construction & UI
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Construction")
	FText BuildingName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Construction")
	TSubclassOf<ABVBuildingBase> BuildingClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Construction")
	TObjectPtr<UTexture2D> BuildingIcon;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Construction")
	TObjectPtr<UStaticMesh> BuildingMesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Construction")
	float ConstructionTime = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Construction")
	int32 ConstructionCost = 100;

	// Combat Stats
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
	float MaxHealth = 500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
	float Defense = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
	float Damage = 20.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
	float AttackSpeed = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
	float AttackRange = 800.0f;
};