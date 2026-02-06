// Fill out your copyright notice in the Description page of Project Settings.


#include "BVPlayerController.h"
#include "Characters/BVAutobotBase.h"
#include "Collision/BVCollision.h"
#include "InputMappingContext.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "MainCharacter.h"
#include "Blueprint/AIBlueprintHelperLibrary.h"
#include "Buildings/BVBuildingBase.h"
#include "Buildings/BVBuildingGhost.h"
#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Widget/BVGoldPopupWidget.h"
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

	static ConstructorHelpers::FObjectFinder<UInputAction> BuildActionRef(TEXT("/Script/EnhancedInput.InputAction'/Game/Inputs/IA_Build.IA_Build'"));
	if (BuildActionRef.Succeeded())
	{
		BuildAction = BuildActionRef.Object;
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
	// MainHUDWidget
	if (MainHUDWidgetClass)
	{
		MainHUDWidget = CreateWidget<UUserWidget>(this, MainHUDWidgetClass);
		if (MainHUDWidget)
		{
			MainHUDWidget->AddToViewport();
		}
	}
	
	/*
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

	// Resource Widget
	if (ResourceHUDClass)
	{
		ResourceHUD = CreateWidget<UUserWidget>(this, ResourceHUDClass);
		if (ResourceHUD)
		{
			UE_LOG(LogTemp, Warning, TEXT("Resource widget added!!"));
			ResourceHUD->AddToViewport(10);
		}
	}
	*/

	// <------------------ BGM ------------------>
	if (BGMPlaylist.Num() > 0)
	{
		int32 RandomIndex = FMath::RandRange(0, BGMPlaylist.Num() - 1);
		if (BGMPlaylist[RandomIndex])
		{

			float SoundDuration = BGMPlaylist[RandomIndex]->GetDuration();
			float RandomStartTime = FMath::FRandRange(0.f, SoundDuration / 2);
			BGMComponent = UGameplayStatics::SpawnSound2D(this, BGMPlaylist[RandomIndex],Volume, 1.0f, 0.0f, nullptr, true);
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
			// Right Click -> Move
			EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Started, this, &ABVPlayerController::MoveToLocation);
			// Left Click -> Select
			EnhancedInputComponent->BindAction(SelectAction, ETriggerEvent::Started, this, &ABVPlayerController::OnBuildClick);
			// B -> Build
			EnhancedInputComponent->BindAction(BuildAction, ETriggerEvent::Started, this, &ABVPlayerController::OnBuildKeyPressed);
		}
	
}

void ABVPlayerController::MoveToLocation(const FInputActionValue& Value)
{
	FHitResult Hit;
	if (GetHitResultUnderCursor(ECC_Visibility, false, Hit))
	{
		const FVector DestLocation = Hit.ImpactPoint;
		if (CurrentBuildingClass)
		{
			AMainCharacter* MyCharacter = Cast<AMainCharacter>(GetPawn());
			if (MyCharacter)
			{
				MyCharacter->ConstructBuilding(DestLocation, CurrentBuildingClass);

				CurrentBuildingClass = nullptr;
				return;
			}
		}

		if (APawn* ControlledPawn = GetPawn())
		{
			DrawDebugSphere(GetWorld(), DestLocation, 25.0f, 12, FColor::Red, false, 1.0f);
			UAIBlueprintHelperLibrary::SimpleMoveToLocation(this, DestLocation);

			/* Move Sound
			if (AMainCharacter* MainCharacter = Cast<AMainCharacter>(ControlledPawn))
			{
				MainCharacter->PlayRandomMoveSound();
			}
			*/
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

void ABVPlayerController::EnterConstructionMode(TSubclassOf<ABVBuildingBase> InBuildingClass)
{
	CurrentBuildingClass = InBuildingClass;
}

void ABVPlayerController::OnBuildKeyPressed()
{
	FString DebugMsg = FString::Printf(TEXT("B key is pressed!!"));
	GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Red, DebugMsg);

	if (bIsConstructionMode)
	{
		ExitConstructionMode();
	}
	else
	{
		EnterConstructionMode();
	}

	/*
	AMainCharacter* MyCharacter = Cast<AMainCharacter>(GetPawn());
	if (!MyCharacter) return;
	if (!DefaultBuildingClass) return;

	FVector BuildLocation = MyCharacter->GetActorLocation();
	MyCharacter->ConstructBuilding(BuildLocation, DefaultBuildingClass);
	*/
	
}

void ABVPlayerController::OnBuildClick()
{
	if (bIsConstructionMode)
	{
		if (bCanBuild && CurrentGhostActor)
		{
			AMainCharacter* MainCharacter = Cast<AMainCharacter>(GetPawn());
			if (MainCharacter && DefaultBuildingClass)
			{
				MainCharacter->ConstructBuilding(CurrentGhostActor->GetActorLocation(), DefaultBuildingClass);
				ExitConstructionMode();
			}
		}
		else
		{
			// TODO : Can't Build there
		}
	}
	else
	{
		SelectObject();
	}
	
}

void ABVPlayerController::EnterConstructionMode()
{
	if (!DefaultBuildingClass || !GhostActorClass) return;

	bIsConstructionMode = true;

	// 1. First, spawn a building actor 
	if (UWorld* World = GetWorld())
	{
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		CurrentGhostActor = World->SpawnActor<ABVBuildingGhost>(GhostActorClass, FVector::ZeroVector, FRotator::ZeroRotator, Params);

		if (CurrentGhostActor)
		{
			// 2. Add Ghost material to the spawned building
			ABVBuildingBase* BuildingCDO = DefaultBuildingClass->GetDefaultObject<ABVBuildingBase>();
			if (BuildingCDO)
			{
				UStaticMeshComponent* BuildingMeshComp = BuildingCDO->FindComponentByClass<UStaticMeshComponent>();
				if (BuildingMeshComp)
				{
					CurrentGhostActor->InitGhost(BuildingMeshComp->GetStaticMesh(), GhostMaterialBase);
				}
			}
		}
	}
	
}

void ABVPlayerController::ExitConstructionMode()
{
	bIsConstructionMode = false;
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
