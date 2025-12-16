// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AbilitySystemComponent.h"
#include "BVHealthComponent.generated.h"


class UAbilitySystemComponent;
class UCombatAttributeSet;
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHealthChangedUI, float, NewHealthRatio);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class BRAIN_IN_A_VAT_API UBVHealthComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UBVHealthComponent();

	UPROPERTY(BlueprintAssignable, Category="Health")
	FOnHealthChangedUI OnHealthChangedUI;

	UFUNCTION(BlueprintCallable, Category="Health")
	void InitFromGAS(
		UAbilitySystemComponent* InASC,
		UCombatAttributeSet* InAttributeSet
		);
	
	UFUNCTION(BlueprintCallable, Category="Health")
	float GetHealthRatio();

	UFUNCTION(BlueprintCallable, Category="Health")
	float GetCurrentHealth() const;

	UFUNCTION(BlueprintCallable, Category="Health")
	float GetMaxHealth() const;

	int32 GetOwnerTeamFlag() const;
	
protected:

	UPROPERTY()
	TObjectPtr<class UAbilitySystemComponent> ASC;

	UPROPERTY()
	TObjectPtr<class UCombatAttributeSet> CombatAttributes;

	void OnHealthChanged(const FOnAttributeChangeData& Data);
	
};
