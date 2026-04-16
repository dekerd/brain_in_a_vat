// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "BVAnimInstance.generated.h"

/**
 * 
 */
UCLASS()
class BRAIN_IN_A_VAT_API UBVAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	UBVAnimInstance();

	UFUNCTION()
	void SetIsDead();

	// 외부에서(예: 원거리 유닛이 사거리 안에서 조준 중일 때) idle 강제 ON/OFF.
	// true이면 NativeUpdateAnimation에서 Velocity와 무관하게 bIsIdle=true로 고정한다.
	UFUNCTION(BlueprintCallable, Category = "Character")
	void SetForceIdle(bool bInForceIdle) { bForceIdle = bInForceIdle; }

protected:

	virtual void NativeInitializeAnimation() override;

	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Character)
	TObjectPtr<class ACharacter> Owner;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Character)
	TObjectPtr<class UCharacterMovementComponent> MovementComponent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Character)
	FVector Velocity;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Character)
	float GroundSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Character)
	float JumpingThreshold;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Character)
	float MovingThreshold;

	// States
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Character)
	uint8 bIsIdle : 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Character)
	uint8 bIsFalling : 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Character)
	uint8 bIsJumping : 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Character)
	uint8 bIsDead : 1;

	// SetForceIdle()로 켜진다. 사거리 안에서 사격 중일 때 walking 모션을 차단하기 위한 플래그.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Character)
	uint8 bForceIdle : 1;
};
