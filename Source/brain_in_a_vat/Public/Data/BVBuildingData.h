#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Headers/BVTeam.h"
#include "BVBuildingData.generated.h"

class ABVBuildingBase;
class ABVAutobotBase;
class ABVCityBase;
class UUserWidget;

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

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Construction", meta = (MultiLine = true))
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Construction")
	TObjectPtr<UStaticMesh> BuildingMesh;

	// 건물 메시의 균일 스케일. 1.0 = 원본 크기.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Construction", meta = (ClampMin = "0.01"))
	float BuildingScale = 1.f;

	// 건물 메시의 Z축(Yaw) 회전 오프셋. 메시가 잘못된 방향을 보고 있을 때 보정용.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Construction")
	float BuildingYaw = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Construction")
	float ConstructionTime = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Construction")
	int32 ConstructionCost = 100;

	// 이 건물이 속해 있는 거점(월드에 배치된 ABVCityBase 인스턴스).
	// 에디터 드롭다운에서 현재 오픈된 레벨의 City 액터를 고를 수 있다.
	// 비워두면 특정 거점 소속 없음 — 런타임 코드가 직접 판단.
	// 주의: TSoftObjectPtr는 맵 종속적 — 다른 맵의 도시를 가리키면 그 맵이 로드되기 전까지 null.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Construction")
	TSoftObjectPtr<ABVCityBase> OwningCity;

	// Base Stats (방어/체력은 공격 능력과 무관하게 항상 필요)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
	float MaxHealth = 500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
	float Defense = 5.0f;

	// ─── Attack ───────────────────────────────────────────────
	// 이 건물이 능동적으로 공격하는지 여부. 체크하면 아래 공격 관련 필드 노출.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack")
	bool bCanAttack = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack",
		meta = (EditCondition = "bCanAttack", EditConditionHides))
	float Damage = 20.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack",
		meta = (EditCondition = "bCanAttack", EditConditionHides))
	float AttackSpeed = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack",
		meta = (EditCondition = "bCanAttack", EditConditionHides))
	float AttackRange = 800.0f;

	// ─── Spawn Unit ───────────────────────────────────────────
	// 이 건물이 유닛을 주기적으로 스폰하는지 여부. 체크하면 아래 스폰 관련 필드 노출.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawn Unit")
	bool bCanSpawnUnits = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawn Unit",
		meta = (EditCondition = "bCanSpawnUnits", EditConditionHides))
	TSubclassOf<ABVAutobotBase> SpawnUnitClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawn Unit",
		meta = (EditCondition = "bCanSpawnUnits", EditConditionHides, ClampMin = "0.1"))
	float SpawnInterval = 10.0f;

	// UI
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI")
	TSubclassOf<UUserWidget> OverheadWidgetClass;
};