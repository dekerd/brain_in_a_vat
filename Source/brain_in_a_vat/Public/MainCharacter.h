// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GenericTeamAgentInterface.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/Character.h"
#include "Headers/BVTeam.h"
#include "Interface/BVDamageableInterface.h"
#include "MainCharacter.generated.h"

class ABVBuildingBase;
class UBVItemData;
class UBVPlayerData;
class UAbilitySystemComponent;
class UCombatAttributeSet;
class UBVHealthComponent;
class UAIPerceptionStimuliSourceComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInventoryUpdated);

UCLASS()
class BRAIN_IN_A_VAT_API AMainCharacter : public ACharacter,
                                          public IGenericTeamAgentInterface,
                                          public IAbilitySystemInterface,
                                          public IBVDamageableInterface
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


// Team Setting
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Team")
	EBVTeam TeamType = EBVTeam::Player;

	virtual FGenericTeamId GetGenericTeamId() const override { return FGenericTeamId((uint8)TeamType); }
	virtual void SetGenericTeamId(const FGenericTeamId& NewTeamID) override { TeamType = (EBVTeam)NewTeamID.GetId(); }

// Damageable Interface (hover는 no-op)
public:
	virtual FGenericTeamId GetTeamId_Implementation() const override { return GetGenericTeamId(); }
	virtual bool IsDestroyed_Implementation() const override { return bIsDead; }
	virtual void SetHovered_Implementation(bool bInHovered) override {}

// GAS / Stats
public:
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	UFUNCTION(BlueprintPure, Category = "GAS")
	UCombatAttributeSet* GetCombatAttributeSet() const { return CombatAttributes; }

	UFUNCTION(BlueprintPure, Category = "GAS")
	UBVHealthComponent* GetHealthComponent() const { return HealthComponent; }

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
	TObjectPtr<UBVPlayerData> PlayerData;

	void ApplyInitStatFromDataAsset();

	void HandleDeath();

	UPROPERTY(BlueprintReadOnly, Category = "State")
	bool bIsDead = false;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAS")
	TObjectPtr<UAbilitySystemComponent> ASC;

	UPROPERTY()
	TObjectPtr<UCombatAttributeSet> CombatAttributes;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UBVHealthComponent> HealthComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UAIPerceptionStimuliSourceComponent> StimuliSourceComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI")
	TObjectPtr<class UWidgetComponent> OverheadWidgetComponent;
	
// Inventory and Weapons
	
public:

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool EquipItem(class UBVItemData* ItemToEquip);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool UnequipItem(class UBVItemData* ItemToUnequip);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	const TArray<UBVItemData*>& GetEquippedWeapons() const { return EquippedWeapons; }
	
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void EquipWeapon(UBVItemData* NewWeapon) { CurrentWeapon = NewWeapon; }

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool AddItemToInventory(class UBVItemData* ItemData);

	UPROPERTY(BlueprintAssignable, Category = "Inventory")
	FOnInventoryUpdated OnInventoryUpdated;

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	const TArray<UBVItemData*>& GetInventoryItems() const { return InventoryItems; }

	uint8 MAX_EQUIP_ITEM = 8;

protected:

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Inventory")
	TArray<TObjectPtr<UBVItemData>> EquippedWeapons;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Inventory")
	TObjectPtr<UBVItemData> CurrentWeapon;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Inventory")
	TArray<TObjectPtr<UBVItemData>> InventoryItems;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Inventory")
	TArray<float> WeaponCoolTime;
	
// Character Spec
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FogOfWar")
	float VisionRadius = 1500.0f;

// Attack Setting
protected:
	// 시야 범위 감지 구체 (VisionRadius 사용)
	UPROPERTY(EditAnywhere, Category="Combat")
	TObjectPtr<class USphereComponent> VisionSphere;

	float FireInterval = 1.0f;
	float TimeSinceLastShot = 0.f;
	float GlobalFireTimer = 0.0f;
	const float MinFireInterval = 0.2f;

	UPROPERTY()
	TArray<TWeakObjectPtr<class AActor>> EnemiesInRange;

	UFUNCTION()
	void OnVisionRadiusBeginOverlap(
		UPrimitiveComponent* OverlappedComp,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);

	UFUNCTION()
	void OnVisionRadiusEndOverlap(
		UPrimitiveComponent* OverlappedComp,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex);

	void AutoFire(float DeltaSecond);

	class AActor* FindNearestEnemyInRange() const;

	void FireDefaultLaserBeam(float DeltaSecond);

	void FireWeapons(float DeltaSecond, int32 WeaponIndex);

	void FireDefaultMissile(UBVItemData* ItemData, AActor* Target);

// Construct Buildings
public:
	UFUNCTION(BlueprintCallable, Category = "Building")
	void ConstructBuilding(FVector TargetLocation, TSubclassOf<ABVBuildingBase> BuildingClass, float InConstructionTime);

	UPROPERTY(EditDefaultsOnly, Category = "Construction")
	TSubclassOf<class ABVConstructionSite> ConstructionSiteClass;

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
