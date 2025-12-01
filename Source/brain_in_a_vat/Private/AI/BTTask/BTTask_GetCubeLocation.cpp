// Fill out your copyright notice in the Description page of Project Settings.

#include "AI/BTTask/BTTask_GetCubeLocation.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "AI/BVAIController.h"


UBTTask_GetCubeLocation::UBTTask_GetCubeLocation()
{
	
}

EBTNodeResult::Type UBTTask_GetCubeLocation::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	EBTNodeResult::Type Result = Super::ExecuteTask(OwnerComp, NodeMemory);

	// Get Owner Comp
	APawn* ControllingPawn = OwnerComp.GetAIOwner()->GetPawn();
	if (ControllingPawn == nullptr)
	{
		return EBTNodeResult::Failed;
	}

	// Get World
	UWorld* World = ControllingPawn->GetWorld();
	if (World == nullptr)
	{
		return EBTNodeResult::Failed;
	}
	

/*

	if (TargetCube == nullptr)
	{
		return EBTNodeResult::Failed;
	}

	const FVector TargetLocation = TargetCube->GetActorLocation();
	UBlackboardComponent* BlackboardComponent = OwnerComp.GetBlackboardComponent();
	if (BlackboardComponent == nullptr)
	{
		return EBTNodeResult::Failed;
	}
	// Set cube location as a blackboard value
	BlackboardComponent->SetValueAsVector(TEXT("TargetLocation"), TargetLocation);
*/
	
	
	return EBTNodeResult::Succeeded;
}
