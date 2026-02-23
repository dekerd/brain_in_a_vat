// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BVBuildingBase.h"
#include "BVDefenseTower.generated.h"

UCLASS()
class BRAIN_IN_A_VAT_API ABVDefenseTower : public ABVBuildingBase
{
	GENERATED_BODY()

// Initialization
public:
	ABVDefenseTower();
	virtual void Tick(float DeltaTime) override;

protected:
	virtual void BeginPlay() override;
	
// GAS
public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS")
	TSubclassOf<class UGameplayEffect> DamageEffect;

// Building Defense System (Attack)
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	bool bCanAttack = false;
	
	UPROPERTY(VisibleAnywhere, Category = "Combat")
	TObjectPtr<USphereComponent> AttackRangeSphere;
	
	UPROPERTY(EditAnywhere, Category = "Combat")
	float AttackDamage = 20.f;
	
	UPROPERTY(EditAnywhere, Category = "Combat")
	float FireInterval = 1.0f;

	UPROPERTY(EditAnywhere, Category = "Combat")
	float AttackRange = 800.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	FName FireSocketName = FName("LaserBeamStart");

protected:
	float TimeSinceLastShot = 0.f;

	UPROPERTY()
	TArray<TWeakObjectPtr<AActor>> EnemiesInRange;

	UFUNCTION()
	void OnAttackRangeBeginOverlap(UPrimitiveComponent* OverlappedComp,
								   AActor* OtherActor,
								   UPrimitiveComponent* OtherComp,
								   int32 OtherBodyIndex,
								   bool bFromSweep,
								   const FHitResult& SweepResult);

	UFUNCTION()
	void OnAttackRangeEndOverlap(UPrimitiveComponent* OverlappedComp,
								  AActor* OtherActor,
								  UPrimitiveComponent* OtherComp,
								  int32 OtherBodyIndex);

	void FireLaser(float DeltaTime);
	AActor* FindNearestEnemy();
	void ApplyDamage(AActor* Target);
	
};
