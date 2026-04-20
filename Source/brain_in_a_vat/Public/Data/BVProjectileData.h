#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "BVProjectileData.generated.h"

UENUM(BlueprintType)
enum class EBVProjectileTrajectory : uint8
{
	Straight UMETA(DisplayName = "Straight (일직선)"),
	Arc      UMETA(DisplayName = "Arc (포물선)")
};

/**
 * 투사체 설정 데이터 에셋.
 * 메시 / 사운드 / VFX / 궤적 타입 / 속도 / 데미지 / 수명 등을 한 군데서 관리한다.
 * 이 DA 하나로 기존 BP 변형 여러 개를 대체할 수 있다.
 */
UCLASS(BlueprintType)
class BRAIN_IN_A_VAT_API UBVProjectileData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	// --- Info ---
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Info")
	FName ProjectileName;

	// 무기(투사체) 표시용 이름 — 유닛 상세 패널 등에 사용
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Info")
	FText WeaponName;

	// 무기(투사체) 아이콘 — 유닛 상세 패널 등에 사용
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Info")
	TObjectPtr<UTexture2D> WeaponIcon;

	// --- Visual ---
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual")
	TObjectPtr<UStaticMesh> ProjectileMesh;

	// 메시 균일 스케일 (전체 축에 동일 적용).
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual", meta = (ClampMin = "0.01"))
	float MeshScale = 1.f;

	// 메시 로컬 회전 오프셋. 모델 축이 +X 전방이 아닐 때 이걸로 보정.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual")
	FRotator MeshRotation = FRotator::ZeroRotator;

	// 메시 로컬 위치 오프셋. 콜리전 중심과 메시 피벗이 맞지 않을 때 보정.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual")
	FVector MeshLocationOffset = FVector::ZeroVector;

	// 투사체 뒤에 꼬리로 붙는 나이아가라 (선택)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual")
	TObjectPtr<class UNiagaraSystem> TrailEffect;

	// Trail 이펙트의 로컬 회전 오프셋. 이펙트의 forward 축이 투사체 진행 방향과 맞지 않을 때 보정.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual")
	FRotator TrailRotation = FRotator::ZeroRotator;

	// Trail 이펙트의 로컬 위치 오프셋.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual")
	FVector TrailLocationOffset = FVector::ZeroVector;

	// 충돌 시 재생되는 Niagara 이펙트 (우선). 할당되어 있으면 Cascade는 무시됨.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual")
	TObjectPtr<class UNiagaraSystem> HitNiagaraEffect;

	// HitNiagaraEffect 재생 속도 배수 (1.0 = 원본, 2.0 = 2배속, 0.5 = 절반속도).
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual", meta = (ClampMin = "0.01"))
	float HitNiagaraPlayRate = 1.f;

	// 충돌 시 재생되는 Cascade 파티클 (Niagara가 없을 때 폴백)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual")
	TObjectPtr<class UParticleSystem> HitEffect;

	// --- Audio ---
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Audio")
	TObjectPtr<class USoundBase> FireSound;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Audio")
	float FireSoundVolume = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Audio")
	TObjectPtr<class USoundBase> HitSound;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Audio")
	float HitSoundVolume = 1.f;

	// --- Trajectory ---
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Trajectory")
	EBVProjectileTrajectory TrajectoryType = EBVProjectileTrajectory::Straight;

	// cm/s. 직선일 때는 이 값으로 날아감. 포물선일 때도 계산 실패 시 폴백.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Trajectory")
	float ProjectileSpeed = 2500.f;

	// 포물선 세기 (0: 직선에 가까움 / 1: 가까운 아치). SuggestProjectileVelocity_CustomArc의 ArcParam.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Trajectory", meta = (ClampMin = "0.0", ClampMax = "1.0", EditCondition = "TrajectoryType == EBVProjectileTrajectory::Arc"))
	float ArcValue = 0.5f;

	// --- Gameplay ---
	// 0 이하면 발사자의 Damage 스탯을 그대로 사용
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gameplay")
	float DamageOverride = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gameplay")
	float Lifespan = 5.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gameplay")
	float CollisionRadius = 10.f;

	// 투사체 사거리(cm). 이 범위 안의 적에게만 발사하고, 이 거리 이상 날아가면 자동 폭발.
	// 0 이하면 제한 없음.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gameplay")
	float ProjectileRange = 0.f;

	// 발사 간격(초). 플레이어 무기 쿨타임 계산에 사용.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gameplay", meta = (ClampMin = "0.01"))
	float FireInterval = 1.f;
};
