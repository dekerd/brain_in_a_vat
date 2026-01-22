// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystemComponent.h"
#include "GenericTeamAgentInterface.h"
#include "Interface/BVDamageableInterface.h"
#include "Data/UnitStats.h"
#include "BVBuildingBase.generated.h"

UCLASS()
class BRAIN_IN_A_VAT_API ABVBuildingBase :  public AActor,
											public IGenericTeamAgentInterface,
											public IAbilitySystemInterface,
											public IBVDamageableInterface
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ABVBuildingBase();

	virtual void Tick(float DeltaTime) override;

	// Basic Properties
	virtual void DestroyBuilding();
	bool bIsDestroyed = false;
	

	// Team Info
	virtual FGenericTeamId GetGenericTeamId() const override;

	// GAS Public Getter
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	class UCombatAttributeSet* GetCombatAttributeSet() const { return CombatAttributes; }

	// Interface
	uint32 GetTeamFlag() const { return TeamFlag; }

// Capsule Offset

protected:

	virtual void OnConstruction(const FTransform& Transform) override;
	
	UPROPERTY(EditAnywhere, Category = "Collision")
	float CapsuleRadius = 200.f;
	
	UPROPERTY(EditAnywhere, Category = "Collision")
	float CapsuleHalfHeight = 200.f;

	UPROPERTY(EditAnywhere, Category = "Collision")
	FVector CapsuleOffset = FVector::ZeroVector;
	

// Damageable Interface
public:
	virtual FGenericTeamId GetTeamId_Implementation() const override;
	virtual bool IsDestroyed_Implementation() const override;

protected:

	virtual void BeginPlay() override;
	
// Building Components
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<class UBoxComponent> BoxComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Components")
	TObjectPtr<class UStaticMeshComponent> StaticMeshComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<class UBVHealthComponent> HealthComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<class UAIPerceptionStimuliSourceComponent> StimuliSourceComponent;

// Stats
public:
	UPROPERTY(EditAnywhere, Category = "Data")
	uint8 TeamFlag;

	const FUnitStats* GetStats() const;
	void ApplyInitStatFromDataTable();

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Data")
	UDataTable* StatTable;

	UPROPERTY(EditDefaultsOnly, Category = "Data")
	FName StatRowName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Data")
	FText BuildingName = FText::FromString(TEXT("Default Building Name"));

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS")
	TSubclassOf<class UGameplayEffect> InitStatsEffect;
	
// Gameplay Ability System (GAS)
public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAS")
	TObjectPtr<UAbilitySystemComponent> ASC;

	UPROPERTY()
	TObjectPtr<class UCombatAttributeSet> CombatAttributes;

// Unit Spawning
	UPROPERTY(EditAnywhere, Category = "Spawn Unit")
	TSubclassOf<class ABVAutobotBase> SpawnUnitClass;

	UPROPERTY(EditAnywhere, Category = "Spawn Unit")
	float RespawnInterval = 10.f;
	float ElapsedTime;

	UFUNCTION()
	void SpawnUnit();
	FTimerHandle SpawnTimerHandle;
	
// Widgets
public:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<class UWidgetComponent> RespawnWidgetComponent;

	UPROPERTY()
	TSubclassOf<UUserWidget> RespawnWidgetClass;

	UPROPERTY()
	TObjectPtr<class UBVSpawnCooltimeBar> RespawnWidget;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI")
	UWidgetComponent* HealthBarWidgetComponent;

	UPROPERTY()
	TSubclassOf<UUserWidget> HealthBarWidgetClass;

	UPROPERTY()
	UWidgetComponent* NameWidgetComponent;

	UPROPERTY()
	TSubclassOf<UUserWidget> NameWidgetClass;
	

// Mouse-hovering effect
public:
	UFUNCTION()
	void SetHovered_Implementation(bool bInHovered) override;
	
protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI")
	bool bIsHovered = false;
	
};
