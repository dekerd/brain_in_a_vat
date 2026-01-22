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

// Unit Information
public:
	virtual FGenericTeamId GetGenericTeamId() const override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Data")
	FText UnitName = FText::FromString(TEXT("Default Unit Name"));

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
	
	UFUNCTION()
	virtual void StartFadeOut();

// Battle

public:
	bool bIsDead = false;
	void Attack();
	void Dead();
	void PerformAttackHit(); // Apply Damage when the attack succeeds
	void ApplyDamageToTarget(AActor* TargetActor);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack")
	float AttackSpeed = 1.0;

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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Team")
	uint8 TeamFlag;
	
	UPROPERTY()
	bool bHasTarget = false;


// Animations
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	TObjectPtr<UAnimMontage> AttackMontage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	TObjectPtr<UAnimMontage> DeathMontage;

	UFUNCTION()
	void OnAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted);

// Widgets
public:

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI")
	UWidgetComponent* HealthBarWidgetComponent;

	UPROPERTY()
	TSubclassOf<UUserWidget> HealthBarWidgetClass;

	UPROPERTY()
	UWidgetComponent* UnitNameWidgetComponent;

	UPROPERTY()
	TSubclassOf<UUserWidget> UnitNameWidgetClass;


protected:

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

// Sound Effects
public:

	void PlayFootstepSound();
	void PlayAttackSound();
	
protected:

	UPROPERTY(EditAnywhere, Category="Audio")
	TArray<TObjectPtr<class USoundBase>> FootstepSounds;

	UPROPERTY(EditAnywhere, Category="Audio")
	float FootstepSoundVolume = 1.0f;

	UPROPERTY(EditAnywhere, Category="Audio")
	TArray<TObjectPtr<class USoundBase>> AttackSounds;

	UPROPERTY(EditAnywhere, Category="Audio")
	float AttackSoundVolume = 1.0f;

	UPROPERTY(EditAnywhere, Category="Audio")
	TObjectPtr<class USoundBase> DeathSounds;

	UPROPERTY(EditAnywhere, Category="Audio")
	float DeathSoundVolume = 1.0f;
};
