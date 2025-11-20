// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GenericTeamAgentInterface.h"
#include "InputActionValue.h"
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

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

protected:
	UPROPERTY(EditAnywhere, Category="Mesh")
	TObjectPtr<class USkeletalMeshComponent> SkeletalMeshComponent;
	
	UPROPERTY(VisibleAnywhere, Category="Camera")
	TObjectPtr<class UCameraComponent> Camera;

	UPROPERTY(VisibleAnywhere, Category="Camera")
	TObjectPtr<class USpringArmComponent> CameraBoom;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	TObjectPtr<class UInputMappingContext> InputMappingContext; 

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, Meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class UInputAction> MoveAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, Meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class UInputAction> JumpAction;

	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);

	UPROPERTY()
	uint32 TeamFlag;

	virtual FGenericTeamId GetGenericTeamId() const override;

};
