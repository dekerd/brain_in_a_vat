// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/BTTask/BTTask_AttackTargetEnemy.h"
#include "Characters/BVAutobotBase.h"
#include "Data/BVUnitData.h"
#include "Weapons/Projectiles/BVProjectileBase.h"
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

	// 원거리 유닛: 멜리 몽타주(Attack)를 재생하지 않는다.
	// 발사는 ABVAutobotBase::TickRangedAttack가 쿨다운에 맞춰 자동으로 처리한다.
	// 또한 BT가 곧장 MoveTo로 다시 접근하지 않도록 즉시 정지시키고, Succeed로 빠져나간다.
	if (Autobot->UnitData && Autobot->UnitData->WeaponData)
	{
		AIController->StopMovement();
		return EBTNodeResult::Succeeded;
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
