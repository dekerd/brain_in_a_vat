// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "BTService_UpdateClosestEnemy.generated.h"

/**
 * 
 */
UCLASS()
class BRAIN_IN_A_VAT_API UBTService_UpdateClosestEnemy : public UBTService
{
	GENERATED_BODY()

public:
	UBTService_UpdateClosestEnemy();

protected:
	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
};
