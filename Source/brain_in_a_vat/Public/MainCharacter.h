// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GenericTeamAgentInterface.h"
#include "GameFramework/Character.h"
#include "MainCharacter.generated.h"

class UBVItemData;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInventoryUpdated);

UCLASS()
class BRAIN_IN_A_VAT_API AMainCharacter : public ACharacter, public IGenericTeamAgentInterface
{
	GENERATED_BODY()

public:
	AMainCharacter();
	virtual void Tick(float DeltaTime) override;

protected:

	virtual void BeginPlay() override;

// Mesh and Collision Components
	UPROPERTY(EditAnywhere, Category="Mesh")
	TObjectPtr<class USkeletalMeshComponent> SkeletalMeshComponent;
	
	UPROPERTY(VisibleAnywhere, Category="Camera")
	TObjectPtr<class UCameraComponent> Camera;

	UPROPERTY(VisibleAnywhere, Category="Camera")
	TObjectPtr<class USpringArmComponent> CameraBoom;

// Team Setting
public:
	uint32 GetTeamFlag() const { return TeamFlag; }
protected:
	UPROPERTY()
	uint32 TeamFlag = 1;

	virtual FGenericTeamId GetGenericTeamId() const override;
	
// Inventory and Weapons
	
public:
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void EquipWeapon(UBVItemData* NewWeapon) { CurrentWeapon = NewWeapon; }

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool AddItemToInventory(class UBVItemData* ItemData);

	UPROPERTY(BlueprintAssignable, Category = "Inventory")
	FOnInventoryUpdated OnInventoryUpdated;

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	const TArray<UBVItemData*>& GetInventoryItems() const { return InventoryItems; }

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Inventory")
	TObjectPtr<UBVItemData> CurrentWeapon;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Inventory")
	TArray<TObjectPtr<UBVItemData>> InventoryItems;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Inventory")
	TArray<float> WeaponCoolTime;

// Attack Setting

	UPROPERTY(EditAnywhere, Category="Combat")
	TObjectPtr<class USphereComponent> AttackRangeSphere;

	float FireInterval = 1.0f;
	float AttackRange = 600.f;
	float TimeSinceLastShot = 0.f;

	UPROPERTY()
	TArray<TWeakObjectPtr<class AActor>> EnemiesInRange;

	UFUNCTION()
	void OnAttackRangeBeginOverlap(
		UPrimitiveComponent* OverlappedComp,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);

	UFUNCTION()
	void OnAttackRangeEndOverlap(
		UPrimitiveComponent* OverlappedComp,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex);

	void AutoFire(float DeltaSecond);

	class AActor* FindNearestEnemyInRange() const;

	void FireDefaultLaserBeam(float DeltaSecond);

	void FireWeapons(float DeltaSecond, int32 WeaponIndex);

	void FireDefaultMissile(UBVItemData* ItemData, AActor* Target);

// Sound Effects
public:
	void PlayRandomMoveSound();

	void PlayRandomPickupSound();

	void PlayFootstepSound();
	
protected:

	UPROPERTY(EditAnywhere, Category="Audio")
	TArray<TObjectPtr<class USoundBase>> FootstepSounds;
	
	UPROPERTY(EditAnywhere, Category = "Audio")
	TArray<TObjectPtr<class USoundBase>> MoveSounds;

	UPROPERTY(EditAnywhere, Category = "Audio")
	TArray<TObjectPtr<class USoundBase>> ItemPickupSounds;
	
	UPROPERTY(EditAnywhere, Category = "Audio")
	float MoveSoundCooldown = 5.0f;

	float LastMoveSoundTime = -10.0f;

};
