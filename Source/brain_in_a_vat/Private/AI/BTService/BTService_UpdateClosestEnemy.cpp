// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/BTService/BTService_UpdateClosestEnemy.h"
#include "AIController.h"
#include "AI/BVAIController.h"
#include "Characters/BVAutobotBase.h"
#include "Perception/AIPerceptionComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Data/BVUnitData.h"
#include "Weapons/Projectiles/BVProjectileBase.h"
#include "Data/BVProjectileData.h"

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

	if (ClosestActor)
	{
		// 기본값: 적 위치로 이동 (멜리 유닛 / 원거리 폴백)
		Blackboard->SetValueAsVector(TEXT("TargetLocation"), ClosestActor->GetActorLocation());

		// 원거리 유닛: DA의 ProjectileRange 기준으로 사정거리 유지하도록 덮어쓰기
		if (ABVAutobotBase* Autobot = Cast<ABVAutobotBase>(Pawn))
		{
			if (Autobot->UnitData && Autobot->UnitData->ProjectileClass)
			{
				float ProjRange = 0.f;
				if (const ABVProjectileBase* CDO = Autobot->UnitData->ProjectileClass->GetDefaultObject<ABVProjectileBase>())
				{
					if (CDO->ProjectileData && CDO->ProjectileData->ProjectileRange > 0.f)
					{
						ProjRange = CDO->ProjectileData->ProjectileRange;
					}
				}

				if (ProjRange > 0.f)
				{
					const FVector MyLoc = Pawn->GetActorLocation();
					const FVector EnemyLoc = ClosestActor->GetActorLocation();
					const float Dist = FVector::Dist(MyLoc, EnemyLoc);

					if (Dist <= ProjRange * 0.9f)
					{
						Blackboard->SetValueAsVector(TEXT("TargetLocation"), MyLoc);
					}
					else
					{
						const FVector Dir = (EnemyLoc - MyLoc).GetSafeNormal();
						Blackboard->SetValueAsVector(TEXT("TargetLocation"), EnemyLoc - Dir * ProjRange * 0.8f);
					}
				}
			}
		}
	}

}
