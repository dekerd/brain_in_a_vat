// Fill out your copyright notice in the Description page of Project Settings.


#include "BVPlayerController.h"

#include "BVRTSCameraPawn.h"
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
#include "Data/BVBuildingData.h"
#include "Engine/Canvas.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetRenderingLibrary.h"
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
		APawn* ControlledPawn = GetPawn();
		if (ControlledPawn)
		{
			float Distance = FVector::Dist2D(ControlledPawn->GetActorLocation(), TargetBuildLocation);

			if (Distance <= StartToBuildRange)
			{
				StopMovement();

				AMainCharacter* MainCharacter = Cast<AMainCharacter>(ControlledPawn);
				if (MainCharacter && PendingBuildingClass)
				{
					MainCharacter->ConstructBuilding(TargetBuildLocation, PendingBuildingClass, PendingConstructionTime);
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
		if (HoveredObject)
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

	// Fog of War
	UpdateFogOfWar();
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
		}
}

void ABVPlayerController::MoveToLocation(const FInputActionValue& Value)
{

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
				AMainCharacter* MyCharacter = Cast<AMainCharacter>(GetPawn());
				if (MyCharacter)
				{
					float Distance = FVector::Dist(MyCharacter->GetActorLocation(), ClickedNPC->GetActorLocation());
					float InteractableDistance = 300.0f;

					if (Distance <= InteractableDistance)
					{
						ClickedNPC->Interact(MyCharacter);
						GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Red, TEXT("NPC 상호작용 성공!"));
					}
					else
					{
						GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Red, TEXT("NPC가 너무 멀리 있습니다!"));
					}
				}
			}
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

	// Exit Ghost Building Mode if active
	if (bIsConstructionMode)
	{
		ExitConstructionMode();
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
		if (AMainCharacter* MyChar = Cast<AMainCharacter>(GetPawn()))
		{
			if (UCapsuleComponent* Capsule = MyChar->GetCapsuleComponent())
			{
				CharacterRadius = Capsule->GetScaledCapsuleRadius();
			}
		}

		StartToBuildRange = BuildingRadius + CharacterRadius + 100.0f; // Padding

		
		// Moving to construction site
		bIsMovingToBuild = true;
		UAIBlueprintHelperLibrary::SimpleMoveToLocation(this, TargetBuildLocation);
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

void ABVPlayerController::UpdateFogOfWar()
{
	// 에셋이 할당되지 않았거나 조종할 캐릭터가 없으면 리턴
	if (!RT_Discovered || !RT_Vision || !VisionBrushMaterial || !HeroCharacter) return;

	// 1. 현재 시야용 RT는 매 프레임 까맣게 초기화합니다. (탐사 기록용 RT는 초기화하지 않음!)
	UKismetRenderingLibrary::ClearRenderTarget2D(this, RT_Vision, FLinearColor::Black);

	UCanvas* Canvas;
	FVector2D Size;
	FDrawToRenderTargetContext ContextVision;
	FDrawToRenderTargetContext ContextDiscovered;

	// 캐릭터의 월드 위치를 가져옵니다.
	FVector CharLoc = HeroCharacter->GetActorLocation();

	// 월드 좌표를 렌더 타겟의 UV 좌표(0~1)를 거쳐 픽셀 좌표로 변환합니다.
	// (MapCenter 기준으로 MapSize 크기만큼의 맵이라고 가정)
	float NormalizedX = ((CharLoc.X - MapCenter.X) / MapSize) + 0.5f;
	float NormalizedY = ((CharLoc.Y - MapCenter.Y) / MapSize) + 0.5f;

	// 렌더 타겟에 그리기 시작
	UKismetRenderingLibrary::BeginDrawCanvasToRenderTarget(this, RT_Vision, Canvas, Size, ContextVision);
	if (Canvas)
	{
		FVector2D DrawPos(NormalizedX * Size.X, NormalizedY * Size.Y);
		
		// 시야 반경 역시 맵 크기 비율에 맞춰 렌더 타겟 사이즈로 변환
		float BrushSize = (VisionRadius / MapSize) * Size.X;

		// M_VisionBrush를 도장 찍듯이 그려줍니다.
		Canvas->K2_DrawMaterial(
			VisionBrushMaterial,
			DrawPos - FVector2D(BrushSize / 2.0f), // 브러시의 중앙을 맞추기 위해 절반만큼 이동
			FVector2D(BrushSize, BrushSize),
			FVector2D::ZeroVector, FVector2D::UnitVector, 0.0f, FVector2D(0.5f, 0.5f)
		);
	}
	UKismetRenderingLibrary::EndDrawCanvasToRenderTarget(this, ContextVision);

	// 동일한 방식으로 탐사 기록용 RT에도 그려줍니다. (단, 여긴 초기화를 안 하므로 누적됩니다)
	UKismetRenderingLibrary::BeginDrawCanvasToRenderTarget(this, RT_Discovered, Canvas, Size, ContextDiscovered);
	if (Canvas)
	{
		FVector2D DrawPos(NormalizedX * Size.X, NormalizedY * Size.Y);
		float BrushSize = (VisionRadius / MapSize) * Size.X;

		Canvas->K2_DrawMaterial(
			VisionBrushMaterial,
			DrawPos - FVector2D(BrushSize / 2.0f),
			FVector2D(BrushSize, BrushSize),
			FVector2D::ZeroVector, FVector2D::UnitVector, 0.0f, FVector2D(0.5f, 0.5f)
		);
	}
	UKismetRenderingLibrary::EndDrawCanvasToRenderTarget(this, ContextDiscovered);
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
						CurrentGhostActor->SetActorScale3D(SceneRootComp->GetRelativeScale3D());
						GhostMeshComp->SetRelativeTransform(BuildingMeshComp->GetRelativeTransform());
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
	// Check cooltime if the event is BaseUnderAttack
	if (EventType == EBVAnnouncerEvent::BaseUnderAttack)
	{
		float CurrentTime = GetWorld()->GetTimeSeconds();
		if (CurrentTime - LastBaseAttackVoiceTime < BaseAttackVoiceCooldown)
		{
			return; 
		}
		LastBaseAttackVoiceTime = CurrentTime;
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
