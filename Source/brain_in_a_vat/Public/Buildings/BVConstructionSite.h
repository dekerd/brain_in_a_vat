// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GenericTeamAgentInterface.h"
#include "GameFramework/Actor.h"
#include "Interface/BVDamageableInterface.h"
#include "BVConstructionSite.generated.h"

class UWidgetComponent;
class UBoxComponent;
class ABVBuildingBase;
class UAIPerceptionStimuliSourceComponent;

UCLASS()
class BRAIN_IN_A_VAT_API ABVConstructionSite : public AActor,
											   public IGenericTeamAgentInterface,
											   public IAbilitySystemInterface,
											   public IBVDamageableInterface
{
	GENERATED_BODY()

// Initialization
public:
	// Sets default values for this actor's properties
	ABVConstructionSite();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

// Components
public:

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> SceneRootComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UBoxComponent> BoxComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* MeshComponent;

	UPROPERTY()
	TArray<UMaterialInstanceDynamic*> RealMeshDMI;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> GhostMeshComponent;

	UPROPERTY(EditDefaultsOnly, Category = "Components")
	TObjectPtr<UMaterialInterface> GhostMaterialBase;

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> GhostMaterialDMI;


	TSubclassOf<ABVBuildingBase> TargetBuildingClass;

	float ConstructionTime = 20.f;

	FGenericTeamId TeamId;

	void InitConstruction(TSubclassOf<ABVBuildingBase> InBuildingClass, FGenericTeamId InTeamId);
	
	virtual void SetGenericTeamId(const FGenericTeamId& NewTeamId) override { TeamId = NewTeamId; }
	virtual FGenericTeamId GetGenericTeamId() const override { return TeamId; }

private:
	float CurrentProgress = 0.0f;
	bool bIsBuilding = false;
	
	void SpawnRealBuilding();

// GAS
public:

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
	TObjectPtr<UAIPerceptionStimuliSourceComponent> StimuliSourceComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAS")
	TObjectPtr<UAbilitySystemComponent> ASC;

	UPROPERTY()
	TObjectPtr<class UCombatAttributeSet> CombatAttributes;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<class UBVHealthComponent> HealthComponent;

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override {return ASC;}
	virtual FGenericTeamId GetTeamId_Implementation() const override { return TeamId; }
	virtual bool IsDestroyed_Implementation() const override { return bIsDestroyed; }

private:
	bool bIsDestroyed = false;
	float MaxHealth = 100.f;

// Construction Progress Widget
protected:
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI")
	TObjectPtr<UWidgetComponent> ProgressWidgetComponent;
	
};
