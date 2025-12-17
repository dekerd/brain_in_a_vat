// Fill out your copyright notice in the Description page of Project Settings.


#include "Autobots/BVAutobotRed.h"

#include "AIController.h"


// Sets default values
ABVAutobotRed::ABVAutobotRed()
{
	
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> CharacterMeshRef(TEXT("/Script/Engine.SkeletalMesh'/Game/SkeletalMesh/base_robot_02/base_robot_02.base_robot_02'"));
	if (CharacterMeshRef.Object)
	{
		GetMesh()->SetSkeletalMesh(CharacterMeshRef.Object);
	}

	static ConstructorHelpers::FClassFinder<UAnimInstance> AnimInstanceClassRef(TEXT("/Script/Engine.AnimBlueprint'/Game/AnimBP/ABP_AutobotRed.ABP_AutobotRed_C'"));
	if (AnimInstanceClassRef.Class)
	{
		GetMesh()->SetAnimInstanceClass(AnimInstanceClassRef.Class);
	}

	static ConstructorHelpers::FObjectFinder<UAnimMontage> AttackMontageRef(TEXT("/Script/Engine.AnimMontage'/Game/SkeletalMesh/base_robot_02/Montage_Attack.Montage_Attack'"));
	if (AttackMontageRef.Object)
	{
		AttackMontage = AttackMontageRef.Object;
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Can't find AttackMontage for robot 02!!"));
	}

	// Team Info
	TeamFlag = 2;

	// Character Stat
	AttackSpeed = 0.8;
	UnitStatRowName = TEXT("AutobotRed");
	
}

// Called when the game starts or when spawned
void ABVAutobotRed::BeginPlay()
{
	Super::BeginPlay();
	
}

