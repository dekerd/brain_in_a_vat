// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Buildings/BVBuildingBase.h"
#include "BVCityBase.generated.h"

class UBVCityData;
class ABVLane;
class UNiagaraComponent;
class UNiagaraSystem;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCaptureProgressChanged, float, NewProgress);

/**
 * 점령 가능한 거점(도시).
 * HP가 0이 되면 파괴되지 않고, 마지막으로 데미지를 입힌 팀으로 소유권이 이전되고 HP가 풀피로 회복된다.
 * Neutral 상태에선 유닛을 스폰하지 않는다.
 *
 * 세팅은 BuildingData 슬롯에 UBVCityData(또는 상속한 DA)를 할당해서 구성.
 * UBVBuildingData의 기본 필드(메시/스탯/SpawnUnit 등) + UBVCityData의 city-specific 필드 사용.
 */
UCLASS()
class BRAIN_IN_A_VAT_API ABVCityBase : public ABVBuildingBase
{
	GENERATED_BODY()

public:
	ABVCityBase();

	// 점령이 발생한 직후 블루프린트 쪽 훅(이펙트, 사운드 등).
	UFUNCTION(BlueprintImplementableEvent, Category = "City")
	void OnCaptured(EBVTeam NewOwner);

	// 점령 진행도 변경 시 브로드캐스트. -1(Enemy 풀 점령) ~ 0(중립) ~ +1(Player 풀 점령).
	// 위젯 쪽에서 바인딩해 색/길이 갱신.
	UPROPERTY(BlueprintAssignable, Category = "City|Capture")
	FOnCaptureProgressChanged OnCaptureProgressChanged;

	// 현재 점령 진행도. -1=Enemy, 0=중립, +1=Player.
	UFUNCTION(BlueprintPure, Category = "City|Capture")
	float GetCaptureProgress() const { return CaptureProgress; }

	// 팀 소유에 따라 머티리얼의 EmissionColor 벡터 파라미터를 바꿀 색.
	// HDR 허용 (예: FLinearColor(0.f, 0.3f, 2.f)로 bloom 강조).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "City|Emission")
	FLinearColor PlayerEmissionColor = FLinearColor(0.1f, 0.4f, 2.f, 1.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "City|Emission")
	FLinearColor EnemyEmissionColor = FLinearColor(2.f, 0.2f, 0.2f, 1.f);

	// 중립(미점령) 상태에서는 emission을 완전히 끔.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "City|Emission")
	FLinearColor NeutralEmissionColor = FLinearColor(0.f, 0.f, 0.f, 1.f);

	// 점령자가 Player일 때 스폰 유닛이 따라갈 레인 (보통 Enemy 베이스로 향함).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "City|Spawn Unit")
	TObjectPtr<ABVLane> PlayerTeamLane;

	// 점령자가 Enemy일 때 스폰 유닛이 따라갈 레인 (보통 Player 베이스로 향함).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "City|Spawn Unit")
	TObjectPtr<ABVLane> EnemyTeamLane;

	// ─── Bottom VFX (런타임 캐시, DA에서 로드) ───
	// 바닥에 붙어 재생되는 Niagara 컴포넌트. Asset과 Scale은 DA에서 제공.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "City|VFX")
	TObjectPtr<UNiagaraComponent> BottomVFXComponent;

	UPROPERTY(BlueprintReadOnly, Category = "City|VFX", meta = (HideInDetailPanel))
	TObjectPtr<UNiagaraSystem> PlayerCaptureBottomVFX;

	UPROPERTY(BlueprintReadOnly, Category = "City|VFX", meta = (HideInDetailPanel))
	TObjectPtr<UNiagaraSystem> EnemyCaptureBottomVFX;

	UPROPERTY(BlueprintReadOnly, Category = "City|VFX", meta = (HideInDetailPanel))
	float BottomVFXScale = 1.f;

	UPROPERTY(BlueprintReadOnly, Category = "City|VFX", meta = (HideInDetailPanel))
	FVector BottomVFXOffset = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "City|VFX", meta = (HideInDetailPanel))
	float BottomVFXDuration = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "City|VFX", meta = (HideInDetailPanel))
	float BottomVFXPlayRate = 1.f;

	// VFX 자동 정지용 타이머 핸들.
	FTimerHandle BottomVFXStopTimerHandle;

protected:
	virtual void BeginPlay() override;

	// 파괴 대신 풀피 복구만 수행. 점령은 CaptureProgress가 담당.
	virtual void HandleHealthDepleted() override;

	// 데미지 수신 시 점령 진행도에 반영.
	virtual void HandleDamageReceived(const AActor* Attacker, float DamageAmount) override;

	// 가해자 팀과 데미지량을 받아 진행도를 갱신하고, 풀 점령(±1) 도달 시 팀 전환.
	void ApplyCaptureDelta(EBVTeam AttackerTeam, float DamageAmount);

	// 현재 팀에 맞게 스폰 타이머를 재설정 (Neutral이면 정지).
	void ResetSpawnTimerForCurrentTeam();

	// 직접 Health 어트리뷰트를 MaxHealth 값으로 되돌린다.
	void RefillHealthToMax();

	// 현재 TeamType에 맞는 Emission 색을 머티리얼에 적용.
	void UpdateEmissionColorForCurrentTeam();

	// 현재 TeamType에 맞는 바닥 VFX(Niagara) 에셋/스케일을 컴포넌트에 반영.
	void UpdateBottomVFXForCurrentTeam();

	// 팀 전환 직후, 화면 중앙 점령 메시지를 PlayerController에 요청.
	// PrevTeam/NewTeam 조합으로 "점령/수복/탈환" 문구 자동 선택.
	void BroadcastCaptureAnnouncement(EBVTeam PrevTeam, EBVTeam NewTeam) const;

	// 스폰 시점의 팀에 맞는 레인을 AssignedLane에 세팅 후 부모 구현 호출.
	virtual void SpawnUnit() override;

	// BuildingData에 UBVCityData가 할당돼 있으면 그 포인터를 반환. 아니면 nullptr.
	const UBVCityData* GetCityData() const;

	// DA에서 읽어온 런타임 상태. BeginPlay에서 세팅됨.
	// 기본값은 DA가 없을 때의 fallback (기존 동작 유지).
	UPROPERTY(BlueprintReadOnly, Category = "City", meta = (HideInDetailPanel))
	bool bStartsNeutral = true;

	UPROPERTY(BlueprintReadOnly, Category = "City", meta = (HideInDetailPanel))
	float PostCaptureInvulnSeconds = 0.f;

	// 풀 점령까지 필요한 누적 데미지 (DA에서 오버라이드). 0 이하는 금지.
	UPROPERTY(BlueprintReadOnly, Category = "City|Capture", meta = (HideInDetailPanel))
	float CaptureDamageTotal = 100.f;

	// 현재 점령 진행도. -1=Enemy 풀 점령, 0=중립, +1=Player 풀 점령.
	UPROPERTY(BlueprintReadOnly, Category = "City|Capture", meta = (HideInDetailPanel))
	float CaptureProgress = 0.f;
};
