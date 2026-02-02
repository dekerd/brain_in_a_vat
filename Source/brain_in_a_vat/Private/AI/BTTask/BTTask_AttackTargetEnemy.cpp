// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/BTTask/BTTask_AttackTargetEnemy.h"
#include "Characters/BVAutobotBase.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"

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

	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	AActor* TargetActor = Cast<AActor>(BB->GetValueAsObject(TEXT("AttackTargetActor")));
	if (TargetActor)
	{
		AIController->SetFocus(TargetActor);
	}

	Autobot->OnAttackFinished.AddUniqueDynamic(this, &UBTTask_AttackTargetEnemy::HandleAttackFinished);
	Autobot->Attack();
	
	return EBTNodeResult::InProgress;
}

void UBTTask_AttackTargetEnemy::HandleAttackFinished(AAIController* AIController)
{
	if (!AIController) return;

	AIController->ClearFocus(EAIFocusPriority::Gameplay);

	UBrainComponent* BrainComp = AIController->GetBrainComponent();
	UBehaviorTreeComponent* BTComp = Cast<UBehaviorTreeComponent>(BrainComp);
	if (!BTComp) return;

	FinishLatentTask(*BTComp, EBTNodeResult::Succeeded);
}
