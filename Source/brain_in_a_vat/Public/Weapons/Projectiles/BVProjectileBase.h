// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BVProjectileBase.generated.h"

UCLASS(Blueprintable, BlueprintType)
class BRAIN_IN_A_VAT_API ABVProjectileBase : public AActor
{
	GENERATED_BODY()
	
// Basic Setting
public:

	ABVProjectileBase();

	void BeginPlay() override;
	
	void InitVelocity(const FVector& FireDir);

	void SetLaunchVelocity(const FVector& LaunchVelocity);

protected:

	UPROPERTY(VisibleAnywhere, Category = "Projectile")
	TObjectPtr<class USphereComponent> CollisionComponent;

	UPROPERTY(VisibleAnywhere, Category = "Projectile")
	TObjectPtr<class UStaticMeshComponent> StaticMeshComponent;
	
	UPROPERTY(VisibleAnywhere, Category = "Projectile")
	TObjectPtr<class UProjectileMovementComponent> ProjectileMovement;

	UPROPERTY(EditAnywhere, Category = "Audio")
	TObjectPtr<class USoundBase> FireSound;

	UPROPERTY(EditAnywhere, Category = "Audio")
	TObjectPtr<class USoundBase> HitSound;

	UPROPERTY(EditAnywhere, Category = "Audio")
	float FireSoundVolume = 1.0f;
	
	UPROPERTY(EditAnywhere, Category = "Audio")
	float HitSoundVolume = 1.0f;

	UPROPERTY(EditAnywhere, Category = "VFX")
	TObjectPtr<class UParticleSystem> HitEffect;

// GAS
public:
	
protected:

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
