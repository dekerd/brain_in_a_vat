// Fill out your copyright notice in the Description page of Project Settings.

#include "AI/BVLane.h"

ABVLane::ABVLane()
{
	PrimaryActorTick.bCanEverTick = false;
}

FVector ABVLane::GetPerpendicularFoot(const FVector& UnitPos) const
{
	if (!FriendlyBase || !EnemyBase) return UnitPos;

	const FVector A = FriendlyBase->GetActorLocation();
	const FVector B = EnemyBase->GetActorLocation();
	const FVector AB = B - A;

	const float LenSq = AB.SizeSquared();
	if (LenSq < KINDA_SMALL_NUMBER) return A;

	// t = dot(UP - A, AB) / |AB|^2, [0,1] 범위로 클램프
	const float t = FMath::Clamp(FVector::DotProduct(UnitPos - A, AB) / LenSq, 0.f, 1.f);

	FVector Foot = A + t * AB;
	Foot.Z = UnitPos.Z; // 유닛의 높이 유지
	return Foot;
}

FVector ABVLane::GetEnemyBaseLocation() const
{
	return EnemyBase ? EnemyBase->GetActorLocation() : FVector::ZeroVector;
}

FVector ABVLane::GetFriendlyBaseLocation() const
{
	return FriendlyBase ? FriendlyBase->GetActorLocation() : FVector::ZeroVector;
}
