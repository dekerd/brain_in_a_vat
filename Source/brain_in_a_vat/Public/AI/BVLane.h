// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BVLane.generated.h"

/**
 * 아군 베이스와 적군 베이스를 잇는 라인전 레인을 나타내는 액터.
 * 유닛이 스폰될 때 수선의 발(perpendicular foot)로 이동 후 적 베이스 방향으로 진군한다.
 */
UCLASS()
class BRAIN_IN_A_VAT_API ABVLane : public AActor
{
	GENERATED_BODY()

public:
	ABVLane();
	
	virtual void Tick(float DeltaTime) override;

	// 유닛 위치에서 레인 직선에 내린 수선의 발을 반환 (Z는 유닛 높이 유지)
	FVector GetPerpendicularFoot(const FVector& UnitPos) const;

	FVector GetEnemyBaseLocation() const;
	FVector GetFriendlyBaseLocation() const;

	// 아군 베이스 (레인의 시작점)
	UPROPERTY(EditAnywhere, Category = "Lane")
	TObjectPtr<AActor> FriendlyBase;

	// 적군 베이스 (레인의 끝점 / 목적지)
	UPROPERTY(EditAnywhere, Category = "Lane")
	TObjectPtr<AActor> EnemyBase;
};
