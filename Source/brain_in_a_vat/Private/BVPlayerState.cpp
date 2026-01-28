// Fill out your copyright notice in the Description page of Project Settings.


#include "BVPlayerState.h"

ABVPlayerState::ABVPlayerState()
{
	Gold = 0;
	Experience = 0.0f;
}

void ABVPlayerState::AddRewards(int32 InGold, float InExp)
{

	Gold += InGold;
	Experience += InExp;

	OnGoldChange.Broadcast(Gold);
	
	FString DebugMsg = FString::Printf(TEXT("PlayerState Updated - Gold: %d, Exp: %.1f"), Gold, Experience);
	GEngine->AddOnScreenDebugMessage(-1, 3, FColor::Red, DebugMsg);
	
}
