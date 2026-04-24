// Fill out your copyright notice in the Description page of Project Settings.


#include "BVPlayerController.h"

#include "BVRTSCameraPawn.h"
#include "EngineUtils.h"
#include "Characters/BVAutobotBase.h"
#include "Collision/BVCollision.h"
#include "InputMappingContext.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "MainCharacter.h"
#include "Blueprint/AIBlueprintHelperLibrary.h"
#include "Buildings/BVBuildingBase.h"
#include "Buildings/BVBuildingGhost.h"
#include "Characters/BVNPCBase.h"
#include "Components/AudioComponent.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/ShapeComponent.h"
#include "Components/WidgetComponent.h"
#include "Data/BVBuildingData.h"
#include "Engine/Canvas.h"
#include "Interface/BVDamageableInterface.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "Buildings/BVCityBase.h"
#include "Widget/BVBuildingDetailWidget.h"
#include "Widget/BVCaptureAnnouncementWidget.h"
#include "Widget/BVCityDetailWidget.h"
#include "Data/BVCityData.h"
#include "Widget/BVConstructionMenuWidget.h"
#include "Widget/BVUnitDetailWidget.h"
#include "Widget/BVGoldPopupWidget.h"
#include "Widget/BVInventoryWidget.h"
#include "Widget/BVShopWidget.h"

ABVPlayerController::ABVPlayerController()
{
	bShowMouseCursor = true;
	DefaultMouseCursor = EMouseCursor::Default;

	HoveredObject = nullptr;

}

void ABVPlayerController::BeginPlay()
{
	Super::BeginPlay();

	// Setup Input Mapping Context
	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		if (InputMappingContext)
		{
			Subsystem->AddMappingContext(InputMappingContext, 0);
		}
	}

	// Blue Team by default
	TeamID = FGenericTeamId(1);

	// Use Mouse Cursor
	FInputModeGameAndUI InputMode;
	InputMode.SetHideCursorDuringCapture(false);
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(InputMode);

	// Camera Move
	HeroCharacter = Cast<AMainCharacter>(UGameplayStatics::GetActorOfClass(GetWorld(), AMainCharacter::StaticClass()));
	if (HeroCharacter)
	{
		OnCameraCenterPressed();
	}

	// Fog of War
	if (RT_Discovered)
	{
		UKismetRenderingLibrary::ClearRenderTarget2D(this, RT_Discovered, FLinearColor::Black);
	}

	// 0.15초마다 한 번씩(초당 약 6.6회) UpdateFogOfWar 실행.
	// 매 프레임 호출하면 K2_DrawMaterial * (Vision/Discovered 2 × 시야 제공자 수) 만큼의
	// GPU 호출 + 렌더 스레드 동기화가 일어나서 프레임 드랍의 주요 원인이 된다.
	UpdateFogOfWar(); // 첫 프레임이 0.15초 동안 안개 미적용 상태로 깜빡이지 않도록 즉시 1회
	GetWorldTimerManager().SetTimer(FogTimerHandle, this, &ABVPlayerController::UpdateFogOfWar, 0.15f, true);
	
	// <------------------ Widgets ------------------>
	// MainHUDWidget
	if (MainHUDWidgetClass)
	{
		MainHUDWidget = CreateWidget<UUserWidget>(this, MainHUDWidgetClass);
		if (MainHUDWidget)
		{
			MainHUDWidget->AddToViewport();
		}
	}

	// <------------------ BGM ------------------>
	if (BGMPlaylist.Num() > 0)
	{
		int32 RandomIndex = FMath::RandRange(0, BGMPlaylist.Num() - 1);
		if (BGMPlaylist[RandomIndex])
		{

			float SoundDuration = BGMPlaylist[RandomIndex]->GetDuration();
			float RandomStartTime = FMath::FRandRange(0.f, SoundDuration / 2);
			BGMComponent = UGameplayStatics::CreateSound2D(this, BGMPlaylist[RandomIndex], Volume, 1.0f, 0.0f, nullptr, true);
			if (BGMComponent)
			{
				BGMComponent->FadeIn(10.0f, 0.5f, RandomStartTime);
			}

		}
	}
	
}

void ABVPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);

	// --- Box selection drag tracking ---
	if (bIsDragSelecting)
	{
		float MX = 0.f, MY = 0.f;
		if (GetMousePosition(MX, MY))
		{
			DragCurrentScreenPos = FVector2D(MX, MY);

			if (!bDragMovedBeyondThreshold)
			{
				if (FVector2D::Distance(DragCurrentScreenPos, DragStartScreenPos) > DragThresholdPixels)
				{
					bDragMovedBeyondThreshold = true;
				}
			}

			if (bDragMovedBeyondThreshold)
			{
				UpdateBoxHover();
			}
		}
	}

	// Construction Ghost
	if (bIsConstructionMode)
	{
		UpdateGhostLocation();
	}

	// Hero-주도 건설 경로(현장 이동 후 착공)는 City-only 빌드로 전환되며 제거됨.

	// Mouse Hovering
	FHitResult Hit;
	bool bHit = GetHitResultUnderCursor(ECC_MouseHover, false, Hit);
	AActor* NewHitActor = bHit ? Hit.GetActor() : nullptr;

	if (NewHitActor != HoveredObject)
	{
		// 커서가 벗어나도, '선택된 건물' 또는 '박스셀렉션에 포함된 액터'는 호버 유지
		auto IsBoxSelected = [this](AActor* A)
		{
			for (const TWeakObjectPtr<AActor>& W : BoxSelectedActors)
			{
				if (W.Get() == A) return true;
			}
			return false;
		};

		if (HoveredObject &&
			HoveredObject != DetailBuilding.Get() &&
			HoveredObject != SelectedHero.Get() &&
			!IsBoxSelected(HoveredObject))
		{
			if (HoveredObject->Implements<UBVDamageableInterface>())
			{
				IBVDamageableInterface::Execute_SetHovered(HoveredObject, false);
			}
		}

		if (NewHitActor)
		{
			if (NewHitActor->Implements<UBVDamageableInterface>())
			{
				IBVDamageableInterface::Execute_SetHovered(NewHitActor, true);

				if (HoverSound)
				{
					UGameplayStatics::PlaySound2D(this, HoverSound, HoverSoundVolume);
				}
			}
		}
	}

	HoveredObject = NewHitActor;

	// 상세 패널이 열려 있을 때, 리스폰 프로그레스 갱신 + 건물 옆에 붙여 따라가기
	// (BuildingDetailWidget 또는 CityDetailWidget 중 활성화된 쪽을 참조)
	if (ActiveDetailWidget && ActiveDetailWidget->GetVisibility() == ESlateVisibility::Visible)
	{
		if (ABVBuildingBase* DetailB = DetailBuilding.Get())
		{
			if (DetailB->bIsDestroyed)
			{
				HideBuildingDetail();
			}
			else
			{
				// 1) 리스폰 프로그레스 갱신 (위젯 종류별로)
				float Ratio = 0.f;
				if (DetailB->RespawnInterval > 0.f && DetailB->SpawnUnitClass)
				{
					Ratio = FMath::Fmod(DetailB->ElapsedTime, DetailB->RespawnInterval) / DetailB->RespawnInterval;
				}

				if (UBVBuildingDetailWidget* DetailW = Cast<UBVBuildingDetailWidget>(ActiveDetailWidget))
				{
					DetailW->SetRespawnProgress(Ratio);
				}
				else if (UBVCityDetailWidget* CityW = Cast<UBVCityDetailWidget>(ActiveDetailWidget))
				{
					CityW->SetRespawnProgress(Ratio);
				}

				// 2) 건물 월드 위치 -> 위젯(슬레이트) 좌표 변환해 위젯 위치 갱신
				FVector2D WidgetPos;
				if (UWidgetLayoutLibrary::ProjectWorldLocationToWidgetPosition(
						this, DetailB->GetActorLocation(), WidgetPos, false))
				{
					// 스크린 픽셀 기준 단순 오프셋: 건물 중심에서 오른쪽으로 약간 띄우기
					const FVector2D PanelOffset(60.f, -30.f);
					ActiveDetailWidget->SetPositionInViewport(WidgetPos + PanelOffset, false);
					ActiveDetailWidget->SetVisibility(ESlateVisibility::Visible);
				}
				else
				{
					// 건물이 카메라 뒤/밖이면 숨김
					ActiveDetailWidget->SetVisibility(ESlateVisibility::Hidden);
				}
			}
		}
		else
		{
			HideBuildingDetail();
		}
	}

	// 유닛 상세 패널이 열려 있을 때: 라이브 스탯 갱신 + 유닛 옆에 붙여 따라가기
	if (UnitDetailWidget && UnitDetailWidget->GetVisibility() == ESlateVisibility::Visible)
	{
		if (ABVAutobotBase* DetailU = DetailUnit.Get())
		{
			if (DetailU->bIsDead)
			{
				HideUnitDetail();
			}
			else
			{
				if (UBVUnitDetailWidget* DetailW = Cast<UBVUnitDetailWidget>(UnitDetailWidget))
				{
					DetailW->RefreshLiveStats();
				}

				FVector2D WidgetPos;
				if (UWidgetLayoutLibrary::ProjectWorldLocationToWidgetPosition(
						this, DetailU->GetActorLocation(), WidgetPos, false))
				{
					const FVector2D PanelOffset(60.f, -30.f);
					UnitDetailWidget->SetPositionInViewport(WidgetPos + PanelOffset, false);
					UnitDetailWidget->SetVisibility(ESlateVisibility::Visible);
				}
				else
				{
					UnitDetailWidget->SetVisibility(ESlateVisibility::Hidden);
				}
			}
		}
		else
		{
			HideUnitDetail();
		}
	}

	// Fog of war 는 BeginPlay에서 SetTimer(0.15s)로 주기 실행. 매 프레임 호출 안 함.
}

ETeamAttitude::Type ABVPlayerController::GetTeamAttitudeTowards(const AActor& Other) const
{
	const IGenericTeamAgentInterface* OtherTeamAgent = Cast<IGenericTeamAgentInterface>(&Other);
	if (!OtherTeamAgent) return ETeamAttitude::Neutral;

	FGenericTeamId MyTeamId = GetGenericTeamId();
	FGenericTeamId OtherTeamId = OtherTeamAgent->GetGenericTeamId();

	// 중립(EBVTeam::Neutral = 255)은 거점 점령을 위해 Hostile로 취급.
	// 같은 팀만 Friendly, 나머지(다른 팀 / 중립 / 미지정)는 모두 Hostile.
	return (MyTeamId == OtherTeamId) ? ETeamAttitude::Friendly : ETeamAttitude::Hostile;

}

void ABVPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

		if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent))
		{
			// [Camera Move]
			if (CameraPanAction)
			{
				EnhancedInputComponent->BindAction(CameraPanAction, ETriggerEvent::Triggered, this, &ABVPlayerController::OnCameraPan);
			}

			if (CameraCenterAction)
			{
				EnhancedInputComponent->BindAction(CameraCenterAction, ETriggerEvent::Started, this, &ABVPlayerController::OnCameraCenterPressed);
			}

			if (CameraZoomAction)
			{
				EnhancedInputComponent->BindAction(CameraZoomAction, ETriggerEvent::Triggered, this, &ABVPlayerController::OnCameraZoom);
			}

			// Space + 1: 마지막 전투 현장으로 점프 (팔로우 해제)
			if (FocusLastCombatAction)
			{
				EnhancedInputComponent->BindAction(FocusLastCombatAction, ETriggerEvent::Started, this, &ABVPlayerController::OnFocusLastCombatPressed);
			}

			// Hero 강제 선택 (Space 등)
			if (SelectHeroAction)
			{
				EnhancedInputComponent->BindAction(SelectHeroAction, ETriggerEvent::Started, this, &ABVPlayerController::OnSelectHeroPressed);
			}

			// 내 소유 첫 거점으로 카메라 Jump (1)
			if (FocusFirstCityAction)
			{
				EnhancedInputComponent->BindAction(FocusFirstCityAction, ETriggerEvent::Started, this, &ABVPlayerController::OnFocusFirstCityPressed);
			}


			// [Normal Mode]
			if (MoveAction)
			{
				// Right Click -> Move
				EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Started, this, &ABVPlayerController::MoveToLocation);
			}

			if (SelectAction)
			{
				// Left Click Pressed -> 드래그 시작 기록 (단일선택은 Released에서 판정)
				EnhancedInputComponent->BindAction(SelectAction, ETriggerEvent::Started, this, &ABVPlayerController::OnSelectPressed);
				EnhancedInputComponent->BindAction(SelectAction, ETriggerEvent::Completed, this, &ABVPlayerController::OnSelectReleased);
				EnhancedInputComponent->BindAction(SelectAction, ETriggerEvent::Canceled, this, &ABVPlayerController::OnSelectReleased);
			}

			// [Build Mode]
			if (BuildAction)
			{
				// B -> Enter to Build Mode
				EnhancedInputComponent->BindAction(BuildAction, ETriggerEvent::Started, this, &ABVPlayerController::OnBuildKeyPressed);
			}
			
			if (BuildClickAction)
			{
				// Left Click in Build Mode -> Confirm the construction
				EnhancedInputComponent->BindAction(BuildClickAction, ETriggerEvent::Started, this, &ABVPlayerController::OnBuildClick);
			}

			// Inventory Widget
			if (InventoryAction)
			{
				EnhancedInputComponent->BindAction(InventoryAction, ETriggerEvent::Started, this, &ABVPlayerController::ToggleInventoryUI);
			}

			// ESC: Widget Close
			if (CloseUIAction)
			{
				EnhancedInputComponent->BindAction(CloseUIAction, ETriggerEvent::Started, this, &ABVPlayerController::CloseCurrentUI);
			}

			// Tab: Toggle all overhead widgets (units + buildings)
			if (ToggleOverheadWidgetsAction)
			{
				EnhancedInputComponent->BindAction(ToggleOverheadWidgetsAction, ETriggerEvent::Started, this, &ABVPlayerController::ToggleOverheadWidgets);
			}
		}
}

void ABVPlayerController::ToggleOverheadWidgets()
{
	bOverheadWidgetsVisible = !bOverheadWidgetsVisible;

	// 건물 위젯
	for (TActorIterator<ABVBuildingBase> It(GetWorld()); It; ++It)
	{
		if (It->OverheadWidgetComponent)
		{
			It->OverheadWidgetComponent->SetVisibility(bOverheadWidgetsVisible);
		}
	}

	// 유닛 위젯
	for (TActorIterator<ABVAutobotBase> It(GetWorld()); It; ++It)
	{
		if (It->OverheadWidgetComponent)
		{
			It->OverheadWidgetComponent->SetVisibility(bOverheadWidgetsVisible);
		}
	}
}

void ABVPlayerController::MoveToLocation(const FInputActionValue& Value)
{
	// 우클릭 = 상세 패널 닫기 (상시 실행).
	HideBuildingDetail();
	HideUnitDetail();

	if (bIsConstructionMode)
	{
		// 우클릭으로 건설 모드 취소 (유령 건물/데칼 정리는 ExitConstructionMode가 일괄 처리).
		bIsConstructionMode = false;
		PendingBuildingClass = nullptr;

		ExitConstructionMode();

		if (CurrentGhostActor)
		{
			CurrentGhostActor->Destroy();
			CurrentGhostActor = nullptr;
		}
	}

	// Hero 이동은 Hero가 '선택된' 상태일 때만 수행. 선택 없으면 우클릭은 패널 닫기 전용.
	AMainCharacter* MoveHero = SelectedHero.Get();
	if (!MoveHero) return;

	FHitResult Hit;
	if (GetHitResultUnderCursor(ECC_Visibility, false, Hit))
	{
		const FVector DestLocation = Hit.ImpactPoint;
		if (AController* HeroController = MoveHero->GetController())
		{
			UAIBlueprintHelperLibrary::SimpleMoveToLocation(HeroController, DestLocation);
		}
	}
}

void ABVPlayerController::SelectObject()
{
	FHitResult Hit;

	if (GetHitResultUnderCursor(ECC_Visibility, false, Hit))
	{
		AActor* HitActor = Hit.GetActor();
		SelectedActor = HitActor;
		OnSelectionChanged.Broadcast(SelectedActor);

		if (SelectedActor)
		{
			ABVNPCBase* ClickedNPC = Cast<ABVNPCBase>(SelectedActor);
			if (ClickedNPC)
			{
				AMainCharacter* MyCharacter = HeroCharacter;
				if (MyCharacter)
				{
					float Distance = FVector::Dist(MyCharacter->GetActorLocation(), ClickedNPC->GetActorLocation());
					float InteractableDistance = 300.0f;

					if (Distance <= InteractableDistance)
					{
						ClickedNPC->Interact(MyCharacter);
					}
				}
			}

			// Hero(플레이어 캐릭터) 클릭: 선택 상태로 전환해 아래에 HoverRing 유지.
			if (AMainCharacter* ClickedHero = Cast<AMainCharacter>(SelectedActor))
			{
				HideBuildingDetail();
				HideUnitDetail();
				SetHeroSelected(ClickedHero);
			}
			// 건물 클릭: 첫 클릭은 패널 열기/전환, 같은 건물 재클릭은 카메라 점프.
			// (닫기는 우클릭 전담.)
			else if (ABVBuildingBase* ClickedBuilding = Cast<ABVBuildingBase>(SelectedActor))
			{
				ClearHeroSelection();
				HideUnitDetail();

				if (DetailBuilding.Get() == ClickedBuilding)
				{
					// 이미 이 건물의 패널이 열려 있으면 카메라만 그 건물로 점프.
					if (ABVRTSCameraPawn* CamPawn = Cast<ABVRTSCameraPawn>(GetPawn()))
					{
						CamPawn->JumpToLocation(ClickedBuilding->GetActorLocation());
					}
				}
				else
				{
					// 처음 열거나 다른 건물로 전환.
					ShowBuildingDetail(ClickedBuilding);
				}
			}
			// 유닛 클릭 시 상세 패널 토글
			else if (ABVAutobotBase* ClickedUnit = Cast<ABVAutobotBase>(SelectedActor))
			{
				ClearHeroSelection();
				HideBuildingDetail();
				if (DetailUnit.Get() == ClickedUnit)
				{
					HideUnitDetail();
				}
				else
				{
					ShowUnitDetail(ClickedUnit);
				}
			}
			else
			{
				ClearHeroSelection();
				HideBuildingDetail();
				HideUnitDetail();
			}
		}
		else
		{
			// 빈 공간 클릭 -> 선택/상세 패널 모두 해제
			ClearHeroSelection();
			HideBuildingDetail();
			HideUnitDetail();
		}
	}
}

void ABVPlayerController::SetHeroSelected(AMainCharacter* Hero)
{
	// 이전 Hero가 다른 Hero였다면 hover 해제 (현재 커서 위에 있지 않은 경우에만).
	if (AMainCharacter* Prev = SelectedHero.Get())
	{
		if (Prev != Hero && Prev != HoveredObject)
		{
			if (Prev->Implements<UBVDamageableInterface>())
			{
				IBVDamageableInterface::Execute_SetHovered(Prev, false);
			}
		}
	}

	SelectedHero = Hero;

	if (Hero && Hero->Implements<UBVDamageableInterface>())
	{
		IBVDamageableInterface::Execute_SetHovered(Hero, true);
	}
}

void ABVPlayerController::ClearHeroSelection()
{
	if (AMainCharacter* Prev = SelectedHero.Get())
	{
		if (Prev != HoveredObject && Prev->Implements<UBVDamageableInterface>())
		{
			IBVDamageableInterface::Execute_SetHovered(Prev, false);
		}
	}
	SelectedHero = nullptr;
}

void ABVPlayerController::OnBuildKeyPressed()
{
	if (bIsConstructionMode)
	{
		ExitConstructionMode();
		return;
	}

	// City-only 빌드: City Detail 패널이 열려서 도시를 보여줄 때만 B키가 먹힌다.
	// 도시가 아닌 상태에서는 B키 무시 (Hero 주도 글로벌 빌드 경로는 제거됨).
	ABVCityBase* ContextCity = nullptr;
	if (CityDetailWidget && CityDetailWidget->GetVisibility() == ESlateVisibility::Visible)
	{
		ContextCity = Cast<ABVCityBase>(DetailBuilding.Get());
	}

	if (!ContextCity)
	{
		return;
	}

	ToggleConstructionMenuUI(ContextCity);
}

void ABVPlayerController::ToggleInventoryUI()
{
	if (!InventoryWidget && InventoryWidgetClass)
	{
		InventoryWidget = CreateWidget<UUserWidget>(this, InventoryWidgetClass);
		if (InventoryWidget)
		{
			InventoryWidget->AddToViewport();
			InventoryWidget->SetVisibility(ESlateVisibility::Hidden);
		}
	}

	if (!InventoryWidget) return;
	
	if (InventoryWidget->GetVisibility() == ESlateVisibility::Visible)
	{
		InventoryWidget->SetVisibility(ESlateVisibility::Hidden);

		FInputModeGameAndUI InputMode;
		InputMode.SetHideCursorDuringCapture(false);
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		SetInputMode(InputMode);
	}
	else
	{
		CloseCurrentUI(); 

		if (UBVInventoryWidget* InvWidget = Cast<UBVInventoryWidget>(InventoryWidget))
		{
			InvWidget->RefreshInventory();
		}

		InventoryWidget->SetVisibility(ESlateVisibility::Visible);

		FInputModeGameAndUI InputMode;
		InputMode.SetHideCursorDuringCapture(false);
		SetInputMode(InputMode);
	}
}

void ABVPlayerController::ToggleConstructionMenuUI(ABVCityBase* ContextCity)
{

	if (!ConstructionMenuWidget && ConstructionMenuWidgetClass)
	{
		ConstructionMenuWidget = CreateWidget<UUserWidget>(this, ConstructionMenuWidgetClass);
		if (ConstructionMenuWidget)
		{
			ConstructionMenuWidget->AddToViewport();
			ConstructionMenuWidget->SetVisibility(ESlateVisibility::Hidden);
		}
	}

	if (!ConstructionMenuWidget) return;

	if (ConstructionMenuWidget->GetVisibility() == ESlateVisibility::Visible)
	{
		ConstructionMenuWidget->SetVisibility(ESlateVisibility::Hidden);

		FInputModeGameAndUI InputMode;
		InputMode.SetHideCursorDuringCapture(false);
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		SetInputMode(InputMode);
	}
	else
	{
		CloseCurrentUI();

		// 도시 컨텍스트면 도시의 BuildableBuildings로, 아니면 글로벌 Hero 카탈로그로 재구성.
		// NativeConstruct는 최초 1회 오픈 시 글로벌로 이미 채우지만, 도시 전환/재오픈 대비 매 토글마다 호출.
		if (UBVConstructionMenuWidget* MenuW = Cast<UBVConstructionMenuWidget>(ConstructionMenuWidget))
		{
			MenuW->Populate(ContextCity);
		}

		ConstructionMenuWidget->SetVisibility(ESlateVisibility::Visible);

		FInputModeGameAndUI InputMode;
		InputMode.SetWidgetToFocus(ConstructionMenuWidget->TakeWidget());
		SetInputMode(InputMode);
	}
}

void ABVPlayerController::CloseCurrentUI()
{
	// Close Shop
	if (ShopWidget && ShopWidget->GetVisibility() == ESlateVisibility::Visible)
	{
		CloseShopUI();
	}

	// Close Inventory
	if (InventoryWidget && InventoryWidget->GetVisibility() == ESlateVisibility::Visible)
	{
		InventoryWidget->SetVisibility(ESlateVisibility::Hidden);
	}

	// Close Construction Menu
	if (ConstructionMenuWidget && ConstructionMenuWidget->GetVisibility() == ESlateVisibility::Visible)
	{
		ConstructionMenuWidget->SetVisibility(ESlateVisibility::Hidden);
	}

	// Close Building Detail
	HideBuildingDetail();

	// Exit Ghost Building Mode if active
	if (bIsConstructionMode)
	{
		ExitConstructionMode();
	}
}

void ABVPlayerController::ShowBuildingDetail(ABVBuildingBase* InBuilding)
{
	if (!InBuilding) return;

	// 이전 선택 건물이 있고 새 선택과 다르다면, 커서 위에 있지 않을 때 호버 이펙트 해제
	if (ABVBuildingBase* Previous = DetailBuilding.Get())
	{
		if (Previous != InBuilding && Previous != HoveredObject)
		{
			if (Previous->Implements<UBVDamageableInterface>())
			{
				IBVDamageableInterface::Execute_SetHovered(Previous, false);
			}
		}
	}

	// 거점(City)과 일반 건물은 서로 다른 디테일 위젯 사용.
	// City 패널 클래스 결정: DA의 CityDetailWidgetClass 우선, 비어있으면 PC 폴백.
	// 둘 다 비어있으면 기존 BuildingDetailWidget으로 자연 폴백.
	ABVCityBase* City = Cast<ABVCityBase>(InBuilding);

	TSubclassOf<UUserWidget> EffectiveCityWidgetClass = nullptr;
	if (City)
	{
		if (const UBVCityData* CityData = Cast<UBVCityData>(City->BuildingData))
		{
			EffectiveCityWidgetClass = CityData->CityDetailWidgetClass;
		}
		if (!EffectiveCityWidgetClass) // DA 슬롯 비었으면 PC 폴백
		{
			EffectiveCityWidgetClass = CityDetailWidgetClass;
		}
	}

	const bool bUseCityWidget = (City != nullptr) && (EffectiveCityWidgetClass != nullptr);

	// 반대편 위젯은 숨김.
	if (bUseCityWidget)
	{
		if (BuildingDetailWidget) BuildingDetailWidget->SetVisibility(ESlateVisibility::Hidden);
	}
	else
	{
		if (CityDetailWidget) CityDetailWidget->SetVisibility(ESlateVisibility::Hidden);
	}

	// 필요한 쪽 위젯을 lazy-init.
	UUserWidget* TargetWidget = nullptr;

	if (bUseCityWidget)
	{
		// 캐시 클래스가 다르면(다른 도시가 다른 패널 쓰는 경우) 폐기 후 재생성.
		if (CityDetailWidget && CityDetailWidget->GetClass() != EffectiveCityWidgetClass)
		{
			CityDetailWidget->RemoveFromParent();
			CityDetailWidget = nullptr;
		}

		if (!CityDetailWidget)
		{
			CityDetailWidget = CreateWidget<UUserWidget>(this, EffectiveCityWidgetClass);
			if (CityDetailWidget)
			{
				CityDetailWidget->AddToViewport();
				CityDetailWidget->SetVisibility(ESlateVisibility::Hidden);
			}
		}
		TargetWidget = CityDetailWidget;
	}
	else
	{
		if (!BuildingDetailWidget && BuildingDetailWidgetClass)
		{
			BuildingDetailWidget = CreateWidget<UUserWidget>(this, BuildingDetailWidgetClass);
			if (BuildingDetailWidget)
			{
				BuildingDetailWidget->AddToViewport();
				BuildingDetailWidget->SetVisibility(ESlateVisibility::Hidden);
			}
		}
		TargetWidget = BuildingDetailWidget;
	}

	if (!TargetWidget)
	{
		return;
	}

	DetailBuilding = InBuilding;
	ActiveDetailWidget = TargetWidget;

	// 선택된 건물은 호버 이펙트를 강제로 켜둔다 (커서가 벗어나도 유지)
	if (InBuilding->Implements<UBVDamageableInterface>())
	{
		IBVDamageableInterface::Execute_SetHovered(InBuilding, true);
	}

	// 위젯 종류에 맞는 초기화 호출.
	// 거점이라도 CityDetailWidgetClass가 없어서 BuildingDetailWidget으로 폴백한 경우
	// SetFromBuilding로 초기화 (건물 패널이 체력을 보여주게 됨).
	if (bUseCityWidget)
	{
		if (UBVCityDetailWidget* DetailW = Cast<UBVCityDetailWidget>(TargetWidget))
		{
			DetailW->SetFromCity(City);
		}
	}
	else
	{
		if (UBVBuildingDetailWidget* DetailW = Cast<UBVBuildingDetailWidget>(TargetWidget))
		{
			DetailW->SetFromBuilding(InBuilding);
		}
	}

	TargetWidget->SetVisibility(ESlateVisibility::Visible);

	// 오픈 즉시 한 번 위치를 잡아둠 (다음 틱부터 PlayerTick에서 이어서 갱신)
	FVector2D WidgetPos;
	if (UWidgetLayoutLibrary::ProjectWorldLocationToWidgetPosition(
			this, InBuilding->GetActorLocation(), WidgetPos, false))
	{
		const FVector2D PanelOffset(60.f, -30.f);
		TargetWidget->SetPositionInViewport(WidgetPos + PanelOffset, false);
	}
}

void ABVPlayerController::HideBuildingDetail()
{
	// 선택 해제 시, 현재 커서가 그 건물 위에 있지 않다면 호버 이펙트를 끈다
	if (ABVBuildingBase* Previous = DetailBuilding.Get())
	{
		if (Previous != HoveredObject && Previous->Implements<UBVDamageableInterface>())
		{
			IBVDamageableInterface::Execute_SetHovered(Previous, false);
		}
	}

	DetailBuilding = nullptr;
	ActiveDetailWidget = nullptr;

	if (BuildingDetailWidget && BuildingDetailWidget->GetVisibility() == ESlateVisibility::Visible)
	{
		BuildingDetailWidget->SetVisibility(ESlateVisibility::Hidden);
	}
	if (CityDetailWidget && CityDetailWidget->GetVisibility() == ESlateVisibility::Visible)
	{
		CityDetailWidget->SetVisibility(ESlateVisibility::Hidden);
	}
}

void ABVPlayerController::ShowCaptureAnnouncement(const FText& Message)
{
	if (!CaptureAnnouncementWidgetClass)
	{
		return;
	}

	// Lazy-init.
	if (!CaptureAnnouncementWidget)
	{
		CaptureAnnouncementWidget = CreateWidget<UUserWidget>(this, CaptureAnnouncementWidgetClass);
		if (CaptureAnnouncementWidget)
		{
			// ZOrder 높게 해서 다른 UI 위에 표시.
			CaptureAnnouncementWidget->AddToViewport(100);
		}
	}

	if (!CaptureAnnouncementWidget) return;

	// 메시지 설정 + 표시.
	if (UBVCaptureAnnouncementWidget* AnnounceW = Cast<UBVCaptureAnnouncementWidget>(CaptureAnnouncementWidget))
	{
		AnnounceW->SetMessage(Message);
	}
	CaptureAnnouncementWidget->SetVisibility(ESlateVisibility::HitTestInvisible);

	// 이미 띄워진 경우 타이머 리셋(최신 메시지 기준으로 3초 유지).
	GetWorldTimerManager().ClearTimer(CaptureAnnouncementHideTimerHandle);
	GetWorldTimerManager().SetTimer(
		CaptureAnnouncementHideTimerHandle,
		FTimerDelegate::CreateWeakLambda(this, [this]()
		{
			if (!CaptureAnnouncementWidget) return;

			// 페이드 아웃 애니메이션 재생. 애니메이션 종료 시점에 위젯이 자동으로 Hidden 처리.
			// (애니메이션 없으면 PlayFadeOut이 즉시 Hidden 처리)
			if (UBVCaptureAnnouncementWidget* AnnounceW = Cast<UBVCaptureAnnouncementWidget>(CaptureAnnouncementWidget))
			{
				AnnounceW->PlayFadeOut();
			}
			else
			{
				CaptureAnnouncementWidget->SetVisibility(ESlateVisibility::Hidden);
			}
		}),
		CaptureAnnouncementDuration,
		false);
}

void ABVPlayerController::ShowUnitDetail(ABVAutobotBase* InUnit)
{
	if (!InUnit) return;

	// 최초 1회만 위젯 생성
	if (!UnitDetailWidget && UnitDetailWidgetClass)
	{
		UnitDetailWidget = CreateWidget<UUserWidget>(this, UnitDetailWidgetClass);
		if (UnitDetailWidget)
		{
			UnitDetailWidget->AddToViewport();
			UnitDetailWidget->SetVisibility(ESlateVisibility::Hidden);
		}
	}

	if (!UnitDetailWidget) return;

	DetailUnit = InUnit;

	// 선택된 유닛에 호버 이펙트 강제 ON
	if (InUnit->Implements<UBVDamageableInterface>())
	{
		IBVDamageableInterface::Execute_SetHovered(InUnit, true);
	}

	if (UBVUnitDetailWidget* DetailW = Cast<UBVUnitDetailWidget>(UnitDetailWidget))
	{
		DetailW->SetFromUnit(InUnit);
	}

	UnitDetailWidget->SetVisibility(ESlateVisibility::Visible);

	// 초기 위치 잡기
	FVector2D WidgetPos;
	if (UWidgetLayoutLibrary::ProjectWorldLocationToWidgetPosition(
			this, InUnit->GetActorLocation(), WidgetPos, false))
	{
		const FVector2D PanelOffset(60.f, -30.f);
		UnitDetailWidget->SetPositionInViewport(WidgetPos + PanelOffset, false);
	}
}

void ABVPlayerController::HideUnitDetail()
{
	if (ABVAutobotBase* Previous = DetailUnit.Get())
	{
		if (Previous != HoveredObject && Previous->Implements<UBVDamageableInterface>())
		{
			IBVDamageableInterface::Execute_SetHovered(Previous, false);
		}
	}

	DetailUnit = nullptr;

	if (UnitDetailWidget && UnitDetailWidget->GetVisibility() == ESlateVisibility::Visible)
	{
		if (UBVUnitDetailWidget* DetailW = Cast<UBVUnitDetailWidget>(UnitDetailWidget))
		{
			DetailW->SetFromUnit(nullptr);
		}
		UnitDetailWidget->SetVisibility(ESlateVisibility::Hidden);
	}
}

void ABVPlayerController::OnBuildClick()
{
	// This function is called under Build IMC

	// 슬롯 클릭으로 막 빌드 모드에 진입한 직후(수십 ms 이내)에 흘러들어오는 유령 입력만 차단.
	// 사용자의 실제 다음 클릭은 반드시 이 창을 넘긴다.
	if (BuildModeEnterTime > 0.0 && GetWorld())
	{
		const double Elapsed = GetWorld()->GetTimeSeconds() - BuildModeEnterTime;
		if (Elapsed < BuildClickInputIgnoreWindow)
		{
			return;
		}
	}

	if (!bCanBuild || !CurrentGhostActor)
	{
		// TODO : Can't Build there
		return;
	}

	// City-only 빌드 흐름: CurrentBuildCity가 살아있어야만 착공.
	// (Hero 이동 경로는 제거됨. 도시가 자체 자원/타이머로 건설하는 컨셉.)
	ABVCityBase* BuildCity = CurrentBuildCity.Get();
	if (!BuildCity || !HeroCharacter || !CurrentBuildingClass)
	{
		ExitConstructionMode();
		return;
	}

	PendingBuildingClass = CurrentBuildingClass;
	TargetBuildLocation = CurrentGhostActor->GetActorLocation();

	// HeroCharacter::ConstructBuilding은 SpawnActor로 ConstructionSite를 만드는 함수.
	// 이름은 Hero지만 실제로 Hero 위치/이동과 무관한 '월드에 착공'용으로 재사용.
	HeroCharacter->ConstructBuilding(TargetBuildLocation, PendingBuildingClass, PendingConstructionTime);

	PlayAnnouncerVoice(EBVAnnouncerEvent::ConstructionStarted);

	// 데칼/IMC/ghost 정리는 ExitConstructionMode로 일괄 처리.
	PendingBuildingClass = nullptr;

	// ExitConstructionMode가 CurrentBuildCity를 리셋하므로, 재선택용으로 먼저 저장.
	ABVCityBase* CityToReSelect = BuildCity;
	ExitConstructionMode();

	// 착공 후 거점 선택(Detail 패널 + hover ring)을 유지.
	if (CityToReSelect)
	{
		ShowBuildingDetail(CityToReSelect);
	}
}

void ABVPlayerController::OnCameraZoom(const FInputActionValue& Value)
{
	if (ABVRTSCameraPawn* CamPawn = Cast<ABVRTSCameraPawn>(GetPawn()))
	{
		// 휠 스크롤 값(float)을 넘겨줍니다.
		CamPawn->ZoomCamera(Value.Get<float>());
	}
}

void ABVPlayerController::OnCameraPan(const FInputActionValue& Value)
{
	if (ABVRTSCameraPawn* CamPawn = Cast<ABVRTSCameraPawn>(GetPawn()))
	{
		CamPawn->MoveCamera(Value.Get<FVector2D>());
	}
}

void ABVPlayerController::OnCameraCenterPressed()
{
	if (ABVRTSCameraPawn* CamPawn = Cast<ABVRTSCameraPawn>(GetPawn()))
	{
		CamPawn->CenterOnActor(HeroCharacter);
	}
}

void ABVPlayerController::OnFocusLastCombatPressed()
{
	if (!bHasLastCombatLocation)
	{
		return;
	}

	ABVRTSCameraPawn* CamPawn = Cast<ABVRTSCameraPawn>(GetPawn());
	if (!CamPawn)
	{
		return;
	}

	CamPawn->JumpToLocation(LastCombatLocation);
}

void ABVPlayerController::OnSelectHeroPressed()
{
	UE_LOG(LogTemp, Warning, TEXT("[SelectHero] pressed. HeroCharacter=%s"), *GetNameSafe(HeroCharacter));
	// HeroCharacter 자동 선택 + 카메라를 Hero follow 모드로 진입.
	if (!HeroCharacter) return;

	HideBuildingDetail();
	HideUnitDetail();
	SetHeroSelected(HeroCharacter);

	if (ABVRTSCameraPawn* CamPawn = Cast<ABVRTSCameraPawn>(GetPawn()))
	{
		CamPawn->CenterOnActor(HeroCharacter);
	}
}

void ABVPlayerController::OnFocusFirstCityPressed()
{
	// 내 팀 소유 거점 중 첫 번째(TActorIterator 순서)에 카메라 Jump.
	const FGenericTeamId MyTeamId = GetGenericTeamId();
	ABVCityBase* FirstOwnedCity = nullptr;
	for (TActorIterator<ABVCityBase> It(GetWorld()); It; ++It)
	{
		ABVCityBase* City = *It;
		if (!City) continue;
		if (City->GetGenericTeamId() == MyTeamId)
		{
			FirstOwnedCity = City;
			break;
		}
	}

	if (!FirstOwnedCity) return;

	// 거점 자동 선택 (Detail 패널 표시) + 다른 선택 상태 해제.
	ClearHeroSelection();
	HideUnitDetail();
	ShowBuildingDetail(FirstOwnedCity);

	if (ABVRTSCameraPawn* CamPawn = Cast<ABVRTSCameraPawn>(GetPawn()))
	{
		CamPawn->JumpToLocation(FirstOwnedCity->GetActorLocation());
	}
}

void ABVPlayerController::ReportCombatLocation(const FVector& WorldLocation)
{
	LastCombatLocation = WorldLocation;
	bHasLastCombatLocation = true;
}

void ABVPlayerController::UpdateFogOfWar()
{
	if (!RT_Discovered || !RT_Vision || !VisionBrushMaterial) return;

	UKismetRenderingLibrary::ClearRenderTarget2D(this, RT_Vision, FLinearColor::Black);

	struct FVisionTarget
	{
		AActor* TargetActor;
		float Radius;
	};

	TArray<FVisionTarget> VisionProviders;

	if (HeroCharacter) VisionProviders.Add({HeroCharacter, HeroCharacter->VisionRadius});

	for (TActorIterator<ABVBuildingBase> It(GetWorld()); It; ++It)
	{
		if (It->GetGenericTeamId() == GetGenericTeamId() && !It->bIsDestroyed)
		{
			float Rad = 5000.0f;
			VisionProviders.Add({*It, Rad});
		}
	}

	for (TActorIterator<ABVAutobotBase> It(GetWorld()); It; ++It)
	{
		if (It->GetGenericTeamId() == GetGenericTeamId() && !It->bIsDead)
		{
			VisionProviders.Add({*It, It->VisionRadius});
		}
	}

	// City discovery: 아직 발견되지 않은 도시가 아군 시야 반경에 들어오면 1회 발견 처리.
	// MarkDiscoveredByPlayer 내부에서 중복 가드 + 공지/델리게이트까지 처리하므로 여기선 호출만.
	for (TActorIterator<ABVCityBase> It(GetWorld()); It; ++It)
	{
		ABVCityBase* City = *It;
		if (!City || City->bDiscoveredByPlayer) continue;

		const FVector CityLoc = City->GetActorLocation();
		for (const FVisionTarget& Provider : VisionProviders)
		{
			if (!Provider.TargetActor) continue;
			const float Dist = FVector::Distance(CityLoc, Provider.TargetActor->GetActorLocation());
			if (Dist <= Provider.Radius)
			{
				City->MarkDiscoveredByPlayer();
				break;
			}
		}
	}

	// 1. Draw to RT_Vision
	UCanvas* CanvasVision;
	FVector2D SizeVision;
	FDrawToRenderTargetContext ContextVision;

	UKismetRenderingLibrary::BeginDrawCanvasToRenderTarget(this, RT_Vision, CanvasVision, SizeVision, ContextVision);
	if (CanvasVision)
	{
		for (const FVisionTarget& Provider : VisionProviders)
		{
			FVector Loc = Provider.TargetActor->GetActorLocation();
			float NormalizedX = ((Loc.X - MapCenter.X) / MapSize) + 0.5f;
			float NormalizedY = ((Loc.Y - MapCenter.Y) / MapSize) + 0.5f;

			// 브러시는 원이 사각형에 inscribed 되는 머티리얼이라, 한 변 = 지름(2R) 이어야 한다.
			float BrushSize = (Provider.Radius * 2.0f / MapSize) * SizeVision.X;
			FVector2D BrushExtent(BrushSize, BrushSize);
			FVector2D BrushOffset(BrushSize / 2.0f, BrushSize / 2.0f);

			FVector2D DrawPos(NormalizedX * SizeVision.X, NormalizedY * SizeVision.Y);
			FVector2D FinalPos = DrawPos - BrushOffset;

			CanvasVision->K2_DrawMaterial(
				VisionBrushMaterial, FinalPos, BrushExtent,
				FVector2D::ZeroVector, FVector2D::UnitVector, 0.0f, FVector2D(0.5f, 0.5f)
			);
		}
	}
	UKismetRenderingLibrary::EndDrawCanvasToRenderTarget(this, ContextVision);

	// 2. Draw to RT_Discovered
	UCanvas* CanvasDiscovered;
	FVector2D SizeDiscovered;
	FDrawToRenderTargetContext ContextDiscovered;

	UKismetRenderingLibrary::BeginDrawCanvasToRenderTarget(this, RT_Discovered, CanvasDiscovered, SizeDiscovered, ContextDiscovered);
	if (CanvasDiscovered)
	{
		for (const FVisionTarget& Provider : VisionProviders)
		{
			FVector Loc = Provider.TargetActor->GetActorLocation();
			float NormalizedX = ((Loc.X - MapCenter.X) / MapSize) + 0.5f;
			float NormalizedY = ((Loc.Y - MapCenter.Y) / MapSize) + 0.5f;

			// RT_Vision과 동일하게 지름 기준
			float BrushSize = (Provider.Radius * 2.0f / MapSize) * SizeDiscovered.X;
			FVector2D BrushExtent(BrushSize, BrushSize);
			FVector2D BrushOffset(BrushSize / 2.0f, BrushSize / 2.0f);

			FVector2D DrawPos(NormalizedX * SizeDiscovered.X, NormalizedY * SizeDiscovered.Y);
			FVector2D FinalPos = DrawPos - BrushOffset;

			CanvasDiscovered->K2_DrawMaterial(
				VisionBrushMaterial, FinalPos, BrushExtent,
				FVector2D::ZeroVector, FVector2D::UnitVector, 0.0f, FVector2D(0.5f, 0.5f)
			);
		}
	}
	UKismetRenderingLibrary::EndDrawCanvasToRenderTarget(this, ContextDiscovered);

	// 적 건물 숨기기
	for (TActorIterator<ABVBuildingBase> It(GetWorld()); It; ++It)
	{
		// 아군이 아닌 '적(Hostile)' 건물만 검사합니다.
		if (It->GetGenericTeamId() != GetGenericTeamId() && !It->bIsDestroyed)
		{
			bool bIsVisible = false;

			// 모든 아군의 시야 반경과 적 건물의 거리를 비교
			for (const FVisionTarget& Provider : VisionProviders)
			{
				float Distance = FVector::Distance(It->GetActorLocation(), Provider.TargetActor->GetActorLocation());
				if (Distance <= Provider.Radius)
				{
					bIsVisible = true; // 단 한 명의 아군이라도 보고 있다면 시야 확보!
					break;
				}
			}

			// 위젯 끄기/켜기 (Tab 전역 토글과 AND)
			if (It->OverheadWidgetComponent)
			{
				It->OverheadWidgetComponent->SetVisibility(bIsVisible && bOverheadWidgetsVisible);
			}
			It->SetActorHiddenInGame(!bIsVisible);
		}
	}

	// 적 유닛 숨기기
	for (TActorIterator<ABVAutobotBase> It(GetWorld()); It; ++It)
	{
		// 아군이 아닌 '적(Hostile)' 유닛만 검사합니다.
		if (It->GetGenericTeamId() != GetGenericTeamId() && !It->bIsDead)
		{
			bool bIsVisible = false;

			for (const FVisionTarget& Provider : VisionProviders)
			{
				float Distance = FVector::Distance(It->GetActorLocation(), Provider.TargetActor->GetActorLocation());
				if (Distance <= Provider.Radius)
				{
					bIsVisible = true;
					break;
				}
			}

			// 위젯 끄기/켜기 (Tab 전역 토글과 AND)
			if (It->OverheadWidgetComponent)
			{
				It->OverheadWidgetComponent->SetVisibility(bIsVisible && bOverheadWidgetsVisible);
			}

			It->SetActorHiddenInGame(!bIsVisible);
		}
		else if (It->GetGenericTeamId() == GetGenericTeamId() && !It->bIsDead)
		{
			// 아군 유닛: 항상 액터는 보이고, 위젯만 Tab 토글에 따른다
			if (It->OverheadWidgetComponent)
			{
				It->OverheadWidgetComponent->SetVisibility(bOverheadWidgetsVisible);
			}
			It->SetActorHiddenInGame(false);
		}
	}
}

void ABVPlayerController::EnterConstructionMode(TSubclassOf<ABVBuildingBase> InBuildingClass, float InConstructionTime)
{
	if (!DefaultBuildingClass || !GhostActorClass) return;

	if (ConstructionMenuWidget && ConstructionMenuWidget->IsInViewport())
	{
		ToggleConstructionMenuUI();
	}

	CurrentBuildingClass = InBuildingClass;
	PendingConstructionTime = InConstructionTime;
	bIsConstructionMode = true;

	// Switching to Building IMC
	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		if (BuildMappingContext)
		{
			Subsystem->AddMappingContext(BuildMappingContext, 1);
		}
	}

	// 1. First, spawn a building actor 
	if (UWorld* World = GetWorld())
	{
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		CurrentGhostActor = World->SpawnActor<ABVBuildingGhost>(GhostActorClass, FVector::ZeroVector, FRotator::ZeroRotator, Params);

		if (CurrentGhostActor)
		{
			// 2. Add Ghost material to the spawned building
			// 하드코딩 되어있던 DefaultBuildingClass를 지우고 InBuildingClass를 CDO로 가져옵니다.
			ABVBuildingBase* BuildingCDO = InBuildingClass->GetDefaultObject<ABVBuildingBase>();
			if (BuildingCDO)
			{
				UStaticMeshComponent* BuildingMeshComp = BuildingCDO->FindComponentByClass<UStaticMeshComponent>();
				USceneComponent* SceneRootComp = BuildingCDO->FindComponentByClass<USceneComponent>();
				UStaticMeshComponent* GhostMeshComp = CurrentGhostActor->FindComponentByClass<UStaticMeshComponent>();
				
				if (BuildingMeshComp && SceneRootComp && GhostMeshComp)
				{
					UStaticMesh* TargetMesh = nullptr;

					// 데이터 에셋이 등록되어 있고, 메시 정보가 있다면 최우선으로 가져옵니다.
					if (BuildingCDO->BuildingData && BuildingCDO->BuildingData->BuildingMesh)
					{
						TargetMesh = BuildingCDO->BuildingData->BuildingMesh;
					}
					else
					{
						// 만약 에셋이 비어있다면 컴포넌트의 기본 메시를 가져옵니다.
						TargetMesh = BuildingMeshComp->GetStaticMesh();
					}

					if (TargetMesh)
					{
						CurrentGhostActor->InitGhost(TargetMesh, GhostMaterialBase);

						// DA의 BuildingScale 적용
						const float DAScale = BuildingCDO->BuildingData ? BuildingCDO->BuildingData->BuildingScale : 1.f;
						CurrentGhostActor->SetActorScale3D(FVector(DAScale));

						// BVBuildingBase::OnConstruction과 동일한 피벗 보정 (바닥 + X/Y 중앙) + Yaw
						const FBox MeshLocalBox = TargetMesh->GetBoundingBox();
						const FVector MeshCenter = MeshLocalBox.GetCenter();
						const FVector MeshPivotOffset(-MeshCenter.X, -MeshCenter.Y, -MeshLocalBox.Min.Z);
						const float YawOffset = BuildingCDO->BuildingData ? BuildingCDO->BuildingData->BuildingYaw : 0.f;
						GhostMeshComp->SetRelativeLocation(MeshPivotOffset);
						GhostMeshComp->SetRelativeRotation(FRotator(0.f, YawOffset, 0.f));
					}
				}
			}
		}
	}

	// 슬롯 좌클릭으로 진입한 경우 같은 마우스 이벤트가 Build IMC로 흘러들어 즉시 착공되는 걸 방지.
	// 시간 창(BuildClickInputIgnoreWindow) 이내의 OnBuildClick만 무시 — 사용자의 실제 다음 클릭은 통과.
	BuildModeEnterTime = GetWorld() ? GetWorld()->GetTimeSeconds() : -1.0;
}

void ABVPlayerController::EnterCityBuildMode(ABVCityBase* City, TSubclassOf<ABVBuildingBase> InBuildingClass, float InConstructionTime)
{
	// City-only 빌드: City가 없으면 빌드 진입 자체를 거부. (Hero 경로는 제거됨.)
	if (!City)
	{
		return;
	}

	// (선택 가드) 해당 도시가 BuildableBuildings에 이 클래스를 갖고 있어야 빌드 허용.
	// 패널 UI에서 이미 필터링되겠지만, 콘솔/스크립트 호출 대비.
	if (City->BuildableBuildings.Num() > 0 && !City->BuildableBuildings.Contains(InBuildingClass))
	{
		return;
	}

	// 기존 ghost/IMC 셋업을 그대로 재사용 (즉시-건설 분기는 OnBuildClick에서 처리).
	EnterConstructionMode(InBuildingClass, InConstructionTime);

	// 셋업이 성공해 빌드 모드가 켜진 경우에만 도시 상태/데칼을 활성화.
	if (bIsConstructionMode)
	{
		CurrentBuildCity = City;
		City->SetBuildRadiusVisualVisible(true);

		// ghost 배치 모드 동안에도 거점 선택 상태(DetailBuilding + hover ring)는 유지하되,
		// 패널 자체는 잠시 숨김 (화면 가림 방지). 착공/취소 후 다시 표시됨.
		ShowBuildingDetail(City);
		if (CityDetailWidget)
		{
			CityDetailWidget->SetVisibility(ESlateVisibility::Hidden);
		}
	}
}

void ABVPlayerController::ExitConstructionMode()
{
	bIsConstructionMode = false;
	BuildModeEnterTime = -1.0;

	// City build mode 였으면 반경 데칼을 끄고 도시 참조를 해제.
	if (ABVCityBase* City = CurrentBuildCity.Get())
	{
		City->SetBuildRadiusVisualVisible(false);
	}
	CurrentBuildCity.Reset();

	// Switch to Normal IMC
	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		if (BuildMappingContext)
		{
			Subsystem->RemoveMappingContext(BuildMappingContext);
		}
	}

	// Turn off the Ghost Actor
	if (CurrentGhostActor)
	{
		CurrentGhostActor->Destroy();
		CurrentGhostActor = nullptr;
	}
}

void ABVPlayerController::UpdateGhostLocation()
{
	if (!CurrentGhostActor) return;

	FHitResult Hit;
	bool bHit = GetHitResultUnderCursor(ECC_Visibility, false, Hit);

	if (bHit)
	{
		FVector TargetLoc = Hit.ImpactPoint;
		CurrentGhostActor->SetActorLocation(TargetLoc);

		// 기본은 항상 build 가능. City 빌드 모드면 도시 반경 안일 때만 허용.
		bool bAllowed = true;
		if (ABVCityBase* City = CurrentBuildCity.Get())
		{
			bAllowed = City->IsLocationWithinBuildRadius(TargetLoc);
		}

		// TODO : 추가 검증 (충돌, 슬로프 등)
		bCanBuild = bAllowed;
		CurrentGhostActor->SetValid(bCanBuild);
	}
}

void ABVPlayerController::PlayAnnouncerVoice(EBVAnnouncerEvent EventType)
{
	const float CurrentTime = GetWorld()->GetTimeSeconds();

	// Legacy: BaseUnderAttack은 기존 전용 쿨다운 유지
	if (EventType == EBVAnnouncerEvent::BaseUnderAttack)
	{
		if (CurrentTime - LastBaseAttackVoiceTime < BaseAttackVoiceCooldown)
		{
			return;
		}
		LastBaseAttackVoiceTime = CurrentTime;
	}
	else
	{
		// 이벤트별 쿨다운 맵에 등록된 이벤트면 간격 체크
		if (const float* Cooldown = AnnouncerCooldowns.Find(EventType))
		{
			const float* LastTime = LastAnnouncerPlayTime.Find(EventType);
			if (LastTime && CurrentTime - *LastTime < *Cooldown)
			{
				return;
			}
			LastAnnouncerPlayTime.Add(EventType, CurrentTime);
		}
	}

	// If not, then find the appropriate announcer sound according to the event type
	if (TObjectPtr<USoundBase>* FoundSound = AnnouncerVoices.Find(EventType))
	{
		if (*FoundSound)
		{
			UGameplayStatics::PlaySound2D(this, *FoundSound, AnnouncerVolumeMultiplier);
		}
	}
}

void ABVPlayerController::ShowGoldReward(int32 Amount, FVector WorldLocation)
{
	if (!GoldPopupWidgetClass) return;

	// Widget
	UUserWidget* NewWidget = CreateWidget<UUserWidget>(this, GoldPopupWidgetClass);
	if (NewWidget)
	{
		if (UBVGoldPopupWidget* GoldWidget = Cast<UBVGoldPopupWidget>(NewWidget))
		{
			GoldWidget->InitPopup(Amount, WorldLocation);
		}

		NewWidget->AddToViewport();
		
	}

	// Gold Pickup Sound
	if (GoldPickupSound)
	{
		UGameplayStatics::PlaySound2D(this, GoldPickupSound, GoldPickupSoundVolume, 1.5f);
	}
}

void ABVPlayerController::OpenShopUI(ABVNPCBase* TargetNPC)
{
	if (!ShopWidget && ShopWidgetClass)
	{
		ShopWidget = CreateWidget<UUserWidget>(this, ShopWidgetClass);
		if (ShopWidget)
		{
			ShopWidget->AddToViewport();
			ShopWidget->SetVisibility(ESlateVisibility::Hidden);
		}
	}
	
	if (ShopWidget)
	{
		if (ShopWidget->GetVisibility() == ESlateVisibility::Visible) return;

		CloseCurrentUI(); 
		UBVShopWidget* MyShopWidget = Cast<UBVShopWidget>(ShopWidget);
		if (MyShopWidget)
		{
			MyShopWidget->InitShop(TargetNPC);
		}
		
		ShopWidget->SetVisibility(ESlateVisibility::Visible);
		
		FInputModeGameAndUI InputMode;
		InputMode.SetWidgetToFocus(ShopWidget->TakeWidget());
		SetInputMode(InputMode);
		bShowMouseCursor = true;
	}
}

void ABVPlayerController::CloseShopUI()
{
	if (ShopWidget && ShopWidget->GetVisibility() == ESlateVisibility::Visible)
	{
		ShopWidget->SetVisibility(ESlateVisibility::Hidden);

		FInputModeGameAndUI InputMode;
		InputMode.SetHideCursorDuringCapture(false);
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		SetInputMode(InputMode);
	}
}

// ======================= Box Selection =======================

void ABVPlayerController::OnSelectPressed()
{
	// 기존 박스 셀렉션 먼저 해제
	ClearBoxSelection();

	bIsDragSelecting = true;
	bDragMovedBeyondThreshold = false;

	float MX = 0.f, MY = 0.f;
	if (GetMousePosition(MX, MY))
	{
		DragStartScreenPos = FVector2D(MX, MY);
		DragCurrentScreenPos = DragStartScreenPos;
	}
}

void ABVPlayerController::OnSelectReleased()
{
	const bool bWasDrag = bIsDragSelecting && bDragMovedBeyondThreshold;

	bIsDragSelecting = false;
	bDragMovedBeyondThreshold = false;

	if (bWasDrag)
	{
		// 박스 셀렉션에 정확히 1개만 걸렸으면 해당 대상을 선택 처리.
		if (BoxSelectedActors.Num() == 1)
		{
			AActor* OnlyActor = BoxSelectedActors[0].Get();
			if (AMainCharacter* Hero = Cast<AMainCharacter>(OnlyActor))
			{
				HideBuildingDetail();
				HideUnitDetail();
				SetHeroSelected(Hero);
			}
			else if (ABVAutobotBase* Unit = Cast<ABVAutobotBase>(OnlyActor))
			{
				ClearHeroSelection();
				HideBuildingDetail();
				ShowUnitDetail(Unit);
			}
			else if (ABVBuildingBase* Building = Cast<ABVBuildingBase>(OnlyActor))
			{
				ClearHeroSelection();
				HideUnitDetail();
				ShowBuildingDetail(Building);
			}
		}
		else
		{
			// 여러 개 선택 → 기존 상세 패널 닫기 (Hero 선택은 유지).
			HideBuildingDetail();
			HideUnitDetail();
		}
		return;
	}

	// 짧은 클릭 → 기존 단일 셀렉트 동작
	SelectObject();
}

void ABVPlayerController::UpdateBoxHover()
{
	// 박스 사각형 계산 (스크린 좌표)
	const FVector2D Min(FMath::Min(DragStartScreenPos.X, DragCurrentScreenPos.X),
	                    FMath::Min(DragStartScreenPos.Y, DragCurrentScreenPos.Y));
	const FVector2D Max(FMath::Max(DragStartScreenPos.X, DragCurrentScreenPos.X),
	                    FMath::Max(DragStartScreenPos.Y, DragCurrentScreenPos.Y));

	// 현재 박스 내부에 있는 액터 수집 (유닛 + 건물)
	TArray<TWeakObjectPtr<AActor>> NewSet;

	const FGenericTeamId MyTeamId = GetGenericTeamId();

	auto TestAndAdd = [&](AActor* A)
	{
		if (!A) return;
		if (!A->Implements<UBVDamageableInterface>()) return;

		// 죽은 유닛 / 파괴된 건물 제외
		if (IBVDamageableInterface::Execute_IsDestroyed(A)) return;
		if (ABVAutobotBase* Bot = Cast<ABVAutobotBase>(A))
		{
			if (Bot->bIsDead) return;
		}

		// 아군만 선택 — 팀이 없거나(NoTeam) 다른 팀이면 제외
		if (const IGenericTeamAgentInterface* TeamAgent = Cast<IGenericTeamAgentInterface>(A))
		{
			const FGenericTeamId OtherId = TeamAgent->GetGenericTeamId();
			if (OtherId == FGenericTeamId::NoTeam) return;
			if (OtherId != MyTeamId) return;
		}
		else
		{
			return;
		}

		FVector2D Screen;
		if (!ProjectWorldLocationToScreen(A->GetActorLocation(), Screen)) return;
		if (Screen.X < Min.X || Screen.X > Max.X) return;
		if (Screen.Y < Min.Y || Screen.Y > Max.Y) return;

		NewSet.Emplace(A);
	};

	UWorld* World = GetWorld();
	if (!World) return;

	for (TActorIterator<ABVAutobotBase> It(World); It; ++It)
	{
		TestAndAdd(*It);
	}
	for (TActorIterator<ABVBuildingBase> It(World); It; ++It)
	{
		TestAndAdd(*It);
	}
	for (TActorIterator<AMainCharacter> It(World); It; ++It)
	{
		TestAndAdd(*It);
	}

	// 이전 박스에 있었지만 이번엔 빠진 액터 → un-hover
	for (const TWeakObjectPtr<AActor>& Old : BoxSelectedActors)
	{
		AActor* OldActor = Old.Get();
		if (!OldActor) continue;

		bool bStillIn = false;
		for (const TWeakObjectPtr<AActor>& N : NewSet)
		{
			if (N.Get() == OldActor) { bStillIn = true; break; }
		}
		if (!bStillIn)
		{
			if (OldActor != HoveredObject &&
				OldActor != DetailBuilding.Get() &&
				OldActor != SelectedHero.Get())
			{
				if (OldActor->Implements<UBVDamageableInterface>())
				{
					IBVDamageableInterface::Execute_SetHovered(OldActor, false);
				}
			}
		}
	}

	// 이번 박스에 들어온 액터 → hover ON
	for (const TWeakObjectPtr<AActor>& N : NewSet)
	{
		if (AActor* A = N.Get())
		{
			if (A->Implements<UBVDamageableInterface>())
			{
				IBVDamageableInterface::Execute_SetHovered(A, true);
			}
		}
	}

	BoxSelectedActors = NewSet;
}

void ABVPlayerController::ClearBoxSelection()
{
	for (const TWeakObjectPtr<AActor>& W : BoxSelectedActors)
	{
		AActor* A = W.Get();
		if (!A) continue;
		if (A == HoveredObject) continue;
		if (A == DetailBuilding.Get()) continue;
		if (A == SelectedHero.Get()) continue;
		if (A->Implements<UBVDamageableInterface>())
		{
			IBVDamageableInterface::Execute_SetHovered(A, false);
		}
	}
	BoxSelectedActors.Reset();
}
