// Fill out your copyright notice in the Description page of Project Settings.


#include "Autobots/BVAutobotBlue.h"


// Sets default values
ABVAutobotBlue::ABVAutobotBlue()
{
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> CharacterMeshRef(TEXT("/Script/Engine.SkeletalMesh'/Game/SkeletalMesh/base_robot_01/base_robot_01.base_robot_01'"));
	if (CharacterMeshRef.Object)
	{
		GetMesh()->SetSkeletalMesh(CharacterMeshRef.Object);
	}

	static ConstructorHelpers::FClassFinder<UAnimInstance> AnimInstanceClassRef(TEXT("/Script/Engine.AnimBlueprint'/Game/AnimBP/ABP_AutobotBlue.ABP_AutobotBlue_C'"));
	if (AnimInstanceClassRef.Class)
	{
		GetMesh()->SetAnimInstanceClass(AnimInstanceClassRef.Class);
	}

	static ConstructorHelpers::FObjectFinder<UAnimMontage> AttackMontageRef(TEXT("/Script/Engine.AnimMontage'/Game/SkeletalMesh/base_robot_01/Montage_Attack.Montage_Attack'"));
	if (AttackMontageRef.Object)
	{
		AttackMontage = AttackMontageRef.Object;
	}

	
	// Team Info
	TeamFlag = 1;

	// Character Stat
	AttackSpeed = 1.3;

}

// Called when the game starts or when spawned
void ABVAutobotBlue::BeginPlay()
{
	Super::BeginPlay();
	
}

