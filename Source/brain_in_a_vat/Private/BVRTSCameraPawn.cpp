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
	CameraBoom->TargetArmLength = 6000.0f; // TargetZoom 기본값과 일치시켜 시작 시 Tick 보간 튐 방지.
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

FVector ABVRTSCameraPawn::ComputeFocusWorldOffset() const
{
	if (!CameraBoom || FMath::IsNearlyZero(FocusScreenOffsetRatio))
	{
		return FVector::ZeroVector;
	}

	const APlayerController* PC = Cast<APlayerController>(GetController());
	int32 VX = 1920, VY = 1080;
	if (PC) PC->GetViewportSize(VX, VY);
	const float Aspect = (VY > 0) ? (static_cast<float>(VX) / static_cast<float>(VY)) : (16.f / 9.f);
	const float HFOVDeg = CameraComponent ? CameraComponent->FieldOfView : 90.f;
	const float HalfVFOVRad = FMath::Atan(FMath::Tan(FMath::DegreesToRadians(HFOVDeg) * 0.5f) / Aspect);

	// 화면 중앙에서 위로 FocusScreenOffsetRatio 만큼 떨어진 점의 각도.
	// 정규화 y (화면 반높이 기준) = 2 * ratio. tan(angle) = y_norm * tan(halfVFOV).
	const float AngleUpRad = FMath::Atan(2.f * FocusScreenOffsetRatio * FMath::Tan(HalfVFOVRad));

	const float PitchRad = FMath::DegreesToRadians(CameraBoom->GetRelativeRotation().Pitch);
	const float ArmLength = CameraBoom->TargetArmLength;

	// Camera 위치(pawn 원점 기준). Spring arm은 forward 반대 방향으로 ArmLength 만큼 떨어진 소켓.
	const float CameraX = -FMath::Cos(PitchRad) * ArmLength;
	const float CameraZ = -FMath::Sin(PitchRad) * ArmLength;

	// 화면 상단 쪽 ray(카메라 forward에서 pitch를 위로 AngleUpRad만큼 더한 방향).
	const float RayPitch = PitchRad + AngleUpRad;
	const float RayX = FMath::Cos(RayPitch);
	const float RayZ = FMath::Sin(RayPitch);
	if (FMath::Abs(RayZ) < KINDA_SMALL_NUMBER) return FVector::ZeroVector;

	const float T = -CameraZ / RayZ; // pawn 평면과의 교차까지 거리.
	const float GroundHitX = CameraX + RayX * T;

	FVector ForwardGround = CameraBoom->GetForwardVector();
	ForwardGround.Z = 0.f;
	if (!ForwardGround.Normalize()) return FVector::ZeroVector;
	return ForwardGround * GroundHitX;
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

	// 2. Jump 진행 — 등속 linear interp.
	if (bIsJumping)
	{
		// 타겟이 움직이는 actor(CenterOnActor로 진입한 경우)면 매 틱 타겟 위치 갱신.
		// 그래야 Hero가 이동 중이어도 Jump 종료 시 Hero 위치와 정확히 일치.
		if (AActor* Follow = TargetToFollow)
		{
			JumpTargetLocation = Follow->GetActorLocation() - ComputeFocusWorldOffset();
			JumpTargetLocation.Z = JumpStartLocation.Z;
		}

		JumpElapsed += DeltaTime;
		const float Alpha = FMath::Clamp(JumpElapsed / FMath::Max(JumpDuration, KINDA_SMALL_NUMBER), 0.f, 1.f);
		SetActorLocation(FMath::Lerp(JumpStartLocation, JumpTargetLocation, Alpha));

		if (Alpha >= 1.f)
		{
			bIsJumping = false;
		}
		return; // jump 중에는 tracking/edge-scroll 로직 중단.
	}

	// 3. 캐릭터 추적 로직 또는 엣지 스크롤 로직
	if (bIsTracking && TargetToFollow)
	{
		FVector CurrentLoc = GetActorLocation();
		FVector TargetLoc = TargetToFollow->GetActorLocation() - ComputeFocusWorldOffset();
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
		bIsJumping = false; // 유저가 직접 조작하면 jump 취소.

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
	if (!TargetActor) return;

	// Jump interp로 부드럽게 이동. Jump가 끝나면 bIsJumping=false가 되고
	// Tick의 tracking 루프(bIsTracking=true)가 이어서 actor를 따라간다.
	JumpStartLocation = GetActorLocation();
	JumpTargetLocation = TargetActor->GetActorLocation() - ComputeFocusWorldOffset();
	JumpTargetLocation.Z = JumpStartLocation.Z;
	JumpElapsed = 0.f;
	bIsJumping = true;

	TargetToFollow = TargetActor;
	bIsTracking = true;
}

void ABVRTSCameraPawn::JumpToLocation(const FVector& WorldLocation)
{
	// 캐릭터 추적과 이전 jump를 취소하고 이번 jump를 시작.
	bIsTracking = false;
	TargetToFollow = nullptr;

	JumpStartLocation = GetActorLocation();

	// Z는 유지(카메라 Rig의 높이는 그대로 둠).
	JumpTargetLocation = WorldLocation - ComputeFocusWorldOffset();
	JumpTargetLocation.Z = JumpStartLocation.Z;

	JumpElapsed = 0.f;
	bIsJumping = true;
}

void ABVRTSCameraPawn::ZoomCamera(float ZoomValue)
{
	if (ZoomValue != 0.0f)
	{
		// 마우스 휠을 위로(양수) 굴리면 줌 인(거리 감소)이 되도록 빼줍니다.
		TargetZoom = FMath::Clamp(TargetZoom - (ZoomValue * ZoomStep), MinZoom, MaxZoom);
	}
}

