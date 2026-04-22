// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystemComponent.h"
#include "GenericTeamAgentInterface.h"
#include "Interface/BVDamageableInterface.h"
#include "Data/BuildingStats.h"
#include "Headers/BVTeam.h"
#include "BVBuildingBase.generated.h"

class UBoxComponent;
class USphereComponent;
class UWidgetComponent;
class UUBVBuildingOverheadWidget;
class ABVLane;

UCLASS()
class BRAIN_IN_A_VAT_API ABVBuildingBase :  public AActor,
											public IGenericTeamAgentInterface,
											public IAbilitySystemInterface,
											public IBVDamageableInterface
{
	GENERATED_BODY()

// Initialization
public:

	ABVBuildingBase();
	virtual void Tick(float DeltaTime) override;

// Team Setting
public:
	UPROPERTY(BlueprintReadOnly, Category="Data", meta = (HideInDetailPanel))
	EBVTeam TeamType = EBVTeam::Neutral;
	
	virtual FGenericTeamId GetGenericTeamId() const override { return FGenericTeamId((uint8)TeamType); }
	virtual void SetGenericTeamId(const FGenericTeamId& NewTeamID) override { TeamType = (EBVTeam)NewTeamID.GetId(); }

// Capsule Offset

protected:

	virtual void OnConstruction(const FTransform& Transform) override;

	// 에디터에서 배치된 인스턴스의 메시/박스 피벗을 바닥으로 강제 보정.
	// Details 패널에서 "Fix Pivot To Bottom" 버튼으로 호출 가능.
	UFUNCTION(CallInEditor, Category = "Construction")
	void FixPivotToBottom();
	
	UPROPERTY(EditAnywhere, Category = "Collision")
	float CapsuleRadius = 200.f;
	
	UPROPERTY(EditAnywhere, Category = "Collision")
	float CapsuleHalfHeight = 200.f;

	UPROPERTY(EditAnywhere, Category = "Collision")
	FVector CapsuleOffset = FVector::ZeroVector;
	

// Damageable Interface
public:
	virtual FGenericTeamId GetTeamId_Implementation() const override;
	virtual bool IsDestroyed_Implementation() const override;

protected:
	virtual void BeginPlay() override;
	
// Building Components
public:

	USceneComponent* GetSceneRootComponent() { return SceneRootComponent; }

	UStaticMeshComponent* GetStaticMeshComponent() { return StaticMeshComponent; }

	UBoxComponent* GetBoxComponent() { return BoxComponent; }

	class UBVHealthComponent* GetHealthComponent() const { return HealthComponent; }

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<class USceneComponent> SceneRootComponent;
	
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<class UBoxComponent> BoxComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<class UStaticMeshComponent> StaticMeshComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<class UBVHealthComponent> HealthComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<class UAIPerceptionStimuliSourceComponent> StimuliSourceComponent;

// Building Stat Data
public:
	const FBuildingStats* GetBuildingStats() const;
	void ApplyInitStatFromDataTable();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Data")
	TObjectPtr<class UBVBuildingData> BuildingData;

	UPROPERTY(BlueprintReadOnly, Category="Data", meta = (HideInDetailPanel))
	FText BuildingName = FText::FromString(TEXT("Default Building Name"));

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS")
	TSubclassOf<class UGameplayEffect> InitStatsEffect;

// Construction
public:
	UPROPERTY(BlueprintReadOnly, Category = "Construction", meta = (HideInDetailPanel))
	int32 ConstructionCost = 100;

	UPROPERTY(BlueprintReadOnly, Category = "Construction", meta = (HideInDetailPanel))
	float ConstructionTime = 10.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Construction", meta = (HideInDetailPanel))
	UTexture2D* BuildingIcon;

protected:
	
	float CurrentBuildProgress = 0.0f;
	bool bIsConstructing = false;

	UPROPERTY()
	UMaterialInstanceDynamic* ConstructionDMI;

	void FinishConstruction();
	
// Gameplay Ability System (GAS)
public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAS")
	TObjectPtr<UAbilitySystemComponent> ASC;

	UPROPERTY()
	TObjectPtr<class UCombatAttributeSet> CombatAttributes;

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	class UCombatAttributeSet* GetCombatAttributeSet() const { return CombatAttributes; }

// Damage Detecting
public:
	// true면 Player 팀 소유의 이 건물이 데미지를 받을 때 "Base Under Attack" 어나운스 음성을 재생.
	// 거점(City)처럼 일반 기지가 아닌 구조물은 false로 꺼둬서 스트레이 피해에 반응하지 않게 함.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
	bool bPlayBaseUnderAttackAnnouncer = true;

protected:

	float PreviousHealthRatio = 1.0f;

	UFUNCTION()
	void HandleHealthChangedForAudio(float NewHealthRatio);

// Unit Spawning
public:
	UPROPERTY(EditAnywhere, Category = "Spawn Unit")
	TSubclassOf<class ABVAutobotBase> SpawnUnitClass;

	UPROPERTY(EditAnywhere, Category = "Spawn Unit")
	float RespawnInterval = 10.f;

	// 스폰된 유닛에게 전달할 레인. 설정 시 유닛이 레인 직선으로 합류 후 적 베이스로 이동한다.
	UPROPERTY(EditAnywhere, Category = "Spawn Unit")
	TObjectPtr<ABVLane> AssignedLane;
	float ElapsedTime;

	UFUNCTION()
	virtual void SpawnUnit();
	FTimerHandle SpawnTimerHandle;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn Unit")
	TObjectPtr<AActor> FriendlyMainBase;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn Unit")
	TObjectPtr<AActor> EnemyMainBase;

// Destruction
public:
	virtual void DestroyBuilding();
	bool bIsDestroyed = false;

	// HP가 0에 도달했을 때의 처리 훅. 기본 구현은 DestroyBuilding()을 호출.
	// 점령 가능한 거점(ABVCityBase)은 이를 오버라이드해 파괴 대신 팀 전환을 수행한다.
	virtual void HandleHealthDepleted();

	// 투사체 등 데미지 원(原)이 이 건물에 피해를 입힐 때 호출. 마지막 가해자 팀을 기록.
	void RecordDamageFrom(const AActor* Attacker);

	// 데미지를 입었을 때 파생 클래스가 반응할 수 있는 공용 훅.
	// HP 변동 경로와 별개로, 가해자와 데미지량을 함께 받음.
	// 기본 구현은 RecordDamageFrom만 수행. 거점(ABVCityBase)은 오버라이드해 점령 진행도 반영.
	virtual void HandleDamageReceived(const AActor* Attacker, float DamageAmount);

protected:
	// 마지막으로 이 건물에 데미지를 입힌 주체의 팀. 점령 판정에 사용된다.
	EBVTeam LastDamagerTeam = EBVTeam::None;

// Emission Pulse (생산 중일 때 머티리얼이 깜빡이게 하는 공용 훅)
public:
	// "생산 중" 상태를 머티리얼에 전달할 때 사용할 스칼라 파라미터 이름.
	// 머티리얼 쪽에서 같은 이름의 Scalar Parameter를 Time 기반 펄스 게이트로 사용.
	UPROPERTY(EditAnywhere, Category = "Building|Emission")
	FName IsProducingParamName = TEXT("IsProducing");

	// 머티리얼의 Emissive Color를 런타임에 바꿀 때 사용할 벡터 파라미터 이름.
	// 머티리얼 쪽에서 같은 이름의 VectorParameter가 EmissionColor 체인에 꽂혀 있어야 함.
	UPROPERTY(EditAnywhere, Category = "Building|Emission")
	FName EmissionColorParamName = TEXT("EmissionColor");

protected:
	// StaticMesh의 각 머티리얼 슬롯을 MID로 래핑해 런타임 파라미터 조작을 가능하게 함.
	virtual void InitDynamicMaterials();

	// 머티리얼의 IsProducing 스칼라 파라미터를 0/1로 설정해 펄스 on/off.
	virtual void SetIsProducing(bool bIsProducing);

	// 머티리얼의 EmissionColor 벡터 파라미터 설정.
	// HDR(값 > 1) 허용 — bloom으로 번짐 강조하고 싶을 때 활용.
	virtual void SetEmissionColor(const FLinearColor& InColor);

	UPROPERTY()
	TArray<TObjectPtr<UMaterialInstanceDynamic>> DynamicMaterials;
	
// Widgets
public:

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI")
	TObjectPtr<UWidgetComponent> OverheadWidgetComponent;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<class UUserWidget> OverheadWidgetClass;

	UPROPERTY()
	TObjectPtr<UUBVBuildingOverheadWidget> OverheadWidget;

// Mouse-hovering effect
public:
	UFUNCTION()
	void SetHovered_Implementation(bool bInHovered) override;
	
protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI")
	bool bIsHovered = false;
	
	
};
