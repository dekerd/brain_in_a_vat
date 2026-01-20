// Fill out your copyright notice in the Description page of Project Settings.


#include "BVAnimNotify_AttackSound.h"

#include "Autobots/BVAutobotBase.h"


void UBVAnimNotify_AttackSound::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
                                       const FAnimNotifyEventReference& EventReference)
{
	if (!MeshComp) return;

	if (ABVAutobotBase* Autobot = Cast<ABVAutobotBase>(MeshComp->GetOwner()))
	{
		Autobot->PlayAttackSound();
	}
}
