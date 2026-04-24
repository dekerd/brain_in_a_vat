// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "GenericTeamAgentInterface.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystemComponent.h"
#include "GAS/CombatAttributeSet.h"
#include "Interface/BVDamageableInterface.h"
#include "Data/UnitStats.h"
#include "Headers/BVTeam.h"
#include "BVCharacterBase.generated.h"

class UWidgetComponent;
class UBVHealthComponent;
class UDataTable;
class UBVStaticHoverRingComponent;

UCLASS()
class BRAIN_IN_A_VAT_API ABVCharacterBase : public ACharacter,
											public IGenericTeamAgentInterface,
											public IBVDamageableInterface
{
	GENERATED_BODY()

// Initialization
public:
	ABVCharacterBase();
protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

// Team Setting
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Team")
	EBVTeam TeamType = EBVTeam::Neutral;
	
	virtual FGenericTeamId GetGenericTeamId() const override { return FGenericTeamId((uint8)TeamType); }
	virtual void SetGenericTeamId(const FGenericTeamId& NewTeamID) override { TeamType = (EBVTeam)NewTeamID.GetId(); }
	
	virtual FGenericTeamId GetTeamId_Implementation() const override;

// Name
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Data")
	FText UnitName = FText::FromString(TEXT("Default Unit Name"));
protected:

// Mouse-hovering effect (지상 링으로 표시) — UBVStaticHoverRingComponent에 위임.
public:
	virtual void SetHovered_Implementation(bool bInHovered) override;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI|Hover")
	TObjectPtr<UBVStaticHoverRingComponent> StaticHoverRingComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI")
	bool bIsHovered = false;


// Sound Effects
public:
	void PlayInteractionSound();
	
protected:
	UPROPERTY(EditAnywhere, Category="Audio")
	TArray<TObjectPtr<class USoundBase>> InteractionSound;

	UPROPERTY(EditAnywhere, Category="Audio")
	float InteractionSoundVolume = 1.0f;

};
