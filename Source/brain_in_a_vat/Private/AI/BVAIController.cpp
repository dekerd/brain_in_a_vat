// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/BVAIController.h"
#include "EngineUtils.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"


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
	SightConfig->PeripheralVisionAngleDegrees = 70.f;

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
	
	const FGenericTeamId MyTeamId = GetGenericTeamId();
	
	for (AActor* Actor : UpdatedActors)
	{
		if (!Actor || Actor == ControllingPawn) continue;
		
		ETeamAttitude::Type Attitude = GetTeamAttitudeTowards(*Actor);
		FString AttitudeStr = StaticEnum<ETeamAttitude::Type>()->GetValueAsString(Attitude);
		
		if (Attitude == ETeamAttitude::Hostile)
		{
			UE_LOG(LogTemp, Warning, TEXT("[%s] has detected the enemy : [%s]."), *ControllingPawn->GetName(), *Actor->GetName())
		}
	}
}


