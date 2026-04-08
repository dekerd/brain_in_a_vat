// Fill out your copyright notice in the Description page of Project Settings.


#include "GAS/CombatAttributeSet.h"

UCombatAttributeSet::UCombatAttributeSet()
{

	InitMaxHealth(100.f);
	InitHealth(100.f);
	InitDamage(25.f);
	InitDefense(0.f);
	InitMaxMana(50.f);
	InitMana(50.f);
	InitHealthRegen(1.0f);
	InitManaRegen(2.0f);
	InitAttackSpeed(1.0f);
	InitAttackRange(20.0f);
	InitMovementSpeed(150.0f);
	
}

void UCombatAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	if (Attribute == GetHealthAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxHealth());
	}
	else if (Attribute == GetManaAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxMana());
	}
}


