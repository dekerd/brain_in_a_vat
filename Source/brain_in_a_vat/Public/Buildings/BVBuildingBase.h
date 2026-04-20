// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystemComponent.h"
#include "GenericTeamAgentInterface.h"
#include "Interface/BVDamageableInterface.h"
#include "Data/BuildingStats.h"
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
	UPROPERTY(BlueprintReadOnly, Category="Data", meta = (HideInDetailPanel))
	EBVTeam TeamType = EBVTeam::Neutral;
	
	virtual FGenericTeamId GetGenericTeamId() const override { return FGenericTeamId((uint8)TeamType); }
	virtual void SetGenericTeamId(const FGenericTeamId& NewTeamID) override { TeamType = (EBVTeam)NewTeamID.GetId(); }

// Capsule Offset

protected:

	virtual void OnConstruction(const FTransform& Transform) override;

	// 에디터에서 배치된 인스턴스의 메시/박스 피벗을 바닥으로 강제 보정.
	// Details 패널에서 "Fix Pivot To Bottom" 버튼으로 호출 가능.
	UFUNCTION(CallInEditor, Category = "Construction")
	void FixPivotToBottom();
	
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

	class UBVHealthComponent* GetHealthComponent() const { return HealthComponent; }

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<class USceneComponent> SceneRootComponent;
	
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<class UBoxComponent> BoxComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<class UStaticMeshComponent> StaticMeshComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<class UBVHealthComponent> HealthComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<class UAIPerceptionStimuliSourceComponent> StimuliSourceComponent;

// Building Stat Data
public:
	const FBuildingStats* GetBuildingStats() const;
	void ApplyInitStatFromDataTable();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Data")
	TObjectPtr<class UBVBuildingData> BuildingData;

	UPROPERTY(BlueprintReadOnly, Category="Data", meta = (HideInDetailPanel))
	FText BuildingName = FText::FromString(TEXT("Default Building Name"));

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS")
	TSubclassOf<class UGameplayEffect> InitStatsEffect;

// Construction
public:
	UPROPERTY(BlueprintReadOnly, Category = "Construction", meta = (HideInDetailPanel))
	int32 ConstructionCost = 100;

	UPROPERTY(BlueprintReadOnly, Category = "Construction", meta = (HideInDetailPanel))
	float ConstructionTime = 10.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Construction", meta = (HideInDetailPanel))
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

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<class UUserWidget> OverheadWidgetClass;

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
