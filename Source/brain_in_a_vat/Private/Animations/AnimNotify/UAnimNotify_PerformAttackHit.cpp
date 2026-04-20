// Fill out your copyright notice in the Description page of Project Settings.


#include "Animations/AnimNotify/UAnimNotify_PerformAttackHit.h"
#include "Characters/BVAutobotBase.h"

void UUAnimNotify_PerformAttackHit::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	if (!IsValid(MeshComp)) return;

	// 에디터 프리뷰 월드에서 노티파이가 실행될 때는 Owner가 CDO 또는 null이므로 건너뜀.
	UWorld* World = MeshComp->GetWorld();
	if (!World || !World->IsGameWorld()) return;

	AActor* Owner = MeshComp->GetOwner();
	if (!IsValid(Owner)) return;

	if (ABVAutobotBase* Autobot = Cast<ABVAutobotBase>(Owner))
	{
		Autobot->PerformAttackHit();
	}
}
