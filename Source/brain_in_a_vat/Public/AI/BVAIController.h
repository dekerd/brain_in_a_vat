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

	virtual ETeamAttitude::Type GetTeamAttitudeTowards(const AActor& Other) const override;

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
	TObjectPtr<AActor> MoveTarget;

	// 레인 이동 관련
	UPROPERTY()
	TObjectPtr<class ABVLane> AssignedLane;

	// true: 이미 레인에 합류 완료 → 적 베이스로 직진
	bool bOnLane = false;

	// 합류 시 이 유닛이 사용할 레인 가로 오프셋(±LaneWidth/2). OnPossess에서 한 번만 결정됨.
	float LaneJoinOffset = 0.f;

	FTimerHandle LaneCheckTimerHandle;

	// 주기적으로 레인 합류 여부를 확인하고, 합류 시 목적지를 적 베이스로 전환
	UFUNCTION()
	void CheckLaneArrival();

	void SetTargetLocationFromLaneState(APawn* ControllingPawn);
};
