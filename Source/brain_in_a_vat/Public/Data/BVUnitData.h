#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Headers/BVTeam.h"
#include "BVUnitData.generated.h"

class UBVProjectileData;

UCLASS(BlueprintType)
class BRAIN_IN_A_VAT_API UBVUnitData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Info")
	FName UnitName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Info")
	TObjectPtr<UTexture2D> UnitIcon;

	// 무기 데이터 (투사체 DA). WeaponName / WeaponIcon은 여기서 읽어온다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Info")
	TObjectPtr<UBVProjectileData> WeaponData;

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

	// 공격 몽타주 재생속도 배수. 기본 1.0 = 발사 주기(1/AttackSpeed초)에 정확히 맞춤.
	// 2.0 = 2배속(몽타주 빨리 끝나고 남는 시간에 전진), 0.5 = 절반속도.
	// 0 이하면 1.0 으로 간주.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Info", meta = (ClampMin = "0.01"))
	float AttackMontagePlayRateMultiplier = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Info")
	TObjectPtr<UAnimMontage> DeathMontage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spec")
	float MaxHealth = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spec")
	float Damage = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spec")
	float Defense = 1.0f;

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

	// 투사체가 스폰될 스켈레탈 메시 소켓 이름. None이면 액터 위치 + 전방 오프셋에서 스폰.
	// (원거리 여부는 WeaponData가 유효한지로 판정한다)
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