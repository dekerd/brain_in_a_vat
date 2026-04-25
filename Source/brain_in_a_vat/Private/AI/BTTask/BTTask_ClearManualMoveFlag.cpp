// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/BTTask/BTTask_ClearManualMoveFlag.h"
#include "BehaviorTree/BlackboardComponent.h"

UBTTask_ClearManualMoveFlag::UBTTask_ClearManualMoveFlag()
{
	NodeName = TEXT("Clear Manual Move Flag");
}

EBTNodeResult::Type UBTTask_ClearManualMoveFlag::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	if (!BB) return EBTNodeResult::Failed;

	BB->SetValueAsBool(TEXT("bManualMoveActive"), false);
	return EBTNodeResult::Succeeded;
}
