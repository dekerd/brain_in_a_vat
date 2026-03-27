#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "BuildingMenuData.generated.h"

class ABVBuildingBase;
class UTexture2D;
class UStaticMesh;

USTRUCT(BlueprintType)
struct FBuildingMenuData : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSubclassOf<ABVBuildingBase> BuildingClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FText BuildingName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UTexture2D> BuildingIcon;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UStaticMesh> BuildingMesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float ConstructionTime = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 ConstructionCost = 100;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float BuildingHealth = 100.0f;
};