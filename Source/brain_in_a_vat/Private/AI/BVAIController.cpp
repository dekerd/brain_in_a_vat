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
ABVAIController::ABVAIController(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UCrowdFollowingComponent>(TEXT("PathFollowingComponent")))
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

	// 중립(EBVTeam::Neutral = 255 = NoTeam)은 거점 점령을 위해 Hostile로 취급.
	// 같은 팀만 Friendly, 나머지(다른 팀 / 중립 / 미지정)는 모두 Hostile.
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

	// 레인 없는 유닛: City 게리슨이면 RallyDestination, 아니면 기존 TargetBuilding 태그 방식.
	MoveTarget = nullptr;

	if (ABVAutobotBase* AutoPawn = Cast<ABVAutobotBase>(InPawn))
	{
		if (AutoPawn->bHasRallyDestination)
		{
			// City 게리슨 유닛 — 도시 내 rally point으로 이동 후 거기서 정지.
			if (BlackboardComponent)
			{
				BlackboardComponent->SetValueAsVector(TEXT("TargetLocation"), AutoPawn->RallyDestination);
			}
			return;
		}
	}

	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		if (It->ActorHasTag("TargetBuilding"))
		{
			MoveTarget = *It;
			break;
		}
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

	const FVector PawnLoc = ControllingPawn->GetActorLocation();
	FVector Foot = AssignedLane->GetPerpendicularFoot(PawnLoc, LaneJoinOffset);
	const float DistToFoot = FVector::Dist2D(PawnLoc, Foot);

	// --- Stuck detection: if pawn hasn't made progress toward Foot, push a detour waypoint ---
	// Tick interval is 0.5s; if pawn moved < 30 units in that window, it's likely blocked.
	const float MovedThisTick = FVector::Dist2D(PawnLoc, LastPawnLocation);
	LastPawnLocation = PawnLoc;

	if (MovedThisTick < 30.f && DistToFoot > 500.f)
	{
		StuckAccumTime += 0.5f;
	}
	else
	{
		StuckAccumTime = 0.f;
	}

	// If stuck 1s+, try routing via a lateral detour point and force BT to re-path by issuing MoveToLocation
	if (StuckAccumTime >= 1.0f)
	{
		// Alternate detour direction each time we get stuck so we try both sides
		DetourSign = (DetourSign >= 0) ? -1 : 1;

		// Lateral offset perpendicular to the pawn->Foot direction
		const FVector ToFoot = (Foot - PawnLoc).GetSafeNormal2D();
		const FVector Lateral = FVector::CrossProduct(ToFoot, FVector::UpVector).GetSafeNormal2D();
		const float DetourDist = 600.f;
		const FVector Detour = PawnLoc + ToFoot * 200.f + Lateral * DetourDist * (float)DetourSign;

		if (BlackboardComponent)
		{
			BlackboardComponent->SetValueAsVector(TEXT("TargetLocation"), Detour);
		}

		// Also kick an explicit path-following move so the unit starts moving even if BT MoveTo stalled
		FAIMoveRequest Req;
		Req.SetGoalLocation(Detour);
		Req.SetAcceptanceRadius(50.f);
		Req.SetUsePathfinding(true);
		Req.SetAllowPartialPath(true);
		MoveTo(Req);

		StuckAccumTime = 0.f;
		return;
	}

	// Normal case: keep TargetLocation fresh so BT MoveTo re-paths around newly spawned obstacles
	if (BlackboardComponent)
	{
		BlackboardComponent->SetValueAsVector(TEXT("TargetLocation"), Foot);
	}

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

		// 레인 전투 현장 보고: 플레이어 팀이 관여된 교전만 기록(아군 공격 or 아군 피격)
		if (UWorld* World = GetWorld())
		{
			if (ABVPlayerController* PC = Cast<ABVPlayerController>(UGameplayStatics::GetPlayerController(World, 0)))
			{
				const FVector CombatMid = (ControllingPawn->GetActorLocation() + ClosestTarget->GetActorLocation()) * 0.5f;
				PC->ReportCombatLocation(CombatMid);
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
		else if (ABVAutobotBase* AutoPawn = Cast<ABVAutobotBase>(ControllingPawn);
			AutoPawn && AutoPawn->bHasRallyDestination)
		{
			// 게리슨 유닛: 전투 종료 후 rally point로 복귀.
			BlackboardComponent->SetValueAsVector(TEXT("TargetLocation"), AutoPawn->RallyDestination);
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


