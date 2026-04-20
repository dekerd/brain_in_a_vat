// Fill out your copyright notice in the Description page of Project Settings.


#include "Widget/BVBuildingDetailWidget.h"

#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Buildings/BVBuildingBase.h"
#include "Characters/BVAutobotBase.h"
#include "Components/BVHealthComponent.h"
#include "Data/BVBuildingData.h"
#include "Data/BVUnitData.h"
#include "GenericTeamAgentInterface.h"
#include "Kismet/GameplayStatics.h"

namespace
{
	static void SetWidgetCollapsed(UWidget* W, bool bCollapsed)
	{
		if (!W) return;
		W->SetVisibility(bCollapsed ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
	}
}

void UBVBuildingDetailWidget::SetFromBuilding(ABVBuildingBase* InBuilding)
{
	// 이전 바인딩 해제
	if (UBVHealthComponent* Prev = BoundHealth.Get())
	{
		Prev->OnHealthChangedUI.RemoveDynamic(this, &UBVBuildingDetailWidget::HandleHealthChanged);
	}

	BoundBuilding = InBuilding;
	BoundHealth = nullptr;

	if (!InBuilding)
	{
		// 안전: null 넘어오면 비워두기
		if (BuildingIconImage)   { BuildingIconImage->SetBrushFromTexture(nullptr); }
		if (BuildingNameText)    { BuildingNameText->SetText(FText::GetEmpty()); }
		if (DescriptionText)     { DescriptionText->SetText(FText::GetEmpty()); }
		if (HealthText)          { HealthText->SetText(FText::GetEmpty()); }
		if (HealthBar)           { HealthBar->SetPercent(0.f); }
		SetWidgetCollapsed(SpawnUnitIconImage, true);
		SetWidgetCollapsed(SpawnUnitNameText, true);
		SetWidgetCollapsed(RespawnProgressBar, true);
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

	// --- Health bind ---
	BoundHealth = InBuilding->GetHealthComponent();
	if (UBVHealthComponent* HC = BoundHealth.Get())
	{
		HC->OnHealthChangedUI.AddUniqueDynamic(this, &UBVBuildingDetailWidget::HandleHealthChanged);
		if (HealthBar) HealthBar->SetPercent(HC->GetHealthRatio());
	}
	UpdateHealthTextFromComponent();

	// --- Health bar color (ally=green / enemy=red / neutral=yellow) ---
	if (HealthBar)
	{
		FLinearColor BarColor = FLinearColor(1.f, 0.85f, 0.f); // neutral: yellow
		if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
		{
			if (const IGenericTeamAgentInterface* PlayerTeam = Cast<IGenericTeamAgentInterface>(PC))
			{
				const uint8 PlayerId = PlayerTeam->GetGenericTeamId().GetId();
				uint8 BuildingId = FGenericTeamId::NoTeam.GetId();
				if (const IGenericTeamAgentInterface* BldgTeam = Cast<IGenericTeamAgentInterface>(InBuilding))
				{
					BuildingId = BldgTeam->GetGenericTeamId().GetId();
				}

				if (BuildingId != FGenericTeamId::NoTeam.GetId())
				{
					BarColor = (BuildingId == PlayerId)
						? FLinearColor(0.1f, 0.9f, 0.1f)   // ally green
						: FLinearColor(0.9f, 0.1f, 0.1f);  // enemy red
				}
			}
		}
		HealthBar->SetFillColorAndOpacity(BarColor);
	}

	// --- Spawn unit icon / name ---
	// SpawnUnitClass의 CDO에서 UnitData를 읽어 아이콘/이름을 가져온다.
	// 스폰 유닛이 없거나 UnitData/아이콘이 없으면 스폰 관련 위젯 전부 숨김.
	TSubclassOf<ABVAutobotBase> UnitClass = InBuilding->SpawnUnitClass;
	const UBVUnitData* UData = nullptr;
	if (UnitClass)
	{
		if (const ABVAutobotBase* UnitCDO = UnitClass->GetDefaultObject<ABVAutobotBase>())
		{
			UData = UnitCDO->UnitData;
		}
	}

	const bool bHasSpawnUnit = (UnitClass != nullptr) && (UData != nullptr);

	if (!bHasSpawnUnit)
	{
		SetWidgetCollapsed(SpawnUnitIconImage, true);
		SetWidgetCollapsed(SpawnUnitNameText, true);
		SetWidgetCollapsed(RespawnProgressBar, true);
		return;
	}

	// 스폰 유닛 존재 → 관련 위젯 표시
	SetWidgetCollapsed(SpawnUnitIconImage, false);
	SetWidgetCollapsed(SpawnUnitNameText, false);
	SetWidgetCollapsed(RespawnProgressBar, false);

	if (SpawnUnitIconImage)
	{
		UTexture2D* UnitIcon = UData->UnitIcon;
		if (UnitIcon)
		{
			SpawnUnitIconImage->SetBrushFromTexture(UnitIcon);
		}
		else
		{
			SpawnUnitIconImage->SetBrushFromTexture(nullptr);
		}
	}

	if (SpawnUnitNameText)
	{
		if (UData->UnitName != NAME_None)
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

void UBVBuildingDetailWidget::HandleHealthChanged(float NewRatio)
{
	if (HealthBar) HealthBar->SetPercent(NewRatio);
	UpdateHealthTextFromComponent();
}

void UBVBuildingDetailWidget::UpdateHealthTextFromComponent()
{
	if (!HealthText) return;

	if (UBVHealthComponent* HC = BoundHealth.Get())
	{
		const int32 Cur = FMath::RoundToInt(HC->GetCurrentHealth());
		const int32 Max = FMath::RoundToInt(HC->GetMaxHealth());
		HealthText->SetText(FText::FromString(FString::Printf(TEXT("%d / %d"), Cur, Max)));
	}
	else
	{
		HealthText->SetText(FText::GetEmpty());
	}
}
