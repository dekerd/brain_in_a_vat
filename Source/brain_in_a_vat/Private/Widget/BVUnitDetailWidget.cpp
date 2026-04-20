// Fill out your copyright notice in the Description page of Project Settings.


#include "Widget/BVUnitDetailWidget.h"

#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Characters/BVAutobotBase.h"
#include "Components/BVHealthComponent.h"
#include "Data/BVUnitData.h"
#include "GenericTeamAgentInterface.h"
#include "Kismet/GameplayStatics.h"

void UBVUnitDetailWidget::SetFromUnit(ABVAutobotBase* InUnit)
{
	// 이전 바인딩 해제
	if (UBVHealthComponent* Prev = BoundHealth.Get())
	{
		Prev->OnHealthChangedUI.RemoveDynamic(this, &UBVUnitDetailWidget::HandleHealthChanged);
	}

	BoundUnit = InUnit;
	BoundHealth = nullptr;

	if (!InUnit)
	{
		if (UnitIconImage)    { UnitIconImage->SetBrushFromTexture(nullptr); }
		if (UnitNameText)     { UnitNameText->SetText(FText::GetEmpty()); }
		if (HealthText)       { HealthText->SetText(FText::GetEmpty()); }
		if (HealthBar)        { HealthBar->SetPercent(0.f); }
		if (DamageText)       { DamageText->SetText(FText::GetEmpty()); }
		if (DefenseText)      { DefenseText->SetText(FText::GetEmpty()); }
		if (AttackSpeedText)  { AttackSpeedText->SetText(FText::GetEmpty()); }
		if (AttackRangeText)  { AttackRangeText->SetText(FText::GetEmpty()); }
		if (VisionRangeText)  { VisionRangeText->SetText(FText::GetEmpty()); }
		return;
	}

	const UBVUnitData* UData = InUnit->UnitData;

	// --- Icon / Name ---
	if (UnitIconImage)
	{
		UTexture2D* IconTex = UData ? UData->UnitIcon : nullptr;
		UnitIconImage->SetBrushFromTexture(IconTex);
	}

	if (UnitNameText)
	{
		if (UData && UData->UnitName != NAME_None)
		{
			UnitNameText->SetText(FText::FromName(UData->UnitName));
		}
		else
		{
			UnitNameText->SetText(FText::FromString(InUnit->GetName()));
		}
	}

	// --- Health bind ---
	BoundHealth = InUnit->HealthComponent;
	if (UBVHealthComponent* HC = BoundHealth.Get())
	{
		HC->OnHealthChangedUI.AddUniqueDynamic(this, &UBVUnitDetailWidget::HandleHealthChanged);
		if (HealthBar) HealthBar->SetPercent(HC->GetHealthRatio());
	}

	// --- Health bar color (ally=green / enemy=red / neutral=yellow) ---
	if (HealthBar)
	{
		FLinearColor BarColor = FLinearColor(1.f, 0.85f, 0.f); // neutral: yellow
		if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
		{
			if (const IGenericTeamAgentInterface* PlayerTeam = Cast<IGenericTeamAgentInterface>(PC))
			{
				const uint8 PlayerId = PlayerTeam->GetGenericTeamId().GetId();
				uint8 UnitId = FGenericTeamId::NoTeam.GetId();
				if (const IGenericTeamAgentInterface* UnitTeam = Cast<IGenericTeamAgentInterface>(InUnit))
				{
					UnitId = UnitTeam->GetGenericTeamId().GetId();
				}
				else if (const AController* UnitCtrl = InUnit->GetController())
				{
					if (const IGenericTeamAgentInterface* CtrlTeam = Cast<IGenericTeamAgentInterface>(UnitCtrl))
					{
						UnitId = CtrlTeam->GetGenericTeamId().GetId();
					}
				}

				if (UnitId != FGenericTeamId::NoTeam.GetId())
				{
					BarColor = (UnitId == PlayerId)
						? FLinearColor(0.1f, 0.9f, 0.1f)   // ally green
						: FLinearColor(0.9f, 0.1f, 0.1f);  // enemy red
				}
			}
		}
		HealthBar->SetFillColorAndOpacity(BarColor);
	}

	UpdateHealthTextFromComponent();

	// --- 스탯 (GAS Attribute) ---
	RefreshLiveStats();
}

void UBVUnitDetailWidget::RefreshLiveStats()
{
	ABVAutobotBase* Unit = BoundUnit.Get();
	if (!Unit) return;

	if (DamageText)
	{
		DamageText->SetText(FText::AsNumber(FMath::RoundToInt(Unit->GetDamage())));
	}
	if (DefenseText)
	{
		DefenseText->SetText(FText::AsNumber(FMath::RoundToInt(Unit->GetDefense())));
	}
	if (AttackSpeedText)
	{
		AttackSpeedText->SetText(FText::AsNumber(Unit->GetAttackSpeed()));
	}
	if (AttackRangeText)
	{
		AttackRangeText->SetText(FText::AsNumber(FMath::RoundToInt(Unit->GetAttackRange())));
	}
	if (VisionRangeText)
	{
		VisionRangeText->SetText(FText::AsNumber(FMath::RoundToInt(Unit->VisionRadius)));
	}

	UpdateHealthTextFromComponent();
}

void UBVUnitDetailWidget::HandleHealthChanged(float NewRatio)
{
	if (HealthBar) HealthBar->SetPercent(NewRatio);
	UpdateHealthTextFromComponent();
}

void UBVUnitDetailWidget::UpdateHealthTextFromComponent()
{
	if (!HealthText) return;

	if (UBVHealthComponent* HC = BoundHealth.Get())
	{
		const int32 Cur = FMath::RoundToInt(HC->GetCurrentHealth());
		const int32 Max = FMath::RoundToInt(HC->GetMaxHealth());
		HealthText->SetText(FText::FromString(FString::Printf(TEXT("%d / %d"), Cur, Max)));
	}
}
