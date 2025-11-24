// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GenericTeamAgentInterface.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystemComponent.h"
#include "GAS/CombatAttributeSet.h"
#include "GameFramework/Character.h"
#include "BVAutobotBase.generated.h"

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

	void Attack();
	
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	
	UPROPERTY()
	bool bHasTarget = false;

	UPROPERTY()
	float AttackSpeed = 1.0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	TObjectPtr<UAnimMontage> AttackMontage;

	// Gameplay Ability System (GAS)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAS")
	TObjectPtr<UAbilitySystemComponent> ASC;

	TObjectPtr<class UCombatAttributeSet> CombatAttributes;
	
	void OnHealthChanged(const FOnAttributeChangeData& Data);
	
	
};
