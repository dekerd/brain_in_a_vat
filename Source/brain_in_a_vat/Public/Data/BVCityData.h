#pragma once

#include "CoreMinimal.h"
#include "Data/BVBuildingData.h"
#include "BVCityData.generated.h"

class UNiagaraSystem;

/**
 * ABVCityBase 전용 DataAsset.
 * UBVBuildingData의 모든 필드(메시, 스탯, 스폰 유닛 등)를 그대로 쓰고,
 * city-specific한 세팅만 여기에 추가한다.
 */
UCLASS(BlueprintType)
class BRAIN_IN_A_VAT_API UBVCityData : public UBVBuildingData
{
	GENERATED_BODY()

public:
	// true면 레벨 시작 시 Building DA의 TeamType을 무시하고 Neutral로 강제 시작.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "City")
	bool bStartsNeutral = true;

	// 점령 후 무적 시간(초). 0 이하면 무적 없음 (연타 재점령 방지용 여유).
	// NOTE: 로직은 아직 미구현. DA 슬롯만 먼저 준비.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "City", meta = (ClampMin = "0.0"))
	float PostCaptureInvulnSeconds = 0.f;

	// NOTE: 점령 진행도 풀 점령(±1)까지 필요한 누적 데미지 총량은
	// 상속받은 UBVBuildingData.MaxHealth 값을 그대로 사용한다.
	// (체력 = 점령 저항력). 별도 오버라이드가 필요해지면 여기에 필드 추가.

	// ─── Emission Colors ──────────────────────────────────────
	// 머티리얼의 EmissionColor 벡터 파라미터에 주입되는 HDR 색.
	// HDR 허용(값 > 1 가능, bloom으로 번짐 강조).

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "City|Emission")
	FLinearColor PlayerEmissionColor = FLinearColor(0.1f, 0.4f, 2.f, 1.f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "City|Emission")
	FLinearColor EnemyEmissionColor = FLinearColor(2.f, 0.2f, 0.2f, 1.f);

	// 중립(미점령) 상태 emission. 기본 검정 = 소등.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "City|Emission")
	FLinearColor NeutralEmissionColor = FLinearColor(0.f, 0.f, 0.f, 1.f);

	// ─── Bottom VFX ──────────────────────────────────────────
	// 거점 바닥에서 지속적으로 재생되는 Niagara 이펙트 (팀 식별/분위기용).
	// nullptr로 두면 해당 팀 소유 시 VFX 없음.

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "City|VFX")
	TObjectPtr<UNiagaraSystem> PlayerCaptureBottomVFX;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "City|VFX")
	TObjectPtr<UNiagaraSystem> EnemyCaptureBottomVFX;

	// 바닥 VFX의 균일 스케일 (거점 크기에 맞춰 조정). 단일 배수 값.
	// NOTE: 파티클 내부 크기가 고정된 Niagara 시스템은 이 값을 무시할 수 있음.
	// 그런 경우 Niagara Asset에 User Float "Scale" 파라미터를 추가하고
	// 거기에도 이 값을 주입하는 방법을 쓰면 됨.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "City|VFX", meta = (ClampMin = "0.01"))
	float BottomVFXScale = 1.f;

	// 바닥 VFX의 위치 미세 조정 오프셋 (메시 바닥 중앙 기준).
	// Z를 약간 올리면 지면 위로 뜨고, Y/X로 앞뒤/좌우 조정 가능.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "City|VFX")
	FVector BottomVFXOffset = FVector::ZeroVector;

	// 바닥 VFX 지속 시간(초). 0 이하면 무한(점령 유지 동안 계속 재생).
	// 0보다 크면 점령 순간/팀 전환 시 해당 초만큼 재생하고 자동 정지.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "City|VFX", meta = (ClampMin = "0.0"))
	float BottomVFXDuration = 0.f;

	// 바닥 VFX 재생 속도 배수. 1.0=원속, 2.0=2배 빠름, 0.5=절반 속도.
	// Niagara 컴포넌트의 CustomTimeDilation로 전달.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "City|VFX", meta = (ClampMin = "0.01"))
	float BottomVFXPlayRate = 1.f;
};
