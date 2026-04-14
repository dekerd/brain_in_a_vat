#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Headers/BVTeam.h"
#include "BVUnitData.generated.h"

UCLASS(BlueprintType)
class BRAIN_IN_A_VAT_API UBVUnitData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Info")
	FName UnitName;

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

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spec")
	float VisionRadius = 1200.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reward")
	int32 GoldReward = 10;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reward")
	float ExpReward = 100.f;
};