// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/BTTask/BTTask_AttackTargetEnemy.h"
#include "Autobots/BVAutobotBase.h"
#include "AIController.h"

UBTTask_AttackTargetEnemy::UBTTask_AttackTargetEnemy()
{
	
}

EBTNodeResult::Type UBTTask_AttackTargetEnemy::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AIController = OwnerComp.GetAIOwner();
	if (!AIController) return EBTNodeResult::Failed;

	APawn* Pawn = AIController->GetPawn();
	if (!Pawn) return EBTNodeResult::Failed;

	ABVAutobotBase* Autobot = Cast<ABVAutobotBase>(Pawn);
	if (!Autobot) return EBTNodeResult::Failed;

	Autobot->Attack();
	
	return EBTNodeResult::Succeeded;
}
