// Fill out your copyright notice in the Description page of Project Settings.


#include "BVPlayerController.h"
#include "Autobots/BVAutobotBase.h"
#include "Collision/BVCollision.h"
#include "InputMappingContext.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Blueprint/AIBlueprintHelperLibrary.h"
#include "Widget/BVInventoryWidget.h"

ABVPlayerController::ABVPlayerController()
{
	bShowMouseCursor = true;
	DefaultMouseCursor = EMouseCursor::Default;

	HoveredObject = nullptr;

	// Character Input

	static ConstructorHelpers::FObjectFinder<UInputMappingContext> InputMappingContextRef(TEXT("/Script/EnhancedInput.InputMappingContext'/Game/Inputs/IMC_Player.IMC_Player'"));
	if (InputMappingContextRef.Succeeded())
	{
		InputMappingContext = InputMappingContextRef.Object;
	}

	static ConstructorHelpers::FObjectFinder<UInputAction> MoveActionRef(TEXT("/Script/EnhancedInput.InputAction'/Game/Inputs/IA_RightClickMove.IA_RightClickMove'"));
	if (MoveActionRef.Succeeded())
	{
		MoveAction = MoveActionRef.Object;
	}

	static ConstructorHelpers::FObjectFinder<UInputAction> SelectActionRef(TEXT("/Script/EnhancedInput.InputAction'/Game/Inputs/IA_Select.IA_Select'"));
	if (SelectActionRef.Succeeded())
	{
		SelectAction = SelectActionRef.Object;
	}

	// [Widgets]
	// Inventory Widget
	static ConstructorHelpers::FClassFinder<UBVInventoryWidget> WidgetClassFinder(TEXT("/Script/UMGEditor.WidgetBlueprint'/Game/HUD/Widget/InventoryWidget/WBP_InventoryWidget.WBP_InventoryWidget_C'"));
	if (WidgetClassFinder.Succeeded())
	{
		InventoryWidgetClass = WidgetClassFinder.Class;
	}
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

	// <------------------ Widgets ------------------>
	// Inventory Widget
	if (InventoryWidgetClass)
	{
		InventoryWidget = CreateWidget<UBVInventoryWidget>(this, InventoryWidgetClass);
		if (InventoryWidget)
		{
			InventoryWidget->AddToViewport();
			InventoryWidget->RefreshInventory();
		}
	}
	
}

void ABVPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);

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
	}

	HoveredObject = NewHitActor;

	if (HoveredObject)
	{
		if (HoveredObject->Implements<UBVDamageableInterface>())
		{
			IBVDamageableInterface::Execute_SetHovered(HoveredObject, true);
		}
	}
}

void ABVPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

		if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent))
		{
			// Right Click -> Move
			EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Started, this, &ABVPlayerController::MoveToLocation);
			// Left Click -> Select
			EnhancedInputComponent->BindAction(SelectAction, ETriggerEvent::Started, this, &ABVPlayerController::SelectObject);
		}
	
}

void ABVPlayerController::MoveToLocation(const FInputActionValue& Value)
{
	FHitResult Hit;
	if (GetHitResultUnderCursor(ECC_Visibility, false, Hit))
	{
		const FVector DestLocation = Hit.ImpactPoint;

		if (APawn* ControlledPawn = GetPawn())
		{
			DrawDebugSphere(GetWorld(), DestLocation, 25.0f, 12, FColor::Red, false, 1.0f);
			UAIBlueprintHelperLibrary::SimpleMoveToLocation(this, DestLocation);
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
		}
	}
}
