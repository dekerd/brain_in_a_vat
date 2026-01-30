// Fill out your copyright notice in the Description page of Project Settings.


#include "Characters/BVCharacterBase.h"

#include "Components/CapsuleComponent.h"
#include "BVPlayerState.h"
#include "Animations/BVAnimInstance.h"
#include "Collision/BVCollision.h"
#include "Components/WidgetComponent.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Widget/BVHealthBarWidget.h"
#include "Widget/BVUnitNameWidget.h"
#include "BVPlayerController.h"
#include "Kismet/GameplayStatics.h"


// Sets default values
ABVCharacterBase::ABVCharacterBase()
{

	// Capsule
	GetCapsuleComponent()->InitCapsuleSize(30.f, 42.0f);

	// Mesh and Collision
	float CapsuleHalfHeight = GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight();
	GetMesh()->SetRelativeLocationAndRotation(FVector(0.0f, 0.0f, -CapsuleHalfHeight), FRotator(0.0f, -90.0f, 0.0f));
	GetMesh()->SetAnimationMode(EAnimationMode::AnimationBlueprint);
	GetMesh()->SetCollisionProfileName(TEXT("Hoverable"));
	
	// <------------ Widgets ------------>
	// UnitNameWidget
	UnitNameWidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("UnitNameWidgetComponent"));
	static ConstructorHelpers::FClassFinder<UBVUnitNameWidget> UnitNameWidgetRef(TEXT("/Script/UMGEditor.WidgetBlueprint'/Game/HUD/Widget/WBP_UnitNameWidget.WBP_UnitNameWidget_C'"));
	if (UnitNameWidgetRef.Class != nullptr)
	{
		UnitNameWidgetClass = UnitNameWidgetRef.Class;
		UnitNameWidgetComponent->SetWidgetClass(UnitNameWidgetClass);
	}
	UnitNameWidgetComponent->SetupAttachment(RootComponent);
	UnitNameWidgetComponent->SetWidgetSpace(EWidgetSpace::Screen);
}

void ABVCharacterBase::BeginPlay()
{
	Super::BeginPlay();

	// Disable Jitter Effect of Hovering
	UKismetSystemLibrary::ExecuteConsoleCommand(GetWorld(), TEXT("r.CustomDepthTemporalAAJitter 0"));

	GetMesh()->SetCollisionProfileName(TEXT("Hoverable"));

	// Widget Setting
	if (UnitNameWidgetComponent)
	{
		UUserWidget* WidgetObject = UnitNameWidgetComponent->GetUserWidgetObject();
		if (WidgetObject)
		{
			UBVUnitNameWidget* NameWidget = Cast<UBVUnitNameWidget>(WidgetObject);
			if (NameWidget)
			{
				NameWidget->SetUnitName(UnitName);
			}
		}
	}
}

void ABVCharacterBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

FGenericTeamId ABVCharacterBase::GetTeamId_Implementation() const
{
	return GetGenericTeamId();
}

void ABVCharacterBase::SetHovered_Implementation(bool bInHovered)
{
	bIsHovered = bInHovered;
	if (USkeletalMeshComponent* CharacterMesh = GetMesh())
	{
		uint8 Stencil = 0;

		if (bIsHovered)
		{

			APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
			IGenericTeamAgentInterface* TeamAgentPC = Cast<IGenericTeamAgentInterface>(PC);

			if (TeamAgentPC)
			{
				ETeamAttitude::Type Attitude = TeamAgentPC->GetTeamAttitudeTowards((*this));

				switch (Attitude)
				{
				case ETeamAttitude::Friendly:
					Stencil = 1;
					break;
				case ETeamAttitude::Hostile:
					Stencil = 2;
					break;
				case ETeamAttitude::Neutral:
					Stencil = 3;
					break;
				default:
					Stencil = 0;
					break;
				}
			}

		}
		
		CharacterMesh->SetRenderCustomDepth(bIsHovered);
		CharacterMesh->SetCustomDepthStencilValue(Stencil);
		
		// FString DebugMsg = FString::Printf(TEXT("[%s] is hovered! Stencil : %d"), *GetName(), Stencil);
		// GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Orange, DebugMsg);
	}
}

void ABVCharacterBase::PlayInteractionSound()
{
}

