// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Buildings/BVBuildingBase.h"
#include "BVCityBase.generated.h"

class UBVCityData;
class ABVLane;
class ABVAutobotBase;
class UNiagaraComponent;
class UNiagaraSystem;
class UDecalComponent;
class UMaterialInterface;
class UMaterialInstanceDynamic;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCaptureProgressChanged, float, NewProgress);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCityDiscovered, ABVCityBase*, City);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRallyPointChanged, FVector, NewRallyPoint);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGarrisonChanged, int32, NewGarrisonCount);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDispatchTargetChanged, ABVCityBase*, NewTargetCity);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnDispatchFired, ABVCityBase*, FromCity, ABVCityBase*, ToCity);

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

	// ─── Discovery (안개 시야로 처음 발견됐는지) ──────────────
	// 처음 발견될 때만 공지/델리게이트 발사. true 이후엔 다시 false로 안 돌아감.
	UPROPERTY(BlueprintReadOnly, Category = "City|Discovery", meta = (HideInDetailPanel))
	bool bDiscoveredByPlayer = false;

	// 시작부터 발견된 상태로 둘 도시 (플레이어 본진 등). 공지는 안 띄움.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "City|Discovery")
	bool bStartDiscovered = false;

	// 처음 발견될 때 1회 브로드캐스트. UI 패널의 "알려진 거점 리스트"가 바인딩.
	UPROPERTY(BlueprintAssignable, Category = "City|Discovery")
	FOnCityDiscovered OnCityDiscovered;

	// 한 번만 효과 발생(공지 + 델리게이트). 중복 호출은 no-op이라 호출 측에서 가드 불필요.
	void MarkDiscoveredByPlayer();

	// ─── Build / Garrison (DA 미러) ────────────────────────
	UPROPERTY(BlueprintReadOnly, Category = "City|Build", meta = (HideInDetailPanel))
	float BuildRadius = 1500.f;

	UPROPERTY(BlueprintReadOnly, Category = "City|Build")
	TArray<TSubclassOf<ABVBuildingBase>> BuildableBuildings;

	UPROPERTY(BlueprintReadOnly, Category = "City|Garrison", meta = (HideInDetailPanel))
	int32 MaxGarrison = 20;

	UPROPERTY(BlueprintReadOnly, Category = "City|Garrison", meta = (HideInDetailPanel))
	int32 DispatchThreshold = 5;

	// 빌드 모드 진입/종료 시 PlayerController가 호출. 반경 데칼만 토글.
	void SetBuildRadiusVisualVisible(bool bVisible);

	// 도시 중심에서 BuildRadius 안에 들어오는지 (XY 평면 기준).
	UFUNCTION(BlueprintPure, Category = "City|Build")
	bool IsLocationWithinBuildRadius(const FVector& WorldLoc) const;

	// ─── Rally Point ──────────────────────────────────────────
	// 게리슨 유닛이 모일 도시 내 한 지점(월드 좌표). BeginPlay에서 도시 액터 위치로 초기화.
	// 항상 BuildRadius 안에 있어야 함 — SetRallyPoint가 클램프 처리.
	UPROPERTY(BlueprintReadOnly, Category = "City|Garrison", meta = (HideInDetailPanel))
	FVector RallyPoint = FVector::ZeroVector;

	// RallyPoint 변경 시(UI 패널이 위치를 옮길 때 등) 1회 브로드캐스트.
	UPROPERTY(BlueprintAssignable, Category = "City|Garrison")
	FOnRallyPointChanged OnRallyPointChanged;

	// RallyPoint를 새 좌표로 갱신. BuildRadius 밖이면 도시 중심에서 반경 경계로 클램프.
	// 변경되면 이미 게리슨에 있는 살아있는 유닛 전부에게 SetRallyDestination(NewPoint)을 호출해 재집결시킴.
	// 반환: 실제로 적용된(클램프된) 좌표.
	UFUNCTION(BlueprintCallable, Category = "City|Garrison")
	FVector SetRallyPoint(const FVector& NewWorldLoc);

	// ─── Garrison Pool ────────────────────────────────────────
	// 이 도시에서 스폰돼 도시에 귀속된 유닛 목록. 죽거나 만료된 약참조는 lazy prune.
	// 외부에서 직접 변형하지 말고 Add/Remove API 사용 권장.
	// (TArray<TWeakObjectPtr<>> 는 BP 노출 불가 — UPROPERTY는 GC 추적용으로만 유지, 접근은 BP API로.)
	UPROPERTY()
	TArray<TWeakObjectPtr<ABVAutobotBase>> Garrison;

	// 게리슨 인원수 변경 시 브로드캐스트 (UI 카운터 바인딩용).
	UPROPERTY(BlueprintAssignable, Category = "City|Garrison")
	FOnGarrisonChanged OnGarrisonChanged;

	// 살아있는 게리슨 인원수 (죽은/만료된 항목은 prune 후 카운트).
	UFUNCTION(BlueprintPure, Category = "City|Garrison")
	int32 GetGarrisonCount();

	// MaxGarrison 도달 여부 — 생산 일시정지 판정에 사용.
	UFUNCTION(BlueprintPure, Category = "City|Garrison")
	bool IsGarrisonFull() { return GetGarrisonCount() >= MaxGarrison; }

	// 외부에서(예: 도시간 이동 후 도착한 분견대) 게리슨 등록할 때 호출.
	// 반환값:
	//   true  = 등록됨 (혹은 이미 등록되어 있었음 — 멱등 처리)
	//   false = 거부됨 (Unit nullptr/dead, 팀 불일치, MaxGarrison 가득)
	// 거부 케이스에선 OnGarrisonChanged 브로드캐스트 안 함.
	UFUNCTION(BlueprintCallable, Category = "City|Garrison")
	bool AddToGarrison(ABVAutobotBase* Unit);

	// 출격/사망/이전 등으로 게리슨에서 빼낼 때 호출.
	UFUNCTION(BlueprintCallable, Category = "City|Garrison")
	void RemoveFromGarrison(ABVAutobotBase* Unit);

	// ─── Dispatch (도시간 출격) ──────────────────────────────
	// 이 도시에서 출격할 목표 도시. 비어 있으면 자동 출격 평가는 항상 실패.
	// 자기 자신을 가리키는 경우는 무시 (Set 단계에서 가드).
	UPROPERTY(BlueprintReadOnly, Category = "City|Dispatch", meta = (HideInDetailPanel))
	TWeakObjectPtr<ABVCityBase> DispatchTargetCity;

	// DispatchTargetCity 변경 시 1회 브로드캐스트 (UI 화살표/마커 갱신용). nullptr 가능.
	UPROPERTY(BlueprintAssignable, Category = "City|Dispatch")
	FOnDispatchTargetChanged OnDispatchTargetChanged;

	// 출격이 실제로 발사된 직후 1회 브로드캐스트 (FromCity, ToCity).
	// 게리슨이 비워지고 유닛들이 목표 도시로 이동을 시작한 직후 호출.
	UPROPERTY(BlueprintAssignable, Category = "City|Dispatch")
	FOnDispatchFired OnDispatchFired;

	// 출격 목표 도시를 지정. nullptr이나 자기 자신이면 ClearDispatchTargetCity와 동일.
	// 변경 후 즉시 자동 평가가 돌아 Threshold가 이미 충족되면 그 자리에서 발사된다.
	UFUNCTION(BlueprintCallable, Category = "City|Dispatch")
	void SetDispatchTargetCity(ABVCityBase* NewTargetCity);

	// 출격 목표 도시 해제 — 자동 출격이 멈춘다 (게리슨은 그대로 유지).
	UFUNCTION(BlueprintCallable, Category = "City|Dispatch")
	void ClearDispatchTargetCity();

	// 현재 출격 목표 도시 (없으면 nullptr).
	UFUNCTION(BlueprintPure, Category = "City|Dispatch")
	ABVCityBase* GetDispatchTargetCity() const { return DispatchTargetCity.Get(); }

	// 수동 출격 — Threshold 무시. 목표 도시가 유효하고 게리슨이 비어있지 않으면 즉시 전부 출격.
	// 출격 = 게리슨 유닛 모두에게 목표 도시 위치를 RallyDestination으로 지정 + 본 도시 게리슨에서 제거.
	// 유닛은 DispatchedToCity를 들고 가서 Step 5(도착 판정)에서 사용된다.
	UFUNCTION(BlueprintCallable, Category = "City|Dispatch")
	void Dispatch();

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

	// ─── Build Radius Visual ──────────────────────────────────
	// 빌드 모드 진입 시 도시 바닥에 표시되는 반경 데칼. 평소엔 숨김.
	// 사이즈는 BuildRadius * 2 (지름) 로 BeginPlay에서 한 번 세팅.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "City|Build")
	TObjectPtr<UDecalComponent> BuildRadiusDecal;

	// BuildRadiusDecal에 입혀진 MID. 머티리얼 파라미터 조정용 캐시 (현재는 단순 토글만).
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> BuildRadiusMID;

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

	// "거점 [name]을 발견했습니다." 공지를 PlayerController에 요청.
	void BroadcastDiscoveryAnnouncement() const;

	// City는 부모 lane-기반 스폰을 사용하지 않고, 도시 내 RallyPoint로 향하는 게리슨 유닛을 생성한다.
	// MaxGarrison 도달 시 즉시 return (생산 일시정지).
	virtual void SpawnUnit() override;

	// Garrison 배열에서 죽었거나 GC된 약참조를 정리. 변경 발생 시 OnGarrisonChanged 브로드캐스트.
	void PruneGarrison();

	// 자동 출격 평가: Threshold > 0 && 게리슨 수 >= Threshold && DispatchTargetCity 유효 → Dispatch().
	void EvaluateAutoDispatch();

	// OnGarrisonChanged 델리게이트 핸들러. EvaluateAutoDispatch에 위임.
	UFUNCTION()
	void HandleGarrisonChangedForAutoDispatch(int32 NewGarrisonCount);

	// BuildingData에 UBVCityData가 할당돼 있으면 그 포인터를 반환. 아니면 nullptr.
	const UBVCityData* GetCityData() const;

	// DA에서 읽어온 런타임 상태. BeginPlay에서 세팅됨.
	// 거점의 초기 소유 팀은 BuildingData->TeamType을 그대로 사용한다 (별도 필드 없음).
	UPROPERTY(BlueprintReadOnly, Category = "City", meta = (HideInDetailPanel))
	float PostCaptureInvulnSeconds = 0.f;

	// 풀 점령까지 필요한 누적 데미지 (DA에서 오버라이드). 0 이하는 금지.
	UPROPERTY(BlueprintReadOnly, Category = "City|Capture", meta = (HideInDetailPanel))
	float CaptureDamageTotal = 100.f;

	// 현재 점령 진행도. -1=Enemy 풀 점령, 0=중립, +1=Player 풀 점령.
	UPROPERTY(BlueprintReadOnly, Category = "City|Capture", meta = (HideInDetailPanel))
	float CaptureProgress = 0.f;
};
