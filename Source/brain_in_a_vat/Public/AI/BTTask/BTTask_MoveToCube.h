// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_MoveToCube.generated.h"

/**
 * 
 */
UCLASS()
class BRAIN_IN_A_VAT_API UBTTask_MoveToCube : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_MoveToCube();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
};
