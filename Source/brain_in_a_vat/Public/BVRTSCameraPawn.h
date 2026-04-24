// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "BVRTSCameraPawn.generated.h"

UCLASS()
class BRAIN_IN_A_VAT_API ABVRTSCameraPawn : public APawn
{
	GENERATED_BODY()

public:
	// Sets default values for this pawn's properties
	ABVRTSCameraPawn();
	virtual void Tick(float DeltaTime) override;

	void MoveCamera(FVector2D PanDirection);
	void CenterOnActor(AActor* TargetActor);
	void ZoomCamera(float ZoomValue);

	// 특정 월드 위치로 순간 이동(팔로우 해제). Space+1 같은 "전투 현장 보기" 용도.
	void JumpToLocation(const FVector& WorldLocation);

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere)
	class USceneComponent* RootSceneComponent;

	UPROPERTY(VisibleAnywhere)
	class USpringArmComponent* CameraBoom;

	UPROPERTY(VisibleAnywhere)
	class UCameraComponent* CameraComponent;

	UPROPERTY(EditAnywhere, Category = "Camera")
	float CameraPanSpeed = 2000.0f;

	UPROPERTY(EditAnywhere, Category = "Camera")
	float EdgeScrollMargin = 20.0f;

	UPROPERTY(EditAnywhere, Category = "Camera")
	float CameraInterpSpeed = 10.0f;

	UPROPERTY(EditAnywhere, Category = "Camera")
	bool bEnableEdgeScroll = true;

	UPROPERTY(EditAnywhere, Category = "Camera|Zoom")
	float TargetZoom = 2000.0f; // 목표 줌(스프링 암 길이)

	UPROPERTY(EditAnywhere, Category = "Camera|Zoom")
	float MinZoom = 5000.0f; // 최대 줌인 거리

	UPROPERTY(EditAnywhere, Category = "Camera|Zoom")
	float MaxZoom = 7000.0f; // 최대 줌아웃 거리

	UPROPERTY(EditAnywhere, Category = "Camera|Zoom")
	float ZoomStep = 200.0f; // 휠 1칸당 줌 이동량

	UPROPERTY(EditAnywhere, Category = "Camera|Zoom")
	float ZoomInterpSpeed = 10.0f; // 부드러운 줌 보간 속도

	UPROPERTY()
	AActor* TargetToFollow;

	bool bIsTracking = false;

	// Jump interpolation — JumpToLocation을 등속 이동으로.
	UPROPERTY(EditAnywhere, Category = "Camera|Jump", meta=(ClampMin="0.05", ClampMax="2.0"))
	float JumpDuration = 0.2f;

	bool bIsJumping = false;
	FVector JumpStartLocation = FVector::ZeroVector;
	FVector JumpTargetLocation = FVector::ZeroVector;
	float JumpElapsed = 0.f;

	void HandleEdgePanning(float DeltaTime);
};
