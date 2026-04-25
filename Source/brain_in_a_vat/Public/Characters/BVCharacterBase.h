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
	virtual void SetSelected_Implementation(bool bInSelected) override;
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

// Supply (보급) — 매 초 감소, 0 도달 시 Health가 대신 깎임.
// ASC(CombatAttributeSet::Supply/Health)가 없는 캐릭터(주로 Hero)는 타이머가 no-op으로 동작.
protected:
	// 매 초 감소시킬 보급량. 0 이하로 두면 보급 감쇠 완전 off.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Supply", meta = (ClampMin = "0.0"))
	float SupplyDecayPerSecond = 1.0f;

	// Supply가 0에 닿았을 때 매 초 Health에 적용할 데미지. 0 이하로 두면 아사 비활성.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Supply", meta = (ClampMin = "0.0"))
	float StarvationDamagePerSecond = 5.0f;

	FTimerHandle SupplyTickTimerHandle;

	// 1초마다 호출. ASC/CombatAttributeSet이 없으면 skip. 있으면 Supply 감소 후, 0이면 Health 감소.
	void HandleSupplyTick();
};
