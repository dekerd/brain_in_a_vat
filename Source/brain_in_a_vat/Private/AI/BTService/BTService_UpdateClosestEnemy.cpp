// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/BTService/BTService_UpdateClosestEnemy.h"
#include "AIController.h"
#include "AI/BVAIController.h"
#include "Autobots/BVAutobotBase.h"
#include "Perception/AIPerceptionComponent.h"
#include "BehaviorTree/BlackboardComponent.h"

UBTService_UpdateClosestEnemy::UBTService_UpdateClosestEnemy()
{
	NodeName = TEXT("Update Closest Enemy");
	Interval = 0.5f;
	bNotifyTick = true;
}

void UBTService_UpdateClosestEnemy::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

	AAIController* AIController = OwnerComp.GetAIOwner();
	if (!AIController) return;

	APawn* Pawn = AIController->GetPawn();
	if (!Pawn) return;

	UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
	if (!Blackboard) return;

	ABVAIController* BVAIController = Cast<ABVAIController>(AIController);
	UAIPerceptionComponent* Perception = BVAIController ? BVAIController->GetPerceptionComponent() : nullptr;
	if (!Perception) return;

	TArray<AActor*> Actors;
	Perception->GetCurrentlyPerceivedActors(UAISense_Sight::StaticClass(), Actors);

	float Closest = TNumericLimits<float>::Max();
	AActor* ClosestActor = nullptr;
	for (AActor* Actor : Actors)
	{
		if (!Actor || Actor == Pawn) continue;
		if (AIController->GetTeamAttitudeTowards(*Actor) != ETeamAttitude::Hostile) continue;
		if (ABVAutobotBase* TargetBot = Cast<ABVAutobotBase>(Actor))
		{
			if (TargetBot->bIsDead) continue;
		}

		float Dist = FVector::DistSquared(Pawn->GetActorLocation(), Actor->GetActorLocation());
		if (Dist < Closest)
		{
			Closest = Dist;
			ClosestActor = Actor;
		}
	}

	Blackboard->SetValueAsObject(TEXT("AttackTargetActor"), ClosestActor);

}
