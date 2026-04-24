#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "BVHoverRingSettings.generated.h"

class UMaterialInterface;

/**
 * Hover / Selection Ring 전역 설정.
 * Project Settings → "Brain in a Vat" → "Hover Ring"에서 편집 가능.
 * 이 값들은 BeginPlay 시 UBVStaticHoverRingComponent에 적용되어 모든 캐릭터/유닛/건물에 일괄 반영됨.
 */
UCLASS(config=Game, defaultconfig, meta=(DisplayName="Hover Ring"))
class BRAIN_IN_A_VAT_API UBVHoverRingSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	virtual FName GetCategoryName() const override { return TEXT("Brain in a Vat"); }

	// 지면에 그려질 링 머티리얼 (Surface / Unlit / Translucent 또는 Masked).
	UPROPERTY(EditAnywhere, config, Category = "Material")
	TSoftObjectPtr<UMaterialInterface> HoverRingMaterial;

	// 머티리얼 스칼라 파라미터 이름들.
	UPROPERTY(EditAnywhere, config, Category = "Material|Param Names")
	FName RingIntensityParamName = TEXT("Intensity");

	UPROPERTY(EditAnywhere, config, Category = "Material|Param Names")
	FName RingTintParamName = TEXT("Tint");

	UPROPERTY(EditAnywhere, config, Category = "Material|Param Names")
	FName RingThicknessParamName = TEXT("Thickness");

	// 링 두께를 월드 cm 단위로. 컴포넌트가 WorldDiameter로 나눠서 UV 비율로 변환해 머티리얼에 전달.
	// 크기가 다른 캐릭터/건물이 모두 동일한 월드 두께로 보이게 된다.
	UPROPERTY(EditAnywhere, config, Category = "Material", meta=(ClampMin="0.5", ClampMax="50"))
	float RingWorldThickness = 6.f;

	// 즉시 on/off 토글. true면 fade 없이 딱 뜨고 딱 사라짐.
	UPROPERTY(EditAnywhere, config, Category = "Animation")
	bool bInstantToggle = true;

	// Hover fade 속도 (bInstantToggle=false일 때만 사용).
	UPROPERTY(EditAnywhere, config, Category = "Animation", meta=(ClampMin="0.5", ClampMax="30", EditCondition="!bInstantToggle"))
	float HoverEaseSpeed = 10.f;

	// 아군 팀 색.
	UPROPERTY(EditAnywhere, config, Category = "Color")
	FLinearColor FriendlyColor = FLinearColor(0.2f, 1.f, 0.4f);

	// 적 팀 색.
	UPROPERTY(EditAnywhere, config, Category = "Color")
	FLinearColor HostileColor = FLinearColor(1.f, 0.25f, 0.25f);

	// 중립 색.
	UPROPERTY(EditAnywhere, config, Category = "Color")
	FLinearColor NeutralColor = FLinearColor(0.2f, 0.6f, 1.f);

	// Capsule(유닛/영웅) 기반 크기 배율. 1.0이면 캡슐 외곽에 딱.
	UPROPERTY(EditAnywhere, config, Category = "Size", meta=(ClampMin="0.5", ClampMax="4.0"))
	float CapsulePadding = 1.3f;

	// Box(건물/거점) 기반 크기 배율. 1.0이면 박스 외곽에 딱.
	UPROPERTY(EditAnywhere, config, Category = "Size", meta=(ClampMin="0.5", ClampMax="4.0"))
	float BoxPadding = 1.0f;

	// 지면 z-fighting 방지 오프셋. 캡슐 바닥에서 이만큼 위로 띄움.
	UPROPERTY(EditAnywhere, config, Category = "Size", meta=(ClampMin="0"))
	float GroundZOffset = 2.f;
};
