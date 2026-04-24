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
class ABVCityBase;
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

	// ContextCity가 nullptr이면 글로벌 Hero 빌드 카탈로그(Player 빌딩 DA 목록)를 표시하고,
	// 지정되면 해당 도시의 BuildableBuildings로 슬롯을 채워 도시-컨텍스트 빌드 흐름을 탄다.
	UFUNCTION(BlueprintCallable, Category = "UI")
	void ToggleConstructionMenuUI(ABVCityBase* ContextCity = nullptr);

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

	// Hero 강제 선택 (기본 키: Space). HeroCharacter를 즉시 SelectedHero로 세팅해 발밑 링을 켠다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	TObjectPtr<class UInputAction> SelectHeroAction;

	// 내가 소유한 첫 번째 거점으로 카메라 Jump (기본 키: 1).
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	TObjectPtr<class UInputAction> FocusFirstCityAction;

	UPROPERTY()
	bool bIsWaitingForArrival = false;

	UPROPERTY()
	FVector TargetMoveLocation;

	// 줌 함수
	void OnCameraZoom(const FInputActionValue& Value);
	void OnCameraPan(const FInputActionValue& Value);
	void OnCameraCenterPressed();
	void OnFocusLastCombatPressed();
	void OnSelectHeroPressed();
	void OnFocusFirstCityPressed();

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

	// 도시 패널의 "건설" 버튼이 호출. 도시 반경 안에서만 짓고 Hero 이동/도착 절차를 생략하는
	// 즉시-건설 경로. 내부적으로 EnterConstructionMode를 재사용해 ghost/IMC 셋업을 공유한다.
	// City가 nullptr이면 일반 EnterConstructionMode와 동치.
	UFUNCTION(BlueprintCallable, Category = "Build")
	void EnterCityBuildMode(ABVCityBase* City, TSubclassOf<ABVBuildingBase> InBuildingClass, float InConstructionTime);

protected:
	UPROPERTY()
	TSubclassOf<ABVBuildingBase> CurrentBuildingClass;

	// 현재 빌드 모드를 켠 도시. 일반 빌드 모드(Hero가 걸어가서 짓는 경로)에선 비어있음.
	// 비어있으면: 기존 동작(Hero가 이동 후 건설). 유효하면: 도시 즉시-건설 + 반경 내 제한.
	TWeakObjectPtr<ABVCityBase> CurrentBuildCity;

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

	FVector TargetBuildLocation;

	bool bIsConstructionMode = false;
	bool bCanBuild = false;

	// 건설 모드 진입 시각. 슬롯 좌클릭의 "마우스 업" 이벤트가 새로 활성화된 Build IMC로 흘러들어
	// 즉시 착공되는 유령 입력을 짧은 시간 창(BuildClickInputIgnoreWindow)으로 차단.
	// 사용자의 실제 다음 클릭은 수백 ms 이후이므로 통과한다.
	double BuildModeEnterTime = -1.0;
	static constexpr double BuildClickInputIgnoreWindow = 0.10; // seconds

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

// Box Selection (drag-select)
public:
	bool IsBoxSelectActive() const { return bIsDragSelecting && bDragMovedBeyondThreshold; }
	FVector2D GetBoxSelectStart() const { return DragStartScreenPos; }
	FVector2D GetBoxSelectCurrent() const { return DragCurrentScreenPos; }

protected:
	// 드래그 셀렉션 입력 핸들러
	void OnSelectPressed();
	void OnSelectReleased();

	// 박스 내부의 유닛/건물 호버 상태 업데이트
	void UpdateBoxHover();

	// 박스 셀렉션 리셋 — 현재 박스로 호버된 액터들을 모두 un-hover
	void ClearBoxSelection();

	// 박스 셀렉션 상태
	UPROPERTY()
	bool bIsDragSelecting = false;

	UPROPERTY()
	bool bDragMovedBeyondThreshold = false;

	FVector2D DragStartScreenPos = FVector2D::ZeroVector;
	FVector2D DragCurrentScreenPos = FVector2D::ZeroVector;

	// 클릭 vs 드래그 구분을 위한 픽셀 임계치
	UPROPERTY(EditDefaultsOnly, Category = "Selection")
	float DragThresholdPixels = 6.f;

	// 현재 박스에 들어있어서 호버 중인 액터들 (드래그 종료 후에도 hover 유지)
	UPROPERTY()
	TArray<TWeakObjectPtr<AActor>> BoxSelectedActors;

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

	// 거점(City) 전용 상세 패널 클래스 — fallback. BuildingDetail과는 별개.
	// 우선순위: 도시 DA(UBVCityData)의 CityDetailWidgetClass → 비어있으면 이 값.
	// 모든 도시가 같은 패널을 쓰면 여기만 채워두면 됨. 도시별로 다르게 쓸 땐 DA에서 오버라이드.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI")
	TSubclassOf<class UUserWidget> CityDetailWidgetClass;

	UPROPERTY()
	TObjectPtr<class UUserWidget> CityDetailWidget;

	UFUNCTION(BlueprintCallable, Category = "UI")
	void ShowBuildingDetail(ABVBuildingBase* InBuilding);

	UFUNCTION(BlueprintCallable, Category = "UI")
	void HideBuildingDetail();

protected:
	// 현재 상세 패널이 표시 중인 건물 (약한 참조)
	TWeakObjectPtr<ABVBuildingBase> DetailBuilding;

	// 현재 활성화된 상세 위젯(BuildingDetailWidget 또는 CityDetailWidget).
	// Tick에서 위치/프로그레스 업데이트 시 이걸 참조.
	UPROPERTY()
	TObjectPtr<class UUserWidget> ActiveDetailWidget;

// Capture Announcement (거점 점령 화면 중앙 메시지)
public:
	// WBP_CaptureAnnouncement 같은, UBVCaptureAnnouncementWidget 파생 클래스 지정.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI")
	TSubclassOf<class UUserWidget> CaptureAnnouncementWidgetClass;

	UPROPERTY()
	TObjectPtr<class UUserWidget> CaptureAnnouncementWidget;

	// 메시지 표시 지속 시간(초). 이 시간 후 페이드아웃 → 숨김.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI", meta = (ClampMin = "0.1"))
	float CaptureAnnouncementDuration = 3.0f;

	// 중앙에 메시지를 띄운다. 3초 후 자동 숨김.
	UFUNCTION(BlueprintCallable, Category = "UI")
	void ShowCaptureAnnouncement(const FText& Message);

protected:
	FTimerHandle CaptureAnnouncementHideTimerHandle;

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

// Hero selection (ring under player character)
protected:
	// 클릭으로 선택된 Hero. 커서가 벗어나도 다음 선택 변경 전까지 HoverRing 유지.
	// Hero가 선택된 경우에만 우클릭이 이동 명령으로 해석된다.
	TWeakObjectPtr<class AMainCharacter> SelectedHero;

	void SetHeroSelected(class AMainCharacter* Hero);
	void ClearHeroSelection();

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
