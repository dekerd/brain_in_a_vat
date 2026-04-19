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
#include "Widget/BVBuildingDetailWidget.h"
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

	// Construction Ghost
	if (bIsConstructionMode)
	{
		UpdateGhostLocation();
	}

	// Moving To Construction Site
	if (bIsMovingToBuild)
	{
		if (HeroCharacter)
		{
			float Distance = FVector::Dist2D(HeroCharacter->GetActorLocation(), TargetBuildLocation);

			if (Distance <= StartToBuildRange)
			{
				// Stop HeroCharacter's movement
				if (AController* HeroController = HeroCharacter->GetController())
				{
					HeroController->StopMovement();
				}

				if (PendingBuildingClass)
				{
					HeroCharacter->ConstructBuilding(TargetBuildLocation, PendingBuildingClass, PendingConstructionTime);
				}

				if (CurrentGhostActor)
				{
					CurrentGhostActor->Destroy();
					CurrentGhostActor = nullptr;
				}

				bIsMovingToBuild = false;
				PendingBuildingClass = nullptr;
			}
		}
	}

	// Mouse Hovering
	FHitResult Hit;
	bool bHit = GetHitResultUnderCursor(ECC_MouseHover, false, Hit);
	AActor* NewHitActor = bHit ? Hit.GetActor() : nullptr;

	if (NewHitActor != HoveredObject)
	{
		// 커서가 벗어나도, '선택된 건물'은 호버 이펙트 유지
		if (HoveredObject && HoveredObject != DetailBuilding.Get())
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
	if (BuildingDetailWidget && BuildingDetailWidget->GetVisibility() == ESlateVisibility::Visible)
	{
		if (ABVBuildingBase* DetailB = DetailBuilding.Get())
		{
			if (DetailB->bIsDestroyed)
			{
				HideBuildingDetail();
			}
			else
			{
				// 1) 프로그레스 갱신
				if (UBVBuildingDetailWidget* DetailW = Cast<UBVBuildingDetailWidget>(BuildingDetailWidget))
				{
					float Ratio = 0.f;
					if (DetailB->RespawnInterval > 0.f && DetailB->SpawnUnitClass)
					{
						Ratio = FMath::Fmod(DetailB->ElapsedTime, DetailB->RespawnInterval) / DetailB->RespawnInterval;
					}
					DetailW->SetRespawnProgress(Ratio);
				}

				// 2) 건물 월드 위치 -> 위젯(슬레이트) 좌표 변환해 위젯 위치 갱신
				FVector2D WidgetPos;
				if (UWidgetLayoutLibrary::ProjectWorldLocationToWidgetPosition(
						this, DetailB->GetActorLocation(), WidgetPos, false))
				{
					// 스크린 픽셀 기준 단순 오프셋: 건물 중심에서 오른쪽으로 약간 띄우기
					const FVector2D PanelOffset(60.f, -30.f);
					BuildingDetailWidget->SetPositionInViewport(WidgetPos + PanelOffset, false);
					BuildingDetailWidget->SetVisibility(ESlateVisibility::Visible);
				}
				else
				{
					// 건물이 카메라 뒤/밖이면 숨김
					BuildingDetailWidget->SetVisibility(ESlateVisibility::Hidden);
				}
			}
		}
		else
		{
			HideBuildingDetail();
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

	if (OtherTeamId.GetId() == 255)
	{
		return ETeamAttitude::Neutral;
	}

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
			
			// [Normal Mode]
			if (MoveAction)
			{
				// Right Click -> Move
				EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Started, this, &ABVPlayerController::MoveToLocation);
			}

			if (SelectAction)
			{
				// Left Click -> Select
				EnhancedInputComponent->BindAction(SelectAction, ETriggerEvent::Started, this, &ABVPlayerController::SelectObject);
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
	// 우클릭 = 건물 선택 해제
	HideBuildingDetail();

	if (bIsMovingToBuild || bIsConstructionMode)
	{
		// Cancel the construction if the player was going to the construction site
		bIsMovingToBuild = false;
		bIsConstructionMode = false;
		PendingBuildingClass = nullptr;

		ExitConstructionMode();

		if (CurrentGhostActor)
		{
			CurrentGhostActor->Destroy();
			CurrentGhostActor = nullptr;
		}
	}
	
	FHitResult Hit;
	if (GetHitResultUnderCursor(ECC_Visibility, false, Hit))
	{
		const FVector DestLocation = Hit.ImpactPoint;
		if (HeroCharacter)
		{
			DrawDebugSphere(GetWorld(), DestLocation, 25.0f, 12, FColor::Red, false, 1.0f);
			
			// HeroCharacter의 컨트롤러(AI Controller)를 가져와 이동 명령을 내립니다.
			if (AController* HeroController = HeroCharacter->GetController())
			{
				UAIBlueprintHelperLibrary::SimpleMoveToLocation(HeroController, DestLocation);
			}
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
			UE_LOG(LogTemp, Log, TEXT("Selected: %s"), *SelectedActor->GetName())

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

			// 건물 클릭 시 상세 패널 토글 (같은 건물이면 닫기, 다른 건물이면 전환)
			if (ABVBuildingBase* ClickedBuilding = Cast<ABVBuildingBase>(SelectedActor))
			{
				if (DetailBuilding.Get() == ClickedBuilding)
				{
					HideBuildingDetail();
				}
				else
				{
					ShowBuildingDetail(ClickedBuilding);
				}
			}
			else
			{
				HideBuildingDetail();
			}
		}
		else
		{
			// 빈 공간 클릭 -> 상세 패널 닫기
			HideBuildingDetail();
		}
	}
}

void ABVPlayerController::OnBuildKeyPressed()
{
	if (bIsConstructionMode)
	{
		ExitConstructionMode();
	}
	else
	{
		ToggleConstructionMenuUI();
	}
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

void ABVPlayerController::ToggleConstructionMenuUI()
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

	// 최초 1회만 위젯 생성
	if (!BuildingDetailWidget && BuildingDetailWidgetClass)
	{
		BuildingDetailWidget = CreateWidget<UUserWidget>(this, BuildingDetailWidgetClass);
		if (BuildingDetailWidget)
		{
			BuildingDetailWidget->AddToViewport();
			BuildingDetailWidget->SetVisibility(ESlateVisibility::Hidden);
		}
	}

	if (!BuildingDetailWidget) return;

	DetailBuilding = InBuilding;

	// 선택된 건물은 호버 이펙트를 강제로 켜둔다 (커서가 벗어나도 유지)
	if (InBuilding->Implements<UBVDamageableInterface>())
	{
		IBVDamageableInterface::Execute_SetHovered(InBuilding, true);
	}

	if (UBVBuildingDetailWidget* DetailW = Cast<UBVBuildingDetailWidget>(BuildingDetailWidget))
	{
		DetailW->SetFromBuilding(InBuilding);
	}

	BuildingDetailWidget->SetVisibility(ESlateVisibility::Visible);

	// 오픈 즉시 한 번 위치를 잡아둠 (다음 틱부터 PlayerTick에서 이어서 갱신)
	FVector2D WidgetPos;
	if (UWidgetLayoutLibrary::ProjectWorldLocationToWidgetPosition(
			this, InBuilding->GetActorLocation(), WidgetPos, false))
	{
		const FVector2D PanelOffset(60.f, -30.f);
		BuildingDetailWidget->SetPositionInViewport(WidgetPos + PanelOffset, false);
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

	if (BuildingDetailWidget && BuildingDetailWidget->GetVisibility() == ESlateVisibility::Visible)
	{
		BuildingDetailWidget->SetVisibility(ESlateVisibility::Hidden);
	}
}

void ABVPlayerController::OnBuildClick()
{
	// This function is called under Build IMC
	if (bCanBuild && CurrentGhostActor)
	{
		PendingBuildingClass = CurrentBuildingClass;
		TargetBuildLocation = CurrentGhostActor->GetActorLocation();

		float BuildingRadius = 50.0f; 
		FVector Origin, Extent;
		CurrentGhostActor->GetActorBounds(false, Origin, Extent);
		BuildingRadius = FMath::Max(Extent.X, Extent.Y);

		float CharacterRadius = 42.0f;
		if (HeroCharacter)
		{
			if (UCapsuleComponent* Capsule = HeroCharacter->GetCapsuleComponent())
			{
				CharacterRadius = Capsule->GetScaledCapsuleRadius();
			}
		}

		StartToBuildRange = BuildingRadius + CharacterRadius + 100.0f; // Padding

		
		// Moving to construction site
		bIsMovingToBuild = true;
		if (HeroCharacter)
		{
			if (AController* HeroController = HeroCharacter->GetController())
			{
				UAIBlueprintHelperLibrary::SimpleMoveToLocation(HeroController, TargetBuildLocation);
			}
		}
		bIsConstructionMode = false;

		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
		{
			if (BuildMappingContext)
			{
				Subsystem->RemoveMappingContext(BuildMappingContext);
			}
		}

		PlayAnnouncerVoice(EBVAnnouncerEvent::ConstructionStarted);
	}
	else
	{
		// TODO : Can't Build there
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
	UE_LOG(LogTemp, Warning, TEXT("[Camera] FocusLastCombat pressed. bHasLastCombatLocation=%d, Loc=%s"),
		bHasLastCombatLocation ? 1 : 0, *LastCombatLocation.ToString());

	if (!bHasLastCombatLocation)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Camera] FocusLastCombat: 아직 기록된 전투 위치가 없습니다."));
		return;
	}

	ABVRTSCameraPawn* CamPawn = Cast<ABVRTSCameraPawn>(GetPawn());
	if (!CamPawn)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Camera] FocusLastCombat: GetPawn()이 ABVRTSCameraPawn이 아님 (Pawn=%s)"),
			GetPawn() ? *GetPawn()->GetName() : TEXT("NULL"));
		return;
	}

	CamPawn->JumpToLocation(LastCombatLocation);
}

void ABVPlayerController::ReportCombatLocation(const FVector& WorldLocation)
{
	LastCombatLocation = WorldLocation;
	bHasLastCombatLocation = true;
	UE_LOG(LogTemp, Verbose, TEXT("[Camera] ReportCombatLocation: %s"), *WorldLocation.ToString());
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
	
}

void ABVPlayerController::ExitConstructionMode()
{
	bIsConstructionMode = false;

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

		// TODO : Construction validation
		
		bCanBuild = true;
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
