#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "BVBuildingCatalog.generated.h"

class UBVBuildingData;

/**
 * 건설 가능한 건물 목록을 모아놓은 카탈로그.
 * 여러 건설 메뉴/진영에서 공유 가능하도록 별도 DA로 관리.
 */
UCLASS(BlueprintType)
class BRAIN_IN_A_VAT_API UBVBuildingCatalog : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:

	// 카탈로그에 포함된 건물 DA 목록
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Catalog")
	TArray<TObjectPtr<UBVBuildingData>> Buildings;
};
