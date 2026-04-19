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

	// --- Visual ---
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual")
	TObjectPtr<UStaticMesh> ProjectileMesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual", meta = (ClampMin = "0.01"))
	float MeshScale = 1.f;

	// 투사체 뒤에 꼬리로 붙는 나이아가라 (선택)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual")
	TObjectPtr<class UNiagaraSystem> TrailEffect;

	// 충돌 시 재생되는 Niagara 이펙트 (우선). 할당되어 있으면 Cascade는 무시됨.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual")
	TObjectPtr<class UNiagaraSystem> HitNiagaraEffect;

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
};
