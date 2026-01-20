// Fill out your copyright notice in the Description page of Project Settings.


#include "Animations/AnimNotify/UAnimNotify_PerformAttackHit.h"
#include "Autobots/BVAutobotBase.h"

void UUAnimNotify_PerformAttackHit::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	if (!MeshComp) return;

	if (ABVAutobotBase* Autobot = Cast<ABVAutobotBase>(MeshComp->GetOwner()))
	{
		Autobot->PerformAttackHit();
	}
}
