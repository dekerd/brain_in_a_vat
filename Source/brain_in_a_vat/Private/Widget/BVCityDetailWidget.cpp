// Fill out your copyright notice in the Description page of Project Settings.


#include "Widget/BVCityDetailWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Buildings/BVBuildingBase.h"
#include "Buildings/BVCityBase.h"
#include "Characters/BVAutobotBase.h"
#include "Components/Image.h"
#include "Components/PanelWidget.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Data/BVBuildingData.h"
#include "Data/BVUnitData.h"
#include "Headers/BVTeam.h"
#include "Widget/BVConstructionMenuSlotWidget.h"

namespace
{
	static void SetCityWidgetCollapsed(UWidget* W, bool bCollapsed)
	{
		if (!W) return;
		W->SetVisibility(bCollapsed ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
	}

	static FText TeamToLabel(EBVTeam Team)
	{
		switch (Team)
		{
		case EBVTeam::Player:  return FText::FromString(TEXT("아군"));
		case EBVTeam::Enemy:   return FText::FromString(TEXT("적"));
		case EBVTeam::Neutral: return FText::FromString(TEXT("중립"));
		default:               return FText::FromString(TEXT("-"));
		}
	}
}

void UBVCityDetailWidget::SetFromCity(ABVCityBase* InCity)
{
	// 이전 바인딩 해제.
	if (ABVCityBase* Prev = BoundCity.Get())
	{
		Prev->OnCaptureProgressChanged.RemoveDynamic(this, &UBVCityDetailWidget::HandleCaptureProgressChanged);
	}

	BoundCity = InCity;

	if (!InCity)
	{
		if (BuildingIconImage) BuildingIconImage->SetBrushFromTexture(nullptr);
		if (BuildingNameText)  BuildingNameText->SetText(FText::GetEmpty());
		if (DescriptionText)   DescriptionText->SetText(FText::GetEmpty());
		if (CaptureProgressBar) CaptureProgressBar->SetPercent(0.f);
		if (OwnerText)         OwnerText->SetText(FText::GetEmpty());
		if (CaptureProgressText) CaptureProgressText->SetText(FText::GetEmpty());
		SetCityWidgetCollapsed(SpawnUnitIconImage, true);
		SetCityWidgetCollapsed(SpawnUnitNameText, true);
		SetCityWidgetCollapsed(RespawnProgressBar, true);
		if (BuildableContainer)
		{
			BuildableContainer->ClearChildren();
			SetCityWidgetCollapsed(BuildableContainer, true);
		}
		return;
	}

	const UBVBuildingData* BData = InCity->BuildingData;

	// --- 아이콘 / 이름 / 설명 ---
	if (BuildingIconImage)
	{
		UTexture2D* IconTex = BData ? ToRawPtr(BData->BuildingIcon) : InCity->BuildingIcon;
		if (IconTex) BuildingIconImage->SetBrushFromTexture(IconTex);
	}

	if (BuildingNameText)
	{
		FText Name = (BData && !BData->BuildingName.IsEmpty()) ? BData->BuildingName : InCity->BuildingName;
		BuildingNameText->SetText(Name);
	}

	if (DescriptionText)
	{
		DescriptionText->SetText(BData ? BData->Description : FText::GetEmpty());
	}

	// --- 점령 진행도 바인딩 ---
	InCity->OnCaptureProgressChanged.AddUniqueDynamic(this, &UBVCityDetailWidget::HandleCaptureProgressChanged);
	ApplyCaptureVisuals(InCity->GetCaptureProgress());

	// --- 스폰 유닛 정보 (도시가 점령되면 생산) ---
	// 정적인 데이터(아이콘/이름)는 최초 1회 세팅. 표시 여부는 UpdateSpawnUnitVisibility가 담당.
	TSubclassOf<ABVAutobotBase> UnitClass = InCity->SpawnUnitClass;
	const UBVUnitData* UData = nullptr;
	if (UnitClass)
	{
		if (const ABVAutobotBase* UnitCDO = UnitClass->GetDefaultObject<ABVAutobotBase>())
		{
			UData = UnitCDO->UnitData;
		}
	}

	if (UData)
	{
		if (SpawnUnitIconImage)
		{
			SpawnUnitIconImage->SetBrushFromTexture(UData->UnitIcon);
		}

		if (SpawnUnitNameText)
		{
			SpawnUnitNameText->SetText(UData->UnitName != NAME_None
				? FText::FromName(UData->UnitName)
				: FText::GetEmpty());
		}

		if (RespawnProgressBar)
		{
			float Ratio = 0.f;
			if (InCity->RespawnInterval > 0.f)
			{
				Ratio = FMath::Fmod(InCity->ElapsedTime, InCity->RespawnInterval) / InCity->RespawnInterval;
			}
			RespawnProgressBar->SetPercent(Ratio);
		}
	}

	// 현재 팀 기준으로 스폰 섹션 표시 여부 결정.
	UpdateSpawnUnitVisibility();

	// 도시별 건설 카탈로그 채우기 (Player 소유일 때만 보임).
	RebuildBuildableCatalog();
}

void UBVCityDetailWidget::SetRespawnProgress(float Ratio)
{
	if (RespawnProgressBar)
	{
		RespawnProgressBar->SetPercent(FMath::Clamp(Ratio, 0.f, 1.f));
	}
}

void UBVCityDetailWidget::HandleCaptureProgressChanged(float NewProgress)
{
	ApplyCaptureVisuals(NewProgress);
}

void UBVCityDetailWidget::ApplyCaptureVisuals(float Progress)
{
	// Percent = |progress|, Color = 부호에 따라.
	const float AbsProgress = FMath::Abs(Progress);

	if (CaptureProgressBar)
	{
		CaptureProgressBar->SetPercent(AbsProgress);

		FLinearColor FillColor;
		if (Progress > KINDA_SMALL_NUMBER)       FillColor = PlayerCaptureColor;
		else if (Progress < -KINDA_SMALL_NUMBER) FillColor = EnemyCaptureColor;
		else                                     FillColor = NeutralColor;

		CaptureProgressBar->SetFillColorAndOpacity(FillColor);
	}

	// 현재 소유 팀 라벨.
	if (OwnerText)
	{
		EBVTeam CurrentTeam = EBVTeam::Neutral;
		if (ABVCityBase* City = BoundCity.Get())
		{
			CurrentTeam = City->TeamType;
		}
		OwnerText->SetText(TeamToLabel(CurrentTeam));
	}

	// "Player 45%" / "Enemy 80%" / "Neutral" 형식 텍스트.
	if (CaptureProgressText)
	{
		FText Label;
		if (Progress > KINDA_SMALL_NUMBER)
		{
			Label = FText::FromString(FString::Printf(TEXT("아군 %d%%"), FMath::RoundToInt(AbsProgress * 100.f)));
		}
		else if (Progress < -KINDA_SMALL_NUMBER)
		{
			Label = FText::FromString(FString::Printf(TEXT("적 %d%%"), FMath::RoundToInt(AbsProgress * 100.f)));
		}
		else
		{
			Label = FText::FromString(TEXT("중립"));
		}
		CaptureProgressText->SetText(Label);
	}

	// 팀이 변경됐을 수 있으니 스폰 섹션 + 빌드 카탈로그 가시성 재평가.
	UpdateSpawnUnitVisibility();
	RebuildBuildableCatalog();
}

void UBVCityDetailWidget::UpdateSpawnUnitVisibility()
{
	const ABVCityBase* City = BoundCity.Get();

	// 표시 조건: 도시가 유효 + 중립이 아님 + SpawnUnitClass 존재.
	bool bShouldShow = false;
	if (City && City->TeamType != EBVTeam::Neutral && City->SpawnUnitClass)
	{
		bShouldShow = true;
	}

	SetCityWidgetCollapsed(SpawnUnitIconImage, !bShouldShow);
	SetCityWidgetCollapsed(SpawnUnitNameText, !bShouldShow);
	SetCityWidgetCollapsed(RespawnProgressBar, !bShouldShow);
}

void UBVCityDetailWidget::RebuildBuildableCatalog()
{
	if (!BuildableContainer) return;

	// 컨테이너는 매번 깨끗이 비운 뒤 다시 채움. (도시 전환/팀 전환 모두 동일 경로.)
	BuildableContainer->ClearChildren();

	ABVCityBase* City = BoundCity.Get();

	// Player 소유 + 카탈로그 항목 존재 + 슬롯 클래스 지정 — 셋 다 만족할 때만 채움.
	const bool bShouldShow =
		City &&
		City->TeamType == EBVTeam::Player &&
		City->BuildableBuildings.Num() > 0 &&
		CatalogEntryWidgetClass != nullptr;

	SetCityWidgetCollapsed(BuildableContainer, !bShouldShow);
	if (!bShouldShow) return;

	for (const TSubclassOf<ABVBuildingBase>& BuildingClass : City->BuildableBuildings)
	{
		if (!BuildingClass) continue;

		// CDO에서 BuildingData를 꺼내 슬롯 초기화. DA가 없는 클래스는 표시할 정보가 없으니 스킵.
		const ABVBuildingBase* CDO = BuildingClass->GetDefaultObject<ABVBuildingBase>();
		UBVBuildingData* SlotDA = CDO ? CDO->BuildingData : nullptr;
		if (!SlotDA) continue;

		// 변수명을 'Slot'으로 두면 UWidget::Slot 멤버를 가려서 C4458이 뜸 → SlotWidget으로.
		UBVConstructionMenuSlotWidget* SlotWidget =
			CreateWidget<UBVConstructionMenuSlotWidget>(this, CatalogEntryWidgetClass);
		if (!SlotWidget) continue;

		// City 컨텍스트 주입 — 클릭 시 EnterCityBuildMode로 라우팅됨.
		SlotWidget->InitSlotForCity(SlotDA, City);
		BuildableContainer->AddChild(SlotWidget);
	}
}
