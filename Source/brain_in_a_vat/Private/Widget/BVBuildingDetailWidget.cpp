// Fill out your copyright notice in the Description page of Project Settings.


#include "Widget/BVBuildingDetailWidget.h"

#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Buildings/BVBuildingBase.h"
#include "Characters/BVAutobotBase.h"
#include "Data/BVBuildingData.h"
#include "Data/BVUnitData.h"

void UBVBuildingDetailWidget::SetFromBuilding(ABVBuildingBase* InBuilding)
{
	BoundBuilding = InBuilding;

	if (!InBuilding)
	{
		// 안전: null 넘어오면 비워두기
		if (BuildingIconImage)   { BuildingIconImage->SetBrushFromTexture(nullptr); }
		if (BuildingNameText)    { BuildingNameText->SetText(FText::GetEmpty()); }
		if (DescriptionText)     { DescriptionText->SetText(FText::GetEmpty()); }
		if (SpawnUnitIconImage)  { SpawnUnitIconImage->SetBrushFromTexture(nullptr); }
		if (SpawnUnitNameText)   { SpawnUnitNameText->SetText(FText::GetEmpty()); }
		if (RespawnProgressBar)  { RespawnProgressBar->SetPercent(0.f); }
		return;
	}

	const UBVBuildingData* BData = InBuilding->BuildingData;

	// --- Building icon / name / description ---
	if (BuildingIconImage)
	{
		UTexture2D* IconTex = BData ? ToRawPtr(BData->BuildingIcon) : InBuilding->BuildingIcon;
		if (IconTex)
		{
			BuildingIconImage->SetBrushFromTexture(IconTex);
		}
	}

	if (BuildingNameText)
	{
		FText Name;
		if (BData && !BData->BuildingName.IsEmpty())
		{
			Name = BData->BuildingName;
		}
		else
		{
			Name = InBuilding->BuildingName;
		}
		BuildingNameText->SetText(Name);
	}

	if (DescriptionText)
	{
		DescriptionText->SetText(BData ? BData->Description : FText::GetEmpty());
	}

	// --- Spawn unit icon / name ---
	// SpawnUnitClass의 CDO에서 UnitData를 읽어 아이콘/이름을 가져온다.
	TSubclassOf<ABVAutobotBase> UnitClass = InBuilding->SpawnUnitClass;
	const UBVUnitData* UData = nullptr;
	if (UnitClass)
	{
		if (const ABVAutobotBase* UnitCDO = UnitClass->GetDefaultObject<ABVAutobotBase>())
		{
			UData = UnitCDO->UnitData;
		}
	}

	if (SpawnUnitIconImage)
	{
		UTexture2D* UnitIcon = UData ? UData->UnitIcon : nullptr;
		if (UnitIcon)
		{
			SpawnUnitIconImage->SetBrushFromTexture(UnitIcon);
			SpawnUnitIconImage->SetVisibility(ESlateVisibility::Visible);
		}
		else
		{
			SpawnUnitIconImage->SetBrushFromTexture(nullptr);
		}
	}

	if (SpawnUnitNameText)
	{
		if (UData && UData->UnitName != NAME_None)
		{
			SpawnUnitNameText->SetText(FText::FromName(UData->UnitName));
		}
		else
		{
			SpawnUnitNameText->SetText(FText::GetEmpty());
		}
	}

	// 초기 진행률은 현재 ElapsedTime 기준으로 세팅 (ticker가 곧 다시 업데이트함)
	if (RespawnProgressBar)
	{
		float Ratio = 0.f;
		if (InBuilding->RespawnInterval > 0.f)
		{
			Ratio = FMath::Fmod(InBuilding->ElapsedTime, InBuilding->RespawnInterval) / InBuilding->RespawnInterval;
		}
		RespawnProgressBar->SetPercent(Ratio);
	}
}

void UBVBuildingDetailWidget::SetRespawnProgress(float Ratio)
{
	if (RespawnProgressBar)
	{
		RespawnProgressBar->SetPercent(FMath::Clamp(Ratio, 0.f, 1.f));
	}
}
