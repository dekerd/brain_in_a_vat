// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GenericTeamAgentInterface.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystemComponent.h"
#include "Characters/BVCharacterBase.h"
#include "GAS/CombatAttributeSet.h"
#include "BVAutobotBase.generated.h"

class UWidgetComponent;
class UBVHealthComponent;
class ABVLane;
class UBVUnitData;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAttackFinished, AAIController*, AIController);

UCLASS()
class BRAIN_IN_A_VAT_API ABVAutobotBase : public ABVCharacterBase,
										  public IAbilitySystemInterface
{
	GENERATED_BODY()

// Initialization
public:
	ABVAutobotBase();

	virtual void OnConstruction(const FTransform& Transform) override;

	// 에디터에서 배치된 인스턴스의 메시 피벗을 캡슐 바닥으로 강제 보정.
	UFUNCTION(CallInEditor, Category = "Construction")
	void FixPivotToBottom();

protected:
	virtual void BeginPlay() override;

	virtual void Tick(float DeltaTime) override;

// Unit Spec and Gameplay Ability System (GAS)
public:
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	UFUNCTION(BlueprintPure, Category = "GAS|Attributes")
	UCombatAttributeSet* GetCombatAttributeSet() const { return CombatAttributes; }

	void ApplyInitStatFromDataAsset();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unit Spec")
	UBVUnitData* UnitData;

	UPROPERTY(BlueprintReadOnly, Category = "Unit Spec")
	float VisionRadius;

	UFUNCTION(BlueprintPure, Category = "GAS|Attributes")
	float GetHealth() const { return CombatAttributes ? CombatAttributes->GetHealth() : 0.0f; }

	UFUNCTION(BlueprintPure, Category = "GAS|Attributes")
	float GetMaxHealth() const { return CombatAttributes ? CombatAttributes->GetMaxHealth() : 0.0f; }

	UFUNCTION(BlueprintPure, Category = "GAS|Attributes")
	float GetMana() const { return CombatAttributes ? CombatAttributes->GetMana() : 0.0f; }

	UFUNCTION(BlueprintPure, Category = "GAS|Attributes")
	float GetMaxMana() const { return CombatAttributes ? CombatAttributes->GetMaxMana() : 0.0f; }

	UFUNCTION(BlueprintPure, Category = "GAS|Attributes")
	float GetHealthRegen() const { return CombatAttributes ? CombatAttributes->GetHealthRegen() : 0.0f; }

	UFUNCTION(BlueprintPure, Category = "GAS|Attributes")
	float GetManaRegen() const { return CombatAttributes ? CombatAttributes->GetManaRegen() : 0.0f; }

	UFUNCTION(BlueprintPure, Category = "GAS|Attributes")
	float GetDamage() const { return CombatAttributes ? CombatAttributes->GetDamage() : 0.0f; }

	UFUNCTION(BlueprintPure, Category = "GAS|Attributes")
	float GetDefence() const { return CombatAttributes ? CombatAttributes->GetDefense() : 0.0f; }

	UFUNCTION(BlueprintPure, Category = "GAS|Attributes")
	float GetAttackSpeed() const { return CombatAttributes ? CombatAttributes->GetAttackSpeed() : 0.0f; }

	UFUNCTION(BlueprintPure, Category = "GAS|Attributes")
	float GetAttackRange() const { return CombatAttributes ? CombatAttributes->GetAttackRange() : 0.0f; }

	UFUNCTION(BlueprintPure, Category = "GAS|Attributes")
	float GetMovementSpeed() const { return CombatAttributes ? CombatAttributes->GetMovementSpeed() : 0.0f; }
	
protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAS")
	TObjectPtr<UAbilitySystemComponent> ASC;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS")
	TSubclassOf<class UGameplayEffect> DamageEffect;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS")
	TSubclassOf<class UGameplayEffect> InitStatsEffect;

	UPROPERTY()
	TObjectPtr<class UCombatAttributeSet> CombatAttributes;
	
// Custom Components
public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UBVHealthComponent> HealthComponent;

// Damageable Interface Implementation
public:
	virtual FGenericTeamId GetTeamId_Implementation() const override;
	virtual bool IsDestroyed_Implementation() const override;

	
// Battle
public:
	bool bIsDead = false;
	void Attack();
	void Dead();
	void PerformAttackHit();
	void ApplyDamageToTarget(AActor* TargetActor);

	// 원거리 공격: UnitData->ProjectileClass가 있으면 여기서 스폰. Target을 향해 포물선 발사.
	void FireProjectile(AActor* TargetActor);

protected:
	// Tick에서 누적되는 쿨다운 타이머 (원거리 자동 발사용).
	// MainCharacter의 GlobalFireTimer 패턴과 동일.
	float FireCooldownTimer = 0.f;

	// 원거리 자동 공격 루틴. Tick에서 호출됨.
	void TickRangedAttack(float DeltaTime);

	// BB의 AttackTargetActor를 읽어 반환 (유효하지 않으면 nullptr).
	AActor* GetBBAttackTarget() const;

public:

	UPROPERTY()
	bool bHasTarget = false;
	
	UPROPERTY(BlueprintAssignable)
	FOnAttackFinished OnAttackFinished;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lane", meta = (ExposeOnSpawn = "true"))
	TObjectPtr<ABVLane> AssignedLane;
	
// Animations
public:
	UPROPERTY(BlueprintReadWrite, Category = "Animation")
	TObjectPtr<UAnimMontage> AttackMontage;

	UPROPERTY(BlueprintReadWrite, Category = "Animation")
	TObjectPtr<UAnimMontage> DeathMontage;

	UFUNCTION()
	void OnAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted);

// Widgets
public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI")
	UWidgetComponent* OverheadWidgetComponent;

	UPROPERTY()
	TSubclassOf<UUserWidget> OverheadWidgetClass;

// Fade-out effect when destroyed
public:
	UFUNCTION()
	virtual void StartFadeOut();

protected:
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
