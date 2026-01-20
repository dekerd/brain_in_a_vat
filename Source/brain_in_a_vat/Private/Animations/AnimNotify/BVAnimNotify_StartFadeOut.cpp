// Fill out your copyright notice in the Description page of Project Settings.


#include "Animations/AnimNotify/BVAnimNotify_StartFadeOut.h"
#include "Autobots/BVAutobotBase.h"

void UBVAnimNotify_StartFadeOut::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation);

	if (!MeshComp) return;

	if (ABVAutobotBase* Autobot = Cast<ABVAutobotBase>(MeshComp->GetOwner()))
	{
		Autobot->StartFadeOut();
	}
}
