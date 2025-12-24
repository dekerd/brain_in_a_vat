// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GenericTeamAgentInterface.h"
#include "GameFramework/Character.h"
#include "MainCharacter.generated.h"

UCLASS()
class BRAIN_IN_A_VAT_API AMainCharacter : public ACharacter, public IGenericTeamAgentInterface
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AMainCharacter();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every framed
	virtual void Tick(float DeltaTime) override;

	uint32 GetTeamFlag() const { return TeamFlag; }

protected:
	UPROPERTY(EditAnywhere, Category="Mesh")
	TObjectPtr<class USkeletalMeshComponent> SkeletalMeshComponent;
	
	UPROPERTY(VisibleAnywhere, Category="Camera")
	TObjectPtr<class UCameraComponent> Camera;

	UPROPERTY(VisibleAnywhere, Category="Camera")
	TObjectPtr<class USpringArmComponent> CameraBoom;

	// Team Setting
	UPROPERTY()
	uint32 TeamFlag = 1;

	virtual FGenericTeamId GetGenericTeamId() const override;

	// Weapon & Attack Setting

	UPROPERTY(EditAnywhere, Category="Combat")
	TObjectPtr<class USphereComponent> AttackRangeSphere;

	float FireInterval = 1.0f;
	float AttackRange = 600.f;
	float TimeSinceLastShot = 0.f;

	UPROPERTY()
	TArray<TWeakObjectPtr<class AActor>> EnemiesInRange;

	UFUNCTION()
	void OnAttackRangeBeginOverlap(
		UPrimitiveComponent* OverlappedComp,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);

	UFUNCTION()
	void OnAttackRangeEndOverlap(
		UPrimitiveComponent* OverlappedComp,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex);

	void AutoFire(float DeltaSecond);

	class AActor* FindNearestEnemyInRange() const;

	void FireToTarget(AActor* Target);

};
