// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GenericTeamAgentInterface.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystemComponent.h"
#include "GAS/CombatAttributeSet.h"
#include "GameFramework/Character.h"
#include "Interface/BVDamageableInterface.h"
#include "Data/UnitStats.h"
#include "BVAutobotBase.generated.h"


class UWidgetComponent;
class UBVHealthComponent;
class UDataTable;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAttackFinished, AAIController*, AIController);
// DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHealthChangedUI, float, NewHealthRatio);

UCLASS()
class BRAIN_IN_A_VAT_API ABVAutobotBase : public ACharacter, public IGenericTeamAgentInterface, public IAbilitySystemInterface, public IBVDamageableInterface
{
	GENERATED_BODY()

public:
	ABVAutobotBase();

// Team Info
public:
	virtual FGenericTeamId GetGenericTeamId() const override;

// Gameplay Ability System (GAS)
public:
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	UFUNCTION(BlueprintPure, Category = "GAS")
	UCombatAttributeSet* GetCombatAttributeSet() const { return CombatAttributes; }
	
protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAS")
	TObjectPtr<UAbilitySystemComponent> ASC;

	UPROPERTY()
	TObjectPtr<class UCombatAttributeSet> CombatAttributes;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS")
	TSubclassOf<class UGameplayEffect> DamageEffect;

// HealthBar Components
public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UBVHealthComponent> HealthComponent;

// Damageable Interface Implementation
public:
	virtual FGenericTeamId GetTeamId_Implementation() const override;
	virtual bool IsDestroyed_Implementation() const override;
	
// Basic Functionalities


// Stats
public:
	const FUnitStats* GetUnitStats() const;

	void ApplyInitStatFromDataTable();

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Data")
	UDataTable* UnitStatTable;

	UPROPERTY(EditDefaultsOnly, Category = "Data")
	FName UnitStatRowName;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS")
	TSubclassOf<class UGameplayEffect> InitStatsEffect;

	
public:
	void Attack();
	void Dead();
	bool bIsDead = false;
	
	UFUNCTION()
	virtual void StartFadeOut();

	// Apply Damage when the attack succeeds
	void PerformAttackHit();
	void ApplyDamageToTarget(AActor* TargetActor);

// Delegates
	UPROPERTY(BlueprintAssignable)
	FOnAttackFinished OnAttackFinished;

	/*
	UPROPERTY(BlueprintAssignable, Category = "UI")
	FOnHealthChangedUI OnHealthChangedUI;
	*/
	
// Interface
	uint32 GetTeamFlag() const { return TeamFlag; }
	
protected:
	virtual void BeginPlay() override;

	virtual void Tick(float DeltaTime) override;

// Properties

	UPROPERTY()
	uint32 TeamFlag;
	
	UPROPERTY()
	bool bHasTarget = false;

	UPROPERTY()
	float AttackSpeed = 1.0;

// Animations
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	TObjectPtr<UAnimMontage> AttackMontage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	TObjectPtr<UAnimMontage> DeathMontage;

	UFUNCTION()
	void OnAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted);

// HealthBar Widget

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI")
	UWidgetComponent* HealthBarWidgetComponent;

	UPROPERTY()
	TSubclassOf<UUserWidget> HealthBarWidgetClass;

	// void OnHealthChanged(const FOnAttributeChangeData& Data);

// Mouse-hovering effect
public:
	virtual void SetHovered_Implementation(bool bInHovered) override;
	
protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI")
	bool bIsHovered = false;
	
// Fade effect when destroyed

	UPROPERTY()
	TArray<UMaterialInstanceDynamic*> FadeMIDs;

	float FadeElapsed = 0.0f;
	float FadeDuration = 2.0f;
	bool bIsFading = false;

};
