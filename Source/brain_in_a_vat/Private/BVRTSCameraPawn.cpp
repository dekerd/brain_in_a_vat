// Fill out your copyright notice in the Description page of Project Settings.


#include "BVRTSCameraPawn.h"

#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"


// Sets default values
ABVRTSCameraPawn::ABVRTSCameraPawn()
{
	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	RootSceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootSceneComponent"));
	RootComponent = RootSceneComponent;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->bDoCollisionTest = false;
	CameraBoom->TargetArmLength = 5000.0f;
	CameraBoom->SetRelativeRotation(FRotator(-65.f, 0.f, 0.f));
	CameraBoom->bInheritPitch = false;
	CameraBoom->bInheritYaw = false;
	CameraBoom->bInheritRoll = false;

	CameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("CameraComponent"));
	CameraComponent->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
}

// Called when the game starts or when spawned
void ABVRTSCameraPawn::BeginPlay()
{
	Super::BeginPlay();
	
}

void ABVRTSCameraPawn::HandleEdgePanning(float DeltaTime)
{
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC) return;

	int32 ViewportSizeX, ViewportSizeY;
	PC->GetViewportSize(ViewportSizeX, ViewportSizeY);

	float MouseX, MouseY;
	if (!PC->GetMousePosition(MouseX, MouseY)) return;

	FVector2D PanDirection = FVector2D::ZeroVector;

	if (MouseX <= EdgeScrollMargin) PanDirection.X = -1.f;
	else if (MouseX >= ViewportSizeX - EdgeScrollMargin) PanDirection.X = 1.f;

	if (MouseY <= EdgeScrollMargin) PanDirection.Y = 1.f;
	else if (MouseY >= ViewportSizeY - EdgeScrollMargin) PanDirection.Y = -1.f;

	if (!PanDirection.IsNearlyZero())
	{
		PanDirection.Normalize();
		MoveCamera(PanDirection);
	}
}

// Called every frame
void ABVRTSCameraPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// 1. 카메라 줌을 부드럽게(Interpolation) 적용
	CameraBoom->TargetArmLength = FMath::FInterpTo(CameraBoom->TargetArmLength, TargetZoom, DeltaTime, ZoomInterpSpeed);

	// 2. 캐릭터 추적 로직 또는 엣지 스크롤 로직
	if (bIsTracking && TargetToFollow)
	{
		FVector CurrentLoc = GetActorLocation();
		FVector TargetLoc = TargetToFollow->GetActorLocation();
		SetActorLocation(FMath::VInterpTo(CurrentLoc, TargetLoc, DeltaTime, CameraInterpSpeed));
	}
	else if (bEnableEdgeScroll)
	{
		HandleEdgePanning(DeltaTime);
	}
}

void ABVRTSCameraPawn::MoveCamera(FVector2D PanDirection)
{
	if (PanDirection.X != 0.f || PanDirection.Y != 0.f)
	{
		bIsTracking = false;

		FVector Forward = CameraBoom->GetForwardVector();
		Forward.Z = 0.f;
		Forward.Normalize();

		FVector Right = CameraBoom->GetRightVector();
		Right.Z = 0.f;
		Right.Normalize();

		FVector MoveDelta = (Forward * PanDirection.Y + Right * PanDirection.X) * CameraPanSpeed * GetWorld()->GetDeltaSeconds();
		AddActorWorldOffset(MoveDelta);
	}
}

void ABVRTSCameraPawn::CenterOnActor(AActor* TargetActor)
{
	if (TargetActor)
	{
		TargetToFollow = TargetActor;
		bIsTracking = true;
		SetActorLocation(TargetActor->GetActorLocation());
	}
}

void ABVRTSCameraPawn::JumpToLocation(const FVector& WorldLocation)
{
	// 캐릭터 추적을 해제하고 지정된 위치로 즉시 이동
	bIsTracking = false;
	TargetToFollow = nullptr;

	// Z는 유지(카메라 Rig의 높이는 그대로 둠)
	FVector NewLoc = WorldLocation;
	NewLoc.Z = GetActorLocation().Z;
	SetActorLocation(NewLoc);
}

void ABVRTSCameraPawn::ZoomCamera(float ZoomValue)
{
	if (ZoomValue != 0.0f)
	{
		// 마우스 휠을 위로(양수) 굴리면 줌 인(거리 감소)이 되도록 빼줍니다.
		TargetZoom = FMath::Clamp(TargetZoom - (ZoomValue * ZoomStep), MinZoom, MaxZoom);
	}
}

