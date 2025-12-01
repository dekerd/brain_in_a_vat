// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "UAnimNotify_PerformAttackHit.generated.h"

/**
 * 
 */
UCLASS()
class BRAIN_IN_A_VAT_API UUAnimNotify_PerformAttackHit : public UAnimNotify
{
	GENERATED_BODY()

public:
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation) override;
};
