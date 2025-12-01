// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_AttackTargetEnemy.generated.h"

/**
 * 
 */
UCLASS()
class BRAIN_IN_A_VAT_API UBTTask_AttackTargetEnemy : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_AttackTargetEnemy();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

	UFUNCTION()
	void HandleAttackFinished(AAIController* AIController);
};
