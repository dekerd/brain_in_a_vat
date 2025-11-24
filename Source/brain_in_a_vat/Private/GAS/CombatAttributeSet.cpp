// Fill out your copyright notice in the Description page of Project Settings.


#include "GAS/CombatAttributeSet.h"

UCombatAttributeSet::UCombatAttributeSet()
{

	InitMaxHealth(100.f);
	InitHealth(100.f);
	InitAttackDamage(10.f);
	InitDefense(0.f);
	
}

void UCombatAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	if (Attribute == GetHealthAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxHealth());
	}
}


