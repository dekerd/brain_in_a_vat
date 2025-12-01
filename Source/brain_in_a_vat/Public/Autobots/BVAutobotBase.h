// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GenericTeamAgentInterface.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystemComponent.h"
#include "GAS/CombatAttributeSet.h"
#include "GameFramework/Character.h"
#include "BVAutobotBase.generated.h"

class UWidgetComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAttackFinished, AAIController*, AIController);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHealthChangedUI, float, NewHealthRatio);

UCLASS()
class BRAIN_IN_A_VAT_API ABVAutobotBase : public ACharacter, public IGenericTeamAgentInterface, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	ABVAutobotBase();

	// Team Info
	virtual FGenericTeamId GetGenericTeamId() const override;
	
	UPROPERTY()
	uint32 TeamFlag;
	
	// Gameplay Ability System (GAS)
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	UFUNCTION(BlueprintPure, Category = "GAS")
	UCombatAttributeSet* GetCombatAttributeSet() const { return CombatAttributes; }

	// Basic Functionalities
	void Attack();
	void Dead();
	bool bIsDead = false;

	// UI

	UFUNCTION()
	void SetHovered(bool bInHovered);
	
	UFUNCTION()
	virtual void StartFadeOut();

	// Apply Damage when the attack succeeds
	void PerformAttackHit();
	void ApplyDamageToTarget(AActor* TargetActor);

	// Delegates
	UPROPERTY(BlueprintAssignable)
	FOnAttackFinished OnAttackFinished;

	UPROPERTY(BlueprintAssignable, Category = "UI")
	FOnHealthChangedUI OnHealthChangedUI;
	
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	virtual void Tick(float DeltaTime) override;
	
	UPROPERTY()
	bool bHasTarget = false;

	UPROPERTY()
	float AttackSpeed = 1.0;

	// Animations
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	TObjectPtr<UAnimMontage> AttackMontage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	TObjectPtr<UAnimMontage> DeathMontage;

	
	// Gameplay Ability System (GAS)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAS")
	TObjectPtr<UAbilitySystemComponent> ASC;

	UPROPERTY()
	TObjectPtr<class UCombatAttributeSet> CombatAttributes;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS")
	TSubclassOf<class UGameplayEffect> DamageEffect;
	
	void OnHealthChanged(const FOnAttributeChangeData& Data);

	UFUNCTION()
	void OnAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	// Widget

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI")
	UWidgetComponent* HealthBarWidgetComponent;

	UPROPERTY()
	TSubclassOf<UUserWidget> HealthBarWidgetClass;

	// UI

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI")
	bool bIsHovered = false;
	

	// Material

	UPROPERTY()
	TArray<UMaterialInstanceDynamic*> FadeMIDs;

	float FadeElapsed = 0.0f;
	float FadeDuration = 2.0f;
	bool bIsFading = false;

};
