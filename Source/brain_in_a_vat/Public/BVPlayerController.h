// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GenericTeamAgentInterface.h"
#include "InputActionValue.h"
#include "GameFramework/PlayerController.h"
#include "Headers/BVAnnouncerEvent.h"
#include "BVPlayerController.generated.h"

class ABVNPCBase;
class ABVBuildingGhost;
class ABVBuildingBase;
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSelectionChanged, AActor*, NewSelectedActor);

/**
 * 
 */
UCLASS()
class BRAIN_IN_A_VAT_API ABVPlayerController : public APlayerController, public IGenericTeamAgentInterface
{
	GENERATED_BODY()

public:
	virtual FGenericTeamId GetGenericTeamId() const override { return TeamID; }
	virtual void SetGenericTeamId(const FGenericTeamId& InTeamID) override { TeamID = InTeamID; }

	virtual ETeamAttitude::Type GetTeamAttitudeTowards(const AActor& Other) const override;

	ABVPlayerController();
	
protected:
	virtual void BeginPlay() override;

	virtual void PlayerTick(float DeltaTime) override;

// Input Setting
protected:

	virtual void SetupInputComponent() override;

	// 1. Basic IMC
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	TObjectPtr<class UInputMappingContext> InputMappingContext;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, Meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class UInputAction> MoveAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, Meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class UInputAction> SelectAction;
	
	UPROPERTY(BlueprintReadOnly, Category = "Selection")
	TObjectPtr<AActor> SelectedActor;

	UPROPERTY(BlueprintAssignable, Category = "Selection")
	FOnSelectionChanged OnSelectionChanged;

	void MoveToLocation(const FInputActionValue& Value);
	void SelectObject();

	// 2. IMC for building

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	TObjectPtr<class UInputMappingContext> BuildMappingContext;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, Meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class UInputAction> BuildClickAction;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, Meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class UInputAction> BuildAction;
	
	void OnBuildClick();   
	void OnBuildKeyPressed();

	// 3. Inputs for UIs

public:

	UFUNCTION(BlueprintCallable, Category = "UI")
	void ToggleInventoryUI();

	UFUNCTION(BlueprintCallable, Category = "UI")
	void ToggleConstructionMenuUI();

	UFUNCTION(BlueprintCallable, Category = "UI")
	void CloseCurrentUI();

protected:

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<class UInputAction> InventoryAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<class UInputAction> CloseUIAction;

	// Tab 키 등에 바인드: 모든 유닛/건물의 머리 위 위젯을 한 번에 끄고 켠다
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<class UInputAction> ToggleOverheadWidgetsAction;

public:
	UFUNCTION(BlueprintCallable, Category = "UI")
	void ToggleOverheadWidgets();

	// 새로 스폰되는 액터가 현재 토글 상태를 조회할 수 있도록 노출
	UFUNCTION(BlueprintPure, Category = "UI")
	bool AreOverheadWidgetsVisible() const { return bOverheadWidgetsVisible; }

protected:
	// 전체 오버헤드 위젯 표시 여부 (Tab 토글용)
	UPROPERTY(BlueprintReadOnly, Category = "UI")
	bool bOverheadWidgetsVisible = true;
	
	UPROPERTY()
	TObjectPtr<class UUserWidget> InventoryWidget;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI")
	TSubclassOf<class UUserWidget> InventoryWidgetClass;

	UPROPERTY()
	TObjectPtr<class UUserWidget> ConstructionMenuWidget;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI")
	TSubclassOf<class UUserWidget> ConstructionMenuWidgetClass;

	// Player Team setting
private:
	FGenericTeamId TeamID = FGenericTeamId(1);

// Camera Moving
public:
	UPROPERTY(BlueprintReadWrite, Category = "Player")
	TObjectPtr<class AMainCharacter> HeroCharacter;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	TObjectPtr<class UInputAction> CameraPanAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	TObjectPtr<class UInputAction> CameraCenterAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	TObjectPtr<class UInputAction> CameraZoomAction;

	// Space+1: 레인에서 가장 마지막 전투 현장을 보여준다. (팔로우 해제)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	TObjectPtr<class UInputAction> FocusLastCombatAction;

	UPROPERTY()
	bool bIsWaitingForArrival = false;

	UPROPERTY()
	FVector TargetMoveLocation;

	// 줌 함수
	void OnCameraZoom(const FInputActionValue& Value);
	void OnCameraPan(const FInputActionValue& Value);
	void OnCameraCenterPressed();
	void OnFocusLastCombatPressed();

public:
	// 전투 발생 시 유닛/타워가 호출. 가장 최근 전투 좌표를 기록한다.
	UFUNCTION(BlueprintCallable, Category = "Camera|Combat")
	void ReportCombatLocation(const FVector& WorldLocation);

	UFUNCTION(BlueprintPure, Category = "Camera|Combat")
	bool HasLastCombatLocation() const { return bHasLastCombatLocation; }

protected:
	UPROPERTY()
	FVector LastCombatLocation = FVector::ZeroVector;

	UPROPERTY()
	bool bHasLastCombatLocation = false;

// Fog of War
public:
	// --- 전장의 안개(Fog of War) 관련 변수 ---
	UPROPERTY(EditDefaultsOnly, Category = "FogOfWar")
	UTextureRenderTarget2D* RT_Discovered;

	UPROPERTY(EditDefaultsOnly, Category = "FogOfWar")
	UTextureRenderTarget2D* RT_Vision;

	UPROPERTY(EditDefaultsOnly, Category = "FogOfWar")
	UMaterialInterface* VisionBrushMaterial;

	// 맵의 실제 크기 (월드 좌표 기준). 맵 크기에 맞게 조절해야 합니다.
	UPROPERTY(EditDefaultsOnly, Category = "FogOfWar")
	float MapSize = 10000.0f; 

	// 맵의 중앙 좌표
	UPROPERTY(EditDefaultsOnly, Category = "FogOfWar")
	FVector2D MapCenter = FVector2D(0.0f, 0.0f);

	// 시야 반경
	UPROPERTY(EditDefaultsOnly, Category = "FogOfWar")
	float VisionRadius = 1500.0f;

protected:
	FTimerHandle FogTimerHandle;

private:
	void UpdateFogOfWar();

// Construction Mode
public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Build")
	TArray<TSubclassOf<class ABVBuildingBase>> AvailableBuildings;
	
	UFUNCTION(BlueprintCallable, Category = "Build")
	void EnterConstructionMode(TSubclassOf<ABVBuildingBase> InBuildingClass, float InConstructionTime);

protected:
	UPROPERTY()
	TSubclassOf<ABVBuildingBase> CurrentBuildingClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Build")
	TSubclassOf<class ABVBuildingBase> DefaultBuildingClass;

	UPROPERTY(EditAnywhere, Category = "Build")
	TSubclassOf<ABVBuildingGhost> GhostActorClass;

	UPROPERTY(EditAnywhere, Category = "Build")
	TObjectPtr<UMaterialInterface> GhostMaterialBase;

	UPROPERTY()
	TObjectPtr<ABVBuildingGhost> CurrentGhostActor;

	UPROPERTY()
	TSubclassOf<ABVBuildingBase> PendingBuildingClass;

	UPROPERTY()
	float PendingConstructionTime;

	float StartToBuildRange = 100.0f;
	FVector TargetBuildLocation;

	bool bIsMovingToBuild = false;
	bool bIsConstructionMode = false; 
	bool bCanBuild = false;
	
	void ExitConstructionMode();
	void UpdateGhostLocation();
	
// Mouse Hovering
protected:
	UPROPERTY()
	TObjectPtr<class AActor> HoveredObject;

	UPROPERTY(EditAnywhere, Category = "Audio")
	TObjectPtr<class USoundBase> HoverSound;

	UPROPERTY(EditAnywhere, Category = "Audio")
	float HoverSoundVolume = 0.5f;

// Announce Sound
public:
	UFUNCTION(BlueprintCallable, Category = "Audio|Announcer")
	void PlayAnnouncerVoice(EBVAnnouncerEvent EventType);

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Audio|Announcer")
	TMap<EBVAnnouncerEvent, TObjectPtr<class USoundBase>> AnnouncerVoices;
	
	float LastBaseAttackVoiceTime = -100.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Audio|Announcer")
	float BaseAttackVoiceCooldown = 10.0f;

	// 이벤트별 쿨다운 (초). 여기 등록 안 된 이벤트는 쿨다운 없이 재생됨.
	UPROPERTY(EditDefaultsOnly, Category = "Audio|Announcer")
	TMap<EBVAnnouncerEvent, float> AnnouncerCooldowns;

	// 이벤트별 마지막 재생 시각 (런타임에 갱신)
	TMap<EBVAnnouncerEvent, float> LastAnnouncerPlayTime;

	UPROPERTY(EditDefaultsOnly, Category = "Audio|Announcer")
	float AnnouncerVolumeMultiplier = 2.0f;

// <--------------- Widgets --------------->
// Main HUD Widget
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI")
	TSubclassOf<class UUserWidget> MainHUDWidgetClass;
	
	UPROPERTY()
	TObjectPtr<class UUserWidget> MainHUDWidget;
	
// Gold Popup Widget
public:
	void ShowGoldReward(int32 Amount, FVector WorldLocation);

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI")
	TSubclassOf<class UUserWidget> GoldPopupWidgetClass;

// Building Detail Widget
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI")
	TSubclassOf<class UUserWidget> BuildingDetailWidgetClass;

	UPROPERTY()
	TObjectPtr<class UUserWidget> BuildingDetailWidget;

	UFUNCTION(BlueprintCallable, Category = "UI")
	void ShowBuildingDetail(ABVBuildingBase* InBuilding);

	UFUNCTION(BlueprintCallable, Category = "UI")
	void HideBuildingDetail();

protected:
	// 현재 상세 패널이 표시 중인 건물 (약한 참조)
	TWeakObjectPtr<ABVBuildingBase> DetailBuilding;

// Unit Detail Widget
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI")
	TSubclassOf<class UUserWidget> UnitDetailWidgetClass;

	UPROPERTY()
	TObjectPtr<class UUserWidget> UnitDetailWidget;

	UFUNCTION(BlueprintCallable, Category = "UI")
	void ShowUnitDetail(class ABVAutobotBase* InUnit);

	UFUNCTION(BlueprintCallable, Category = "UI")
	void HideUnitDetail();

protected:
	TWeakObjectPtr<class ABVAutobotBase> DetailUnit;

// NPC Interaction Widget
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI")
	TSubclassOf<class UUserWidget> ShopWidgetClass;

	UPROPERTY()
	TObjectPtr<class UUserWidget> ShopWidget;

	UFUNCTION(BlueprintCallable, Category = "UI")
	void OpenShopUI(ABVNPCBase* TargetNPC);

	UFUNCTION(BlueprintCallable, Category = "UI")
	void CloseShopUI();

	
// Background Music
public:

protected:
	UPROPERTY(EditAnywhere, Category = "Audio")
	TArray<TObjectPtr<class USoundBase>> BGMPlaylist;

	UPROPERTY(EditAnywhere, Category = "Audio")
	float Volume = 1.0f;
	
	UPROPERTY()
	TObjectPtr<class UAudioComponent> BGMComponent;

// Sound Effects
public:

protected:

	UPROPERTY(EditAnywhere, Category = "Audio")
	TObjectPtr<class USoundBase> GoldPickupSound;

	UPROPERTY(EditAnywhere, Category = "Audio")
	float GoldPickupSoundVolume = 1.0f;
	
};
