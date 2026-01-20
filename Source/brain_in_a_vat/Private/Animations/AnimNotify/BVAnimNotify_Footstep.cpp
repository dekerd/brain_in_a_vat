// Fill out your copyright notice in the Description page of Project Settings.


#include "Animations/AnimNotify/BVAnimNotify_Footstep.h"

#include "MainCharacter.h"
#include "Autobots/BVAutobotBase.h"

void UBVAnimNotify_Footstep::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{

	if (!MeshComp) return;

	if (AMainCharacter* MainPlayer = Cast<AMainCharacter>(MeshComp->GetOwner()))
	{
		MainPlayer->PlayFootstepSound();
	}
	else if (ABVAutobotBase* Autobot = Cast<ABVAutobotBase>(MeshComp->GetOwner()))
	{
		Autobot->PlayFootstepSound();
	}
}
