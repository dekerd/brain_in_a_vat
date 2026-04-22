// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/BVHealthComponent.h"
#include "GAS/CombatAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "Characters/BVAutobotBase.h"
#include "Buildings/BVBuildingBase.h"
#include "MainCharacter.h"


// Sets default values for this component's properties
UBVHealthComponent::UBVHealthComponent()
{

}

void UBVHealthComponent::InitFromGAS(UAbilitySystemComponent* InASC, UCombatAttributeSet* InAttributeSet)
{
	ASC = InASC;
	CombatAttributes = InAttributeSet;

	if (!ASC || !CombatAttributes) return;

	ASC->GetGameplayAttributeValueChangeDelegate(
		CombatAttributes->GetHealthAttribute()
		).AddUObject(this, &UBVHealthComponent::OnHealthChanged);
	
}

float UBVHealthComponent::GetHealthRatio()
{
	const float MaxHealth = GetMaxHealth();
	return (MaxHealth > 0.0f) ? (GetCurrentHealth() / MaxHealth) : 0.0f;
}

float UBVHealthComponent::GetCurrentHealth() const
{
	return CombatAttributes ? CombatAttributes->GetHealth() : 0.f;
}

float UBVHealthComponent::GetMaxHealth() const
{
	return CombatAttributes ? CombatAttributes->GetMaxHealth() : 0.f;
}

void UBVHealthComponent::OnHealthChanged(const FOnAttributeChangeData& Data)
{
	UE_LOG(LogTemp, Warning,
		TEXT("[HP] %s  Old=%.1f  New=%.1f  Max=%.1f"),
		GetOwner() ? *GetOwner()->GetName() : TEXT("?"),
		Data.OldValue, Data.NewValue, GetMaxHealth());

	const float HealthRatio = GetHealthRatio();
	OnHealthChangedUI.Broadcast(HealthRatio);

	if (GetCurrentHealth() <= 0.f)
	{
		UE_LOG(LogTemp, Error, TEXT("[HP] %s DEAD TRIGGERED"),
			GetOwner() ? *GetOwner()->GetName() : TEXT("?"));

		if (ABVAutobotBase* OwnerAutobot = Cast<ABVAutobotBase>(GetOwner()))
		{
			OwnerAutobot->Dead();
		}
		else if (ABVBuildingBase* OwnerBuilding = Cast<ABVBuildingBase>(GetOwner()))
		{
			// 기본 구현은 DestroyBuilding 호출. 거점(ABVCityBase)은 팀 전환으로 오버라이드.
			OwnerBuilding->HandleHealthDepleted();
		}
		else if (AMainCharacter* OwnerPlayer = Cast<AMainCharacter>(GetOwner()))
		{
			OwnerPlayer->HandleDeath();
		}
	}

}

int32 UBVHealthComponent::GetOwnerTeamFlag() const
{
	AActor* Owner = GetOwner();
	if (!Owner) return 0;

	// If the owner has the team information
	if (const IGenericTeamAgentInterface* TeamAgent = Cast<IGenericTeamAgentInterface>(Owner))
	{
		FGenericTeamId TeamId = TeamAgent->GetGenericTeamId();
		return (int32)TeamId.GetId();
	}

	// If the controller has the team information
	if (const APawn* Pawn = Cast<APawn>(Owner))
	{
		if (const AController* Controller = Pawn->GetController())
		{
			if (const IGenericTeamAgentInterface* TeamAgent = Cast<IGenericTeamAgentInterface>(Controller))
			{
				FGenericTeamId TeamId = TeamAgent->GetGenericTeamId();
				return (int32)TeamId.GetId();
			}
		}
	}

	// Return neutral if not found any
	return 0;
}

