// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystemComponent.h"
#include "GenericTeamAgentInterface.h"
#include "Interface/BVDamageableInterface.h"
#include "Data/UnitStats.h"
#include "Headers/BVTeam.h"
#include "BVBuildingBase.generated.h"

class UBoxComponent;
class USphereComponent;
class UWidgetComponent;
class UUBVBuildingOverheadWidget;
class ABVLane;

UCLASS()
class BRAIN_IN_A_VAT_API ABVBuildingBase :  public AActor,
											public IGenericTeamAgentInterface,
											public IAbilitySystemInterface,
											public IBVDamageableInterface
{
	GENERATED_BODY()

// Initialization
public:

	ABVBuildingBase();
	virtual void Tick(float DeltaTime) override;

// Team Setting
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Team")
	EBVTeam TeamType = EBVTeam::Neutral;
	
	virtual FGenericTeamId GetGenericTeamId() const override { return FGenericTeamId((uint8)TeamType); }
	virtual void SetGenericTeamId(const FGenericTeamId& NewTeamID) override { TeamType = (EBVTeam)NewTeamID.GetId(); }

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
public:

	USceneComponent* GetSceneRootComponent() { return SceneRootComponent; }

	UStaticMeshComponent* GetStaticMeshComponent() { return StaticMeshComponent; }

	UBoxComponent* GetBoxComponent() { return BoxComponent; }
	
protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<class USceneComponent> SceneRootComponent;
	
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
	const FUnitStats* GetStats() const;
	void ApplyInitStatFromDataTable();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Data")
	FText BuildingName = FText::FromString(TEXT("Default Building Name"));

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Data")
	UDataTable* StatTable;

	UPROPERTY(EditDefaultsOnly, Category = "Data")
	FName StatRowName;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS")
	TSubclassOf<class UGameplayEffect> InitStatsEffect;

// Construction
public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Construction")
	int32 ConstructionCost = 100;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Construction")
	float ConstructionTime = 10.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Construction")
	UTexture2D* BuildingIcon;

protected:
	
	float CurrentBuildProgress = 0.0f;
	bool bIsConstructing = false;

	UPROPERTY()
	UMaterialInstanceDynamic* ConstructionDMI;

	void FinishConstruction();
	
// Gameplay Ability System (GAS)
public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAS")
	TObjectPtr<UAbilitySystemComponent> ASC;

	UPROPERTY()
	TObjectPtr<class UCombatAttributeSet> CombatAttributes;

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	class UCombatAttributeSet* GetCombatAttributeSet() const { return CombatAttributes; }

// Damage Detecting
protected:

	float PreviousHealthRatio = 1.0f;
	
	UFUNCTION()
	void HandleHealthChangedForAudio(float NewHealthRatio);

// Unit Spawning
public:
	UPROPERTY(EditAnywhere, Category = "Spawn Unit")
	TSubclassOf<class ABVAutobotBase> SpawnUnitClass;

	UPROPERTY(EditAnywhere, Category = "Spawn Unit")
	float RespawnInterval = 10.f;

	// 스폰된 유닛에게 전달할 레인. 설정 시 유닛이 레인 직선으로 합류 후 적 베이스로 이동한다.
	UPROPERTY(EditAnywhere, Category = "Spawn Unit")
	TObjectPtr<ABVLane> AssignedLane;
	float ElapsedTime;

	UFUNCTION()
	void SpawnUnit();
	FTimerHandle SpawnTimerHandle;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn Unit")
	TObjectPtr<AActor> FriendlyMainBase;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn Unit")
	TObjectPtr<AActor> EnemyMainBase;

// Destruction
public:
	virtual void DestroyBuilding();
	bool bIsDestroyed = false;
	
// Widgets
public:

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI")
	TObjectPtr<UWidgetComponent> OverheadWidgetComponent;

	UPROPERTY()
	TObjectPtr<UUBVBuildingOverheadWidget> OverheadWidget;

// Mouse-hovering effect
public:
	UFUNCTION()
	void SetHovered_Implementation(bool bInHovered) override;
	
protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI")
	bool bIsHovered = false;
	
	
};
