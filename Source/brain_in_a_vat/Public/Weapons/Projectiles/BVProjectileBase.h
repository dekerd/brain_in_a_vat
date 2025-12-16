// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BVProjectileBase.generated.h"

UCLASS()
class BRAIN_IN_A_VAT_API ABVProjectileBase : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ABVProjectileBase();

	void InitBeamEnd(const FVector& StartLocation, const FVector& EndLocation);

	void InitVelocity(const FVector& FireDir);

protected:

	UPROPERTY(VisibleAnywhere, Category = "Projectile")
	TObjectPtr<class USphereComponent> CollisionComponent;

	UPROPERTY(VisibleAnywhere, Category = "Projectile")
	TObjectPtr<class UNiagaraComponent> NiagaraComponent;

	UPROPERTY(VisibleAnywhere, Category = "Projectile")
	TObjectPtr<class UProjectileMovementComponent> ProjectileMovement;

	// GAS

	UPROPERTY(EditDefaultsOnly, Category = "GAS")
	TSubclassOf<class UGameplayEffect> DamageEffect;

	UPROPERTY(EditDefaultsOnly, Category = "GAS")
	float DamageAmount = 40.f;

	UFUNCTION()
	void OnCollisionBeginOverlap(
		class UPrimitiveComponent* OverlappedComp,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult
		);
	
};
