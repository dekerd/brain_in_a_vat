#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Headers/BVTeam.h"
#include "BVUnitData.generated.h"

class ABVProjectileBase;

UCLASS(BlueprintType)
class BRAIN_IN_A_VAT_API UBVUnitData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Info")
	FName UnitName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Info")
	TObjectPtr<UTexture2D> UnitIcon;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Info")
	EBVTeam TeamType = EBVTeam::Neutral;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Info")
	TSubclassOf<UUserWidget> OverheadWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Info")
	USkeletalMesh* UnitMesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Info")
	TSubclassOf<UAnimInstance> AnimClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Info")
	TObjectPtr<UAnimMontage> AttackMontage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Info")
	TObjectPtr<UAnimMontage> DeathMontage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spec")
	float MaxHealth = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spec")
	float Damage = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spec")
	float Defence = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spec")
	float MaxMana = 50.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spec")
	float HealthRegen = 1.0f; 

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spec")
	float ManaRegen = 2.0f; 

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spec")
	float AttackSpeed = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spec")
	float AttackRange = 20.f;

	// 원거리 공격용 투사체 클래스. 비어 있으면 유닛은 근접 공격을 한다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ranged")
	TSubclassOf<ABVProjectileBase> ProjectileClass;

	// 투사체가 스폰될 스켈레탈 메시 소켓 이름. None이면 액터 위치 + 전방 오프셋에서 스폰.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ranged")
	FName MuzzleSocketName = NAME_None;

	// 소켓이 없을 때 사용되는 스폰 오프셋 (유닛 로컬 좌표계).
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ranged")
	FVector MuzzleFallbackOffset = FVector(50.f, 0.f, 80.f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spec")
	float MovementSpeed = 150.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spec")
	float VisionRadius = 1200.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reward")
	int32 GoldReward = 10;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reward")
	float ExpReward = 100.f;
};