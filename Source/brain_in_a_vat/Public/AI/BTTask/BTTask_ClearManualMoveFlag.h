// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_ClearManualMoveFlag.generated.h"

/**
 * 수동 이동 분기 끝에서 bManualMoveActive BB 키를 false로 되돌린다.
 * BT가 다시 정상 분기(공격/레인 이동 등)를 선택하게 하는 용도.
 */
UCLASS()
class BRAIN_IN_A_VAT_API UBTTask_ClearManualMoveFlag : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_ClearManualMoveFlag();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
};
