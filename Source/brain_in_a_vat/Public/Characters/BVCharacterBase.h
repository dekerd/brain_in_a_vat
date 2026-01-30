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
#include "BVCharacterBase.generated.h"

class UWidgetComponent;
class UBVHealthComponent;
class UDataTable;

UCLASS()
class BRAIN_IN_A_VAT_API ABVCharacterBase : public ACharacter, public IGenericTeamAgentInterface, public IBVDamageableInterface
{
	GENERATED_BODY()

// Initialization
public:
	ABVCharacterBase();
protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

// Character and Team Information
public:
	// Team Info
	virtual FGenericTeamId GetGenericTeamId() const override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Team")
	uint8 TeamFlag;

	virtual FGenericTeamId GetTeamId_Implementation() const override;

	// Name
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Data")
	FText UnitName = FText::FromString(TEXT("Default Unit Name"));
protected:

// Widgets
public:
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
	
	
// Sound Effects
public:
	void PlayInteractionSound();
	
protected:
	UPROPERTY(EditAnywhere, Category="Audio")
	TArray<TObjectPtr<class USoundBase>> InteractionSound;

	UPROPERTY(EditAnywhere, Category="Audio")
	float InteractionSoundVolume = 1.0f;

};
