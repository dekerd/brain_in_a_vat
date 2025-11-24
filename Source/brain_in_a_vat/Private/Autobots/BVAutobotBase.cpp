// Fill out your copyright notice in the Description page of Project Settings.


#include "Autobots/BVAutobotBase.h"

#include "AI/BVAIController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GAS/CombatAttributeSet.h"
#include "Components/CapsuleComponent.h"

// Sets default values
ABVAutobotBase::ABVAutobotBase()
{
	// AI 
	AIControllerClass = ABVAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	// Capsule
	GetCapsuleComponent()->InitCapsuleSize(30.f, 42.0f);

	// Mesh
	float CapsuleHalfHeight = GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight();
	GetMesh()->SetRelativeLocationAndRotation(FVector(0.0f, 0.0f, -CapsuleHalfHeight), FRotator(0.0f, -90.0f, 0.0f));
	GetMesh()->SetAnimationMode(EAnimationMode::AnimationBlueprint);
	
	// Movement
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f);
	
	GetCharacterMovement()->JumpZVelocity = 400.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = 150.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;

	// Crowd
	GetCharacterMovement()->bUseRVOAvoidance = true;
	GetCharacterMovement()->AvoidanceConsiderationRadius = 200.f;

	// Gameplay Ability System (GAS)
	ASC = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("ASC"));
	CombatAttributes = CreateDefaultSubobject<UCombatAttributeSet>(TEXT("CombatAttributes"));

}

FGenericTeamId ABVAutobotBase::GetGenericTeamId() const
{
	if (const IGenericTeamAgentInterface* TeamAgent = Cast<IGenericTeamAgentInterface>(GetController()))
	{
		return TeamAgent->GetGenericTeamId();
	}
	return IGenericTeamAgentInterface::GetGenericTeamId();
}

// Called when the game starts or when spawned
void ABVAutobotBase::BeginPlay()
{
	Super::BeginPlay();

	// Setting Team Information
	AAIController* AIController = Cast<AAIController>(GetController());
	if (AIController)
	{
		AIController->SetGenericTeamId(FGenericTeamId(TeamFlag));
	}

	// Setting ASC
	if (ASC)
	{
		ASC->InitAbilityActorInfo(this, this);
		if (CombatAttributes)
		{
			ASC->GetGameplayAttributeValueChangeDelegate(CombatAttributes->GetHealthAttribute()).AddUObject(this, &ABVAutobotBase::OnHealthChanged);
		}
	}
}

void ABVAutobotBase::OnHealthChanged(const FOnAttributeChangeData& Data)
{
	const float NewHealth = Data.NewValue;
	if (NewHealth <= 0.0f)
	{
		// TODO : Character Dead Handling
	}
}

UAbilitySystemComponent* ABVAutobotBase::GetAbilitySystemComponent() const
{
	return ASC;
}

void ABVAutobotBase::Attack()
{
	if (!AttackMontage) return;
	
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && !AnimInstance->Montage_IsPlaying(AttackMontage))
	{
		AnimInstance->Montage_Play(AttackMontage, AttackSpeed);
	}
	
}


