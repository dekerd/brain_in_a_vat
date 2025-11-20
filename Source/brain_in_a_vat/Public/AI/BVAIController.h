// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Runtime/AIModule/Classes/AIController.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "BVAIController.generated.h"

UCLASS()
class BRAIN_IN_A_VAT_API ABVAIController : public AAIController
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ABVAIController();

	void RunAI();
	void StopAI();

protected:
	virtual void OnPossess(APawn* InPawn) override;
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="AI")
	TObjectPtr<UAIPerceptionComponent> AIPerception;

	UPROPERTY()
	TObjectPtr<UAISenseConfig_Sight> SightConfig;

	UFUNCTION()
	void OnPerceptionUpdated(const TArray<AActor*>& UpdatedActors);

private:
	UPROPERTY()
	TObjectPtr<class UBlackboardData> BBAsset;

	UPROPERTY()
	TObjectPtr<class UBehaviorTree> BTAsset;

	UPROPERTY(Transient)
	UBlackboardComponent* BlackboardComponent;

	UPROPERTY()
	TObjectPtr<AActor> TargetCube;
};
