// Fill out your copyright notice in the Description page of Project Settings.


#include "BVPlayerState.h"

ABVPlayerState::ABVPlayerState()
{
	Gold = 0;
	Experience = 0.0f;
}

void ABVPlayerState::BeginPlay()
{
	Super::BeginPlay();

	// BP 디테일에서 설정한 초기값을 런타임 값으로 복사.
	// (생성자 시점엔 BP default가 아직 오버라이드되지 않으므로 BeginPlay에서 처리.)
	Gold = InitialGold;
	Experience = InitialExperience;

	// HUD 등 UI가 델리게이트 바인드를 이미 마친 뒤라면 초기 표시가 갱신됨.
	// 바인드가 아직이면 바인드 시점에 GetGold()로 현재값을 한 번 읽어 표시하면 됨.
	OnGoldChange.Broadcast(Gold);
}

void ABVPlayerState::AddRewards(int32 InGold, float InExp)
{

	Gold += InGold;
	Experience += InExp;

	OnGoldChange.Broadcast(Gold);
}
