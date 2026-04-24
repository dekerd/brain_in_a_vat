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
#include "Components/BVStaticHoverRingComponent.h"


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
	// 유닛이 다른 액터의 호버 링 데칼을 몸에 받지 않도록 차단.
	GetMesh()->SetReceivesDecals(false);

	// <------------ Hover Ring ------------>
	// StaticMesh 기반 링. 캐릭터 Transform에 1:1 attach되어 잔상 없음.
	StaticHoverRingComponent = CreateDefaultSubobject<UBVStaticHoverRingComponent>(TEXT("StaticHoverRingComponent"));
	StaticHoverRingComponent->SetupAttachment(RootComponent);

	// <------------ Widgets ------------>
	// UnitNameWidget
	/*
	UnitNameWidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("UnitNameWidgetComponent"));
	static ConstructorHelpers::FClassFinder<UBVUnitNameWidget> UnitNameWidgetRef(TEXT("/Script/UMGEditor.WidgetBlueprint'/Game/HUD/Widget/WBP_UnitNameWidget.WBP_UnitNameWidget_C'"));
	if (UnitNameWidgetRef.Class != nullptr)
	{
		UnitNameWidgetClass = UnitNameWidgetRef.Class;
		UnitNameWidgetComponent->SetWidgetClass(UnitNameWidgetClass);
	}
	UnitNameWidgetComponent->SetupAttachment(RootComponent);
	UnitNameWidgetComponent->SetRelativeLocation(FVector(0.0f, 0.0f, 50.f));
	UnitNameWidgetComponent->SetWidgetSpace(EWidgetSpace::Screen);
	*/
}

void ABVCharacterBase::BeginPlay()
{
	Super::BeginPlay();

	// Disable Jitter Effect of Hovering
	UKismetSystemLibrary::ExecuteConsoleCommand(GetWorld(), TEXT("r.CustomDepthTemporalAAJitter 0"));

	GetMesh()->SetCollisionProfileName(TEXT("Hoverable"));

	// 스켈레탈 메시는 애니메이션으로 미묘하게 움직여서 커서 트레이스가 간헐적으로 놓침 → 호버 깜빡임.
	// 캡슐도 MouseHover를 블록하게 해서 메시 애니메이션과 무관하게 안정적으로 잡히도록.
	if (UCapsuleComponent* Cap = GetCapsuleComponent())
	{
		Cap->SetCollisionResponseToChannel(ECC_MouseHover, ECR_Block);
	}

	// 유닛 전체가 다른 호버 링 데칼을 받지 않도록 일괄 차단.
	// (메인 메시 외에 무기/부착 메시가 있어도 커버. BP에서 우연히 켜진 경우도 덮어씀.)
	TArray<UPrimitiveComponent*> AllPrims;
	GetComponents<UPrimitiveComponent>(AllPrims);
	for (UPrimitiveComponent* Prim : AllPrims)
	{
		if (Prim)
		{
			Prim->SetReceivesDecals(false);
		}
	}

	// StaticHoverRingComponent의 크기/위치는 자체 BeginPlay에서 캡슐 기반으로 자동 계산됨.

	/*
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
	*/
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
	// StaticMesh 버전만 실제 렌더링. Decal 버전은 호출하지 않아 visibility=false 유지.
	if (StaticHoverRingComponent)
	{
		StaticHoverRingComponent->SetHovered(bInHovered, TeamType);
	}
}

void ABVCharacterBase::PlayInteractionSound()
{
}

