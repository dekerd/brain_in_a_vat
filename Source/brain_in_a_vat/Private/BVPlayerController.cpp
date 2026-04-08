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

	// Fog of War
	if (RT_Discovered)
	{
		UKismetRenderingLibrary::ClearRenderTarget2D(this, RT_Discovered, FLinearColor::Black);
	}

	// 0.15초마다 한 번씩(초당 약 6.6회) UpdateFogOfWar를 실행합니다.
	// GetWorldTimerManager().SetTimer(FogTimerHandle, this, &ABVPlayerController::UpdateFogOfWar, 0.1f, true);
	
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

	// Fog of war
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
			
			float BrushSize = (Provider.Radius / MapSize) * SizeVision.X;
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
			
			float BrushSize = (Provider.Radius / MapSize) * SizeDiscovered.X;
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

			// 위젯 끄기/켜기
			if (It->OverheadWidgetComponent)
			{
				It->OverheadWidgetComponent->SetVisibility(bIsVisible);
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

			// 위젯 끄기/켜기
			if (It->OverheadWidgetComponent)
			{
				It->OverheadWidgetComponent->SetVisibility(bIsVisible);
			}
			
			It->SetActorHiddenInGame(!bIsVisible); 
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
