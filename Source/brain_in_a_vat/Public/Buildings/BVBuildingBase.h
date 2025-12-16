// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystemComponent.h"
#include "GenericTeamAgentInterface.h"
#include "BVBuildingBase.generated.h"

UCLASS()
class BRAIN_IN_A_VAT_API ABVBuildingBase : public AActor, public IGenericTeamAgentInterface, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ABVBuildingBase();

	virtual void Tick(float DeltaTime) override;

	// Basic Properties
	void DestroyBuilding();
	bool bIsDestroyed = false;

	// Team Info
	virtual FGenericTeamId GetGenericTeamId() const override;

	// GAS Public Getter
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	class UCombatAttributeSet* GetCombatAttributeSet() const { return CombatAttributes; }

	// Interface
	uint32 GetTeamFlag() const { return TeamFlag; }

protected:

	virtual void BeginPlay() override;
	
	// Building Components
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<class UCapsuleComponent> CapsuleComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh")
	TObjectPtr<class UStaticMeshComponent> StaticMeshComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<class UBVHealthComponent> HealthComponent;

	// Properties
	UPROPERTY()
	uint32 TeamFlag;

	// Gameplay Ability System (GAS)

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAS")
	TObjectPtr<UAbilitySystemComponent> ASC;

	UPROPERTY()
	TObjectPtr<class UCombatAttributeSet> CombatAttributes;

	// Unit Spawning
	UPROPERTY()
	TSubclassOf<class ABVAutobotBase> SpawnUnitClass;

	UPROPERTY(EditAnywhere, Category = "Spawn Unit")
	float RespawnInterval = 10.f;
	float ElapsedTime;

	UFUNCTION()
	void SpawnUnit();
	
	// Respawn Cooltime Widget
	
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<class UWidgetComponent> RespawnWidgetComponent;

	UPROPERTY()
	TSubclassOf<UUserWidget> RespawnWidgetClass;

	UPROPERTY()
	TObjectPtr<class UBVSpawnCooltimeBar> RespawnWidget;

	FTimerHandle SpawnTimerHandle;

	// HealthBar Widget

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI")
	UWidgetComponent* HealthBarWidgetComponent;

	UPROPERTY()
	TSubclassOf<UUserWidget> HealthBarWidgetClass;
	
};
