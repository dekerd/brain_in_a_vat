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
class ABVCityBase;
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
	float GetDefense() const { return CombatAttributes ? CombatAttributes->GetDefense() : 0.0f; }

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

	// 원거리 공격 시작: 몽타주만 재생하고, 실제 스폰은 PerformAttackHit 노티파이에서 수행.
	void FireProjectile(AActor* TargetActor);

	// 실제 투사체 스폰 로직. PerformAttackHit 노티파이 시점에 호출됨.
	void SpawnProjectileAtTarget(AActor* TargetActor);

protected:
	// FireProjectile 호출 시점에 기억해뒀다가, 노티파이 타이밍에 실제로 스폰할 타겟.
	TWeakObjectPtr<AActor> PendingFireTarget;

	// 몽타주에 PerformAttackHit 노티파이가 없을 때를 대비한 안전 타이머.
	// FireProjectile 호출 후 이 시간 안에 노티파이가 안 오면 자동으로 스폰한다.
	FTimerHandle FireFallbackTimerHandle;

	void FireFallbackSpawn();

public:

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

	// ─── Rally Destination (도시 게리슨용) ──────────────────────
	// City에서 스폰된 유닛이 모일 도시 내 한 지점 (월드 좌표).
	// AssignedLane이 없을 때 AI가 BB의 TargetLocation으로 사용.
	// 도시가 RallyPoint를 옮기면 SetRallyDestination을 다시 호출해 재이동시킬 수 있음.
	UPROPERTY(BlueprintReadOnly, Category = "Garrison", meta = (ExposeOnSpawn = "true"))
	bool bHasRallyDestination = false;

	UPROPERTY(BlueprintReadOnly, Category = "Garrison", meta = (ExposeOnSpawn = "true"))
	FVector RallyDestination = FVector::ZeroVector;

	// 게리슨 재집결 갱신용. BB의 TargetLocation을 즉시 새 지점으로 덮어쓰고 MoveTo 재발행.
	UFUNCTION(BlueprintCallable, Category = "Garrison")
	void SetRallyDestination(const FVector& NewRallyPoint);

	// ─── Dispatch (도시→도시 출격) ────────────────────────────
	// City가 Dispatch()로 출격시킬 때 채워주는 목적지 도시.
	// 도착 시 해당 도시 게리슨에 자기 자신을 등록하는 데 사용된다.
	// 출격 중이 아닐 때는 nullptr.
	UPROPERTY(BlueprintReadOnly, Category = "Garrison", meta = (HideInDetailPanel))
	TWeakObjectPtr<ABVCityBase> DispatchedToCity;

protected:
	// 도착 폴링 누적 타이머 (매 프레임이 아니라 일정 간격으로만 체크해 부하를 줄임).
	float DispatchArrivalCheckTimer = 0.f;

	// 도착 폴링 간격 (초). 이 간격마다 DispatchedToCity 도착 판정 1회.
	UPROPERTY(EditDefaultsOnly, Category = "Garrison", meta = (ClampMin = "0.05"))
	float DispatchArrivalCheckInterval = 0.25f;

	// DispatchedToCity가 유효하면 도착 판정 + 합류 시도. Tick에서 폴링 간격으로 호출.
	void TickDispatchArrival(float DeltaTime);

public:
	
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
