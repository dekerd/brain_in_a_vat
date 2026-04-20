// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/BVAIController.h"
#include "AI/BVLane.h"
#include "EngineUtils.h"
#include "Characters/BVAutobotBase.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Navigation/CrowdFollowingComponent.h"
#include "Headers/BVTeam.h"
#include "Data/BVUnitData.h"
#include "Weapons/Projectiles/BVProjectileBase.h"
#include "Data/BVProjectileData.h"
#include "BVPlayerController.h"
#include "Kismet/GameplayStatics.h"


// Sets default values
ABVAIController::ABVAIController()
{

	// Blackboard and Behavior Tree
	static ConstructorHelpers::FObjectFinder<UBehaviorTree> BTAssetRef(TEXT("/Script/AIModule.BehaviorTree'/Game/AI/BT_Autobot.BT_Autobot'"));
	if (BTAssetRef.Object != nullptr)
	{
		BTAsset = BTAssetRef.Object;
	}

	static ConstructorHelpers::FObjectFinder<UBlackboardData> BBAssetRef(TEXT("/Script/AIModule.BlackboardData'/Game/AI/BB_Autobot.BB_Autobot'"));
	if (BBAssetRef.Object != nullptr)
	{
		BBAsset = BBAssetRef.Object;
	}

	// Perception Component
	AIPerception = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("AIPerception"));
	SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));

	SightConfig->SightRadius = 1000.f;
	SightConfig->LoseSightRadius = 1000.f;
	SightConfig->PeripheralVisionAngleDegrees = 360.f;

	SightConfig->DetectionByAffiliation.bDetectEnemies = true;
	SightConfig->DetectionByAffiliation.bDetectFriendlies = true;
	SightConfig->DetectionByAffiliation.bDetectNeutrals = true;

	AIPerception->ConfigureSense(*SightConfig);
	AIPerception->SetDominantSense(SightConfig->GetSenseImplementation());

}

void ABVAIController::RunAI()
{
	UBlackboardComponent* BlackboardPtr  = Blackboard.Get();
	if (UseBlackboard(BBAsset, BlackboardPtr))
	{
		bool RunResult = RunBehaviorTree(BTAsset);
		ensure(RunResult);
	}
}

void ABVAIController::StopAI()
{
	UBehaviorTreeComponent* BTComponent = Cast<UBehaviorTreeComponent>(BrainComponent);
	if (BTComponent)
	{
		BTComponent->StopTree();
	}
	
}

ETeamAttitude::Type ABVAIController::GetTeamAttitudeTowards(const AActor& Other) const
{
	const IGenericTeamAgentInterface* OtherTeamAgent = Cast<IGenericTeamAgentInterface>(&Other);
	if (!OtherTeamAgent) return ETeamAttitude::Neutral;

	FGenericTeamId MyTeamId = GetGenericTeamId();
	FGenericTeamId OtherTeamId = OtherTeamAgent->GetGenericTeamId();

	if (MyTeamId == FGenericTeamId::NoTeam || OtherTeamId == FGenericTeamId::NoTeam)
	{
		return ETeamAttitude::Neutral;
	}

	return (MyTeamId == OtherTeamId) ? ETeamAttitude::Friendly : ETeamAttitude::Hostile;
}

void ABVAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	
	RunAI();

	BlackboardComponent = GetBlackboardComponent();

	// 팀 정보 설정
	if (IGenericTeamAgentInterface* TeamAgent = Cast<IGenericTeamAgentInterface>(InPawn))
	{
		SetGenericTeamId(TeamAgent->GetGenericTeamId());
	}

	// 레인이 할당된 유닛: 수선의 발 → 적 베이스 2단계 이동
	AssignedLane = nullptr;
	bOnLane = false;
	LaneJoinOffset = 0.f;

	if (ABVAutobotBase* Autobot = Cast<ABVAutobotBase>(InPawn))
	{

		UE_LOG(LogTemp, Warning, TEXT("Autobot의 레인 할당 상태: %s"), Autobot->AssignedLane ? TEXT("성공!") : TEXT("실패(NULL)"));
		if (Autobot->AssignedLane)
		{
			AssignedLane = Autobot->AssignedLane;

			// 같은 레인에 들어온 유닛들이 한 점에 모이지 않게, 합류 오프셋을 무작위로 한 번 정한다.
			const float HalfWidth = AssignedLane->LaneWidth * 0.5f;
			LaneJoinOffset = FMath::FRandRange(-HalfWidth, HalfWidth);

			if (BlackboardComponent)
			{
				const FVector Foot = AssignedLane->GetPerpendicularFoot(InPawn->GetActorLocation(), LaneJoinOffset);
				BlackboardComponent->SetValueAsVector(TEXT("TargetLocation"), Foot);
			}

			// 레인 합류 여부를 주기적으로 확인
			GetWorldTimerManager().SetTimer(
				LaneCheckTimerHandle,
				this,
				&ABVAIController::CheckLaneArrival,
				0.5f,
				true
			);
			return;
		}
	}

	// 레인 없는 유닛: 기존 TargetBuilding 태그 방식
	MoveTarget = nullptr;
	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		if (It->ActorHasTag("TargetBuilding"))
		{
			MoveTarget = *It;
			break;
		}
	}

	if (!MoveTarget)
	{
		UE_LOG(LogTemp, Warning, TEXT("No Target Cube found"))
	}

	if (BlackboardComponent)
	{
		if (IsValid(MoveTarget))
		{
			BlackboardComponent->SetValueAsVector(TEXT("TargetLocation"), MoveTarget->GetActorLocation());
		}
		else
		{
			BlackboardComponent->SetValueAsVector(TEXT("TargetLocation"), InPawn->GetActorLocation());
		}
	}
}

void ABVAIController::CheckLaneArrival()
{
	if (bOnLane)
	{
		GetWorldTimerManager().ClearTimer(LaneCheckTimerHandle);
		return;
	}

	APawn* ControllingPawn = GetPawn();
	if (!ControllingPawn || !AssignedLane) return;

	const FVector Foot = AssignedLane->GetPerpendicularFoot(ControllingPawn->GetActorLocation(), LaneJoinOffset);
	const float DistToFoot = FVector::Dist2D(ControllingPawn->GetActorLocation(), Foot);

	// 수선의 발 100 유닛 이내면 레인 합류로 판정
	if (DistToFoot < 500.f)
	{
		bOnLane = true;
		GetWorldTimerManager().ClearTimer(LaneCheckTimerHandle);

		if (BlackboardComponent && !BlackboardComponent->GetValueAsBool(TEXT("bIsAttacking")))
		{
			// Default destination
			FVector FinalDestination = AssignedLane->GetEnemyBaseLocation();

			// Check Team ID and reverse destination if hostile
			if (IGenericTeamAgentInterface* TeamAgent = Cast<IGenericTeamAgentInterface>(ControllingPawn))
			{
				if (TeamAgent->GetGenericTeamId() == FGenericTeamId(2))
				{
					FinalDestination = AssignedLane->GetFriendlyBaseLocation(); 
				}
			}

			BlackboardComponent->SetValueAsVector(TEXT("TargetLocation"), FinalDestination);
		}
	}
}

void ABVAIController::SetTargetLocationFromLaneState(APawn* ControllingPawn)
{
	if (!AssignedLane || !BlackboardComponent) return;

	if (bOnLane)
	{
		FVector FinalDestination = AssignedLane->GetEnemyBaseLocation();
		
		if (IGenericTeamAgentInterface* TeamAgent = Cast<IGenericTeamAgentInterface>(ControllingPawn))
		{
			if (TeamAgent->GetGenericTeamId() == FGenericTeamId(2))
			{
				FinalDestination = AssignedLane->GetFriendlyBaseLocation();
			}
		}

		BlackboardComponent->SetValueAsVector(TEXT("TargetLocation"), FinalDestination);
	}
	else
	{
		const FVector Foot = AssignedLane->GetPerpendicularFoot(ControllingPawn->GetActorLocation(), LaneJoinOffset);
		BlackboardComponent->SetValueAsVector(TEXT("TargetLocation"), Foot);

		if (!GetWorldTimerManager().IsTimerActive(LaneCheckTimerHandle))
		{
			GetWorldTimerManager().SetTimer(LaneCheckTimerHandle, this, &ABVAIController::CheckLaneArrival, 0.5f, true);
		}
	}
}

void ABVAIController::BeginPlay()
{
	Super::BeginPlay();

	if (AIPerception)
	{
		UE_LOG(LogTemp, Warning, TEXT("AIPerception in operation"))
		AIPerception->OnPerceptionUpdated.AddDynamic(this, &ABVAIController::OnPerceptionUpdated);
	}

}

void ABVAIController::OnPerceptionUpdated(const TArray<AActor*>& UpdatedActors)
{
	
	APawn* ControllingPawn = GetPawn();
	if (!ControllingPawn) return;

	TArray<AActor*> PerceivedActors;
	AIPerception->GetCurrentlyPerceivedActors(UAISense_Sight::StaticClass(), PerceivedActors);
	
	const FGenericTeamId MyTeamId = GetGenericTeamId();

	// Sticky Target
	AActor* CurrentTarget = Cast<AActor>(BlackboardComponent->GetValueAsObject(TEXT("AttackTargetActor")));
	if (CurrentTarget)
	{
		if (GetTeamAttitudeTowards(*CurrentTarget) == ETeamAttitude::Hostile)
		{
			const float MaxStickDistance = 1000.f;
			const float DistSq = FVector::DistSquared(ControllingPawn->GetActorLocation(), CurrentTarget->GetActorLocation());
			if (DistSq < MaxStickDistance * MaxStickDistance) return;
		}
	}

	float ClosestDistance = TNumericLimits<float>::Max();
	AActor* ClosestTarget = nullptr;
	for (AActor* Actor : PerceivedActors)
	{
		if (!Actor || Actor == ControllingPawn) continue;
		
		ETeamAttitude::Type Attitude = GetTeamAttitudeTowards(*Actor);
		FString AttitudeStr = StaticEnum<ETeamAttitude::Type>()->GetValueAsString(Attitude);
		
		if (Attitude != ETeamAttitude::Hostile) continue;

		if (ABVAutobotBase* TargetBot = Cast<ABVAutobotBase>(Actor))
		{
			if (TargetBot->bIsDead) continue;
		}

		const float distance = FVector::DistSquared(ControllingPawn->GetActorLocation(), Actor->GetActorLocation());
		if ( distance < ClosestDistance )
		{
			ClosestDistance = distance;
			ClosestTarget = Actor;
		}
	}
	
	if (ClosestTarget)
	{
		BlackboardComponent->SetValueAsObject(TEXT("AttackTargetActor"), ClosestTarget);
		BlackboardComponent->SetValueAsBool(TEXT("bIsAttacking"), true);
		UE_LOG(LogTemp, Warning, TEXT("[%s] tries to attack [%s]."), *ControllingPawn->GetName(), *ClosestTarget->GetName())

		// 레인 전투 현장 보고: 플레이어 팀이 관여된 교전만 기록(아군 공격 or 아군 피격)
		if (UWorld* World = GetWorld())
		{
			if (ABVPlayerController* PC = Cast<ABVPlayerController>(UGameplayStatics::GetPlayerController(World, 0)))
			{
				const FVector CombatMid = (ControllingPawn->GetActorLocation() + ClosestTarget->GetActorLocation()) * 0.5f;
				PC->ReportCombatLocation(CombatMid);
				UE_LOG(LogTemp, Warning, TEXT("[Camera] ReportCombatLocation from AI: %s"), *CombatMid.ToString());
			}
		}

		// 원거리 유닛: DA의 ProjectileRange 기준으로 사정거리 바깥에서 멈춤
		FVector TargetLoc = ClosestTarget->GetActorLocation();
		if (ABVAutobotBase* Autobot = Cast<ABVAutobotBase>(ControllingPawn))
		{
			if (Autobot->UnitData && Autobot->UnitData->WeaponData)
			{
				float ProjRange = (Autobot->UnitData->WeaponData->ProjectileRange > 0.f)
					? Autobot->UnitData->WeaponData->ProjectileRange : 0.f;

				if (ProjRange > 0.f)
				{
					const float Dist = FVector::Dist(ControllingPawn->GetActorLocation(), TargetLoc);
					if (Dist <= ProjRange * 0.9f)
					{
						TargetLoc = ControllingPawn->GetActorLocation();
					}
					else
					{
						const FVector Dir = (TargetLoc - ControllingPawn->GetActorLocation()).GetSafeNormal();
						TargetLoc = TargetLoc - Dir * ProjRange * 0.8f;
					}
				}
			}
		}
		BlackboardComponent->SetValueAsVector(TEXT("TargetLocation"), TargetLoc);
	}
	else
	{
		BlackboardComponent->SetValueAsObject(TEXT("AttackTargetActor"), nullptr);
		BlackboardComponent->SetValueAsBool(TEXT("bIsAttacking"), false);

		if (AssignedLane)
		{
			// 레인 유닛: 합류 전이면 수선의 발, 합류 후면 적 베이스로 이동
			SetTargetLocationFromLaneState(ControllingPawn);
		}
		else if (IsValid(MoveTarget))
		{
			BlackboardComponent->SetValueAsVector(TEXT("TargetLocation"), MoveTarget->GetActorLocation());
		}
		else
		{
			BlackboardComponent->SetValueAsVector(TEXT("TargetLocation"), ControllingPawn->GetActorLocation());
		}
	}
	
}


