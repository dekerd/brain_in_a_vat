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

FGenericTeamId ABVCharacterBase::GetGenericTeamId() const
{
	if (const IGenericTeamAgentInterface* TeamAgent = Cast<IGenericTeamAgentInterface>(GetController()))
	{
		return TeamAgent->GetGenericTeamId();
	}
	return IGenericTeamAgentInterface::GetGenericTeamId();
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
			const bool bIsEnemy = (TeamFlag != 1);
			Stencil = bIsEnemy ? 2 : 1;
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

