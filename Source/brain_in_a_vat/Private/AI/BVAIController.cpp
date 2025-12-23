// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/BVAIController.h"
#include "EngineUtils.h"
#include "Autobots/BVAutobotBase.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Navigation/CrowdFollowingComponent.h"


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
	SightConfig->LoseSightRadius = 2000.f;
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

void ABVAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	// Generate the blackboard and the behavior tree
	RunAI();

	// Find the cube
	TargetCube = nullptr;
	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		if (It->ActorHasTag("MoveTarget"))
		{
			// UE_LOG(LogTemp, Warning, TEXT("TargetCube Found!"))
			TargetCube = *It;
			break;
		}
	}

	if (TargetCube == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("No Target Cube found"))
	}

	// Set Blackboard Value
	BlackboardComponent = GetBlackboardComponent();
	BlackboardComponent->SetValueAsVector(TEXT("TargetLocation"), TargetCube->GetActorLocation());
	
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

	// UAIPerceptionComponent* PerceptionComponent = GetPerceptionComponent();
	// if (!PerceptionComponent) return;

	TArray<AActor*> PerceivedActors;
	AIPerception->GetCurrentlyPerceivedActors(UAISense_Sight::StaticClass(), PerceivedActors);

	for (AActor* Actor : PerceivedActors)
	{
		if (Actor)
		{
			FString DebugMsg = FString::Printf(TEXT("Perceived: %s -> %s"), *ControllingPawn->GetName(), *Actor->GetName());
			GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Cyan, DebugMsg);
		}
	}
	
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
		BlackboardComponent->SetValueAsVector(TEXT("TargetLocation"), ClosestTarget->GetActorLocation());
		BlackboardComponent->SetValueAsBool(TEXT("bIsAttacking"), true);
		UE_LOG(LogTemp, Warning, TEXT("[%s] tries to attack [%s]."), *ControllingPawn->GetName(), *ClosestTarget->GetName())
	}
	
}


