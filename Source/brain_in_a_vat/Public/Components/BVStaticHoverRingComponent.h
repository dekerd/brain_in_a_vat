#pragma once

#include "CoreMinimal.h"
#include "Components/StaticMeshComponent.h"
#include "Headers/BVTeam.h"
#include "BVStaticHoverRingComponent.generated.h"

/**
 * 캐릭터/플레이어 발밑에 그려지는 hover/selection 링 — 평면 StaticMesh + Unlit Translucent material.
 * Deferred Decal이 TAA/TSR velocity buffer에 기여하지 않아 이동 시 잔상이 생기는 문제를 피하기 위한
 * 메시 기반 구현. 캐릭터 Transform에 1:1 attach되어 프레임 간 motion tracking 완벽.
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class BRAIN_IN_A_VAT_API UBVStaticHoverRingComponent : public UStaticMeshComponent
{
	GENERATED_BODY()

public:
	UBVStaticHoverRingComponent();

	virtual void BeginPlay() override;

	virtual void TickComponent(
		float DeltaTime,
		ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintCallable, Category = "Hover")
	void SetHovered(bool bInHovered, EBVTeam OwnerTeam);

public:
	// 링 머티리얼 — Material Domain = Surface, Shading Model = Unlit, Blend Mode = Translucent 필요.
	UPROPERTY(EditAnywhere, Category = "Hover")
	TSoftObjectPtr<UMaterialInterface> HoverRingMaterial;

	UPROPERTY(EditAnywhere, Category = "Hover")
	FName RingIntensityParamName = TEXT("Intensity");

	UPROPERTY(EditAnywhere, Category = "Hover")
	FName RingTintParamName = TEXT("Tint");

	// 머티리얼에 전달되는 링 두께 스칼라 파라미터 이름. 머티리얼 그래프에도 같은 이름의 Scalar Parameter가 있어야 반영됨.
	UPROPERTY(EditAnywhere, Category = "Hover")
	FName RingThicknessParamName = TEXT("Thickness");

	// 링 두께 (월드 cm). 런타임에 WorldDiameter로 나눠서 UV 비율로 변환 후 머티리얼에 전달.
	UPROPERTY(EditAnywhere, Category = "Hover", meta=(ClampMin="0.5", ClampMax="50"))
	float RingWorldThickness = 6.f;

	// 즉시 on/off 토글. true면 fade 없음.
	UPROPERTY(EditAnywhere, Category = "Hover")
	bool bInstantToggle = true;

	UPROPERTY(EditAnywhere, Category = "Hover", meta=(ClampMin="0.5", ClampMax="30", EditCondition="!bInstantToggle"))
	float HoverEaseSpeed = 10.f;

	UPROPERTY(EditAnywhere, Category = "Hover|Color")
	FLinearColor FriendlyColor = FLinearColor(0.2f, 1.f, 0.4f);

	UPROPERTY(EditAnywhere, Category = "Hover|Color")
	FLinearColor HostileColor = FLinearColor(1.f, 0.25f, 0.25f);

	UPROPERTY(EditAnywhere, Category = "Hover|Color")
	FLinearColor NeutralColor = FLinearColor(0.2f, 0.6f, 1.f);

	// Owner CapsuleComponent 반경에 맞춰 링 크기 자동 세팅.
	UPROPERTY(EditAnywhere, Category = "Hover|AutoSize")
	bool bAutoSizeFromCapsule = true;

	UPROPERTY(EditAnywhere, Category = "Hover|AutoSize", meta=(ClampMin="0.5", ClampMax="4.0"))
	float CapsulePadding = 1.3f;

	UPROPERTY(EditAnywhere, Category = "Hover|AutoSize", meta=(ClampMin="0.5", ClampMax="4.0"))
	float BoxPadding = 1.0f;

	// 지면 z-fighting 방지용 오프셋. 캡슐 바닥에서 이만큼 위로 띄움.
	UPROPERTY(EditAnywhere, Category = "Hover|AutoSize", meta=(ClampMin="0"))
	float GroundZOffset = 2.f;

protected:
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> HoverRingMID;

	float HoverTargetValue = 0.f;
	float HoverCurrentValue = 0.f;
	FLinearColor HoverTintColor = FLinearColor::White;

	// BeginPlay에서 계산된 월드 링 지름. 두께 UV 변환에 사용.
	float CachedWorldDiameter = 100.f;
};
