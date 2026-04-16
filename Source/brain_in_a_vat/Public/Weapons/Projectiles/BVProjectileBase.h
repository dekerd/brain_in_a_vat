// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BVProjectileBase.generated.h"

class UBVProjectileData;

UCLASS(Blueprintable, BlueprintType)
class BRAIN_IN_A_VAT_API ABVProjectileBase : public AActor
{
	GENERATED_BODY()
	
// Basic Setting
public:

	ABVProjectileBase();

	void BeginPlay() override;
	void Tick(float DeltaTime) override;
	virtual void OnConstruction(const FTransform& Transform) override;
	
	void InitVelocity(const FVector& FireDir);

	void SetLaunchVelocity(const FVector& LaunchVelocity);

	// 스폰 후 즉시 데미지를 덮어쓸 때 사용 (유닛의 Damage 스탯으로 덮어쓰기 위함)
	void SetDamageAmount(float NewDamage) { DamageAmount = NewDamage; }

	// 최대 이동거리 설정. 이 거리 이상 날아가면 자동 폭발.
	void SetMaxTravelDistance(float InDistance) { MaxTravelDistance = InDistance; }

	// DA에서 메시/사운드/VFX/궤적 등을 읽어 자신을 구성한다.
	// BeginPlay 시작 시 자동 호출되므로, BP에서 ProjectileData만 할당하면 끝.
	void ApplyDataAsset();

	// 투사체 설정 DA. BP 디폴트에서 할당하면 스폰 시 자동 적용됨.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Data")
	TObjectPtr<UBVProjectileData> ProjectileData;

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

	UFUNCTION()
	void OnCollisionHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	// 최대 이동거리 도달 시 폭발 처리
	void Explode();

	// 스폰 위치 (거리 계산용)
	FVector SpawnOrigin = FVector::ZeroVector;

	// 0 이하면 거리 제한 없음 (Lifespan으로만 소멸)
	float MaxTravelDistance = 0.f;
};
