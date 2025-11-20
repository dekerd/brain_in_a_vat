// Fill out your copyright notice in the Description page of Project Settings.


#include "Animations/BVAnimInstance.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

UBVAnimInstance::UBVAnimInstance()
{
	MovingThreshold = 3.0f;
	JumpingThreshold = 100.0f;
}

void UBVAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	Owner = Cast<ACharacter>(GetOwningActor());
	if (Owner)
	{
		MovementComponent = Owner->GetCharacterMovement();
	}
}

void UBVAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	if (MovementComponent)
	{
		Velocity = MovementComponent->Velocity;
		GroundSpeed = Velocity.Size2D();
		bIsIdle = GroundSpeed < MovingThreshold;
		bIsFalling = MovementComponent->IsFalling();
		bIsJumping = bIsFalling & (Velocity.Z > JumpingThreshold);
	}
}
