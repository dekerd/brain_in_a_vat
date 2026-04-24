#include "Components/BVStaticHoverRingComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Kismet/GameplayStatics.h"
#include "GenericTeamAgentInterface.h"
#include "GameFramework/Character.h"
#include "Components/CapsuleComponent.h"
#include "Components/BoxComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"
#include "Settings/BVHoverRingSettings.h"

UBVStaticHoverRingComponent::UBVStaticHoverRingComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

	// 엔진 기본 Plane (XY 평면, 100x100 units = 1m).
	static ConstructorHelpers::FObjectFinder<UStaticMesh> PlaneRef(
		TEXT("/Engine/BasicShapes/Plane.Plane"));
	if (PlaneRef.Succeeded())
	{
		SetStaticMesh(PlaneRef.Object);
	}

	// 기본 머티리얼 — 사용자가 M_HoverRing (Surface/Unlit/Translucent) 제작 후 BP에서 override.
	HoverRingMaterial = TSoftObjectPtr<UMaterialInterface>(
		FSoftObjectPath(TEXT("/Game/Materials/M_HoverRing.M_HoverRing")));

	SetVisibility(false);
	SetHiddenInGame(true);
	SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetCastShadow(false);
	SetReceivesDecals(false);
	bSelectable = false;
}

void UBVStaticHoverRingComponent::BeginPlay()
{
	Super::BeginPlay();

	// Project Settings → "Brain in a Vat" → "Hover Ring"의 전역 값을 컴포넌트에 주입.
	// 한 곳에서 수정하면 모든 캐릭터/유닛/건물에 일괄 반영된다.
	if (const UBVHoverRingSettings* Settings = GetDefault<UBVHoverRingSettings>())
	{
		if (!Settings->HoverRingMaterial.IsNull())
		{
			HoverRingMaterial = Settings->HoverRingMaterial;
		}
		RingIntensityParamName = Settings->RingIntensityParamName;
		RingTintParamName      = Settings->RingTintParamName;
		RingThicknessParamName = Settings->RingThicknessParamName;
		RingWorldThickness     = Settings->RingWorldThickness;
		bInstantToggle         = Settings->bInstantToggle;
		HoverEaseSpeed         = Settings->HoverEaseSpeed;
		FriendlyColor          = Settings->FriendlyColor;
		HostileColor           = Settings->HostileColor;
		NeutralColor           = Settings->NeutralColor;
		CapsulePadding         = Settings->CapsulePadding;
		BoxPadding             = Settings->BoxPadding;
		GroundZOffset          = Settings->GroundZOffset;
	}

	if (!bAutoSizeFromCapsule) return;

	AActor* OwnerActor = GetOwner();
	if (!OwnerActor) return;

	// 액터 scale 역보정을 위해 월드 기준으로 계산한 뒤 SetWorldScale3D로 고정.
	// (액터 scale이 10배여도 링은 의도한 월드 크기로 그려지게.)
	const FVector ActorScale = OwnerActor->GetActorScale3D();
	const float ActorScaleXY = FMath::Max(FMath::Max(FMath::Abs(ActorScale.X), FMath::Abs(ActorScale.Y)), KINDA_SMALL_NUMBER);
	const float ActorScaleZ  = FMath::Max(FMath::Abs(ActorScale.Z), KINDA_SMALL_NUMBER);

	float WorldDiameter = 0.f;
	float BottomOffsetWorld = 0.f; // 액터 위치 기준, 바닥 면까지의 월드 Z 오프셋.

	// 1순위: ACharacter의 CapsuleComponent (유닛/영웅).
	if (ACharacter* OwnerCharacter = Cast<ACharacter>(OwnerActor))
	{
		if (UCapsuleComponent* Cap = OwnerCharacter->GetCapsuleComponent())
		{
			const float HalfHeight = Cap->GetUnscaledCapsuleHalfHeight();
			const float Radius = Cap->GetUnscaledCapsuleRadius();
			WorldDiameter = Radius * 2.f * ActorScaleXY * CapsulePadding;
			BottomOffsetWorld = -HalfHeight * ActorScaleZ;
		}
	}

	// 2순위: BoxComponent (건물/거점). 크기만 사용. Z는 Actor 위치 그대로 (pivot이 바닥이라고 가정).
	if (WorldDiameter <= 0.f)
	{
		if (UBoxComponent* Box = OwnerActor->FindComponentByClass<UBoxComponent>())
		{
			const FVector Extent = Box->GetUnscaledBoxExtent();
			const float MaxSide = FMath::Max(Extent.X, Extent.Y) * 2.f;
			if (MaxSide > KINDA_SMALL_NUMBER)
			{
				WorldDiameter = MaxSide * ActorScaleXY * BoxPadding;
				// BottomOffsetWorld = 0: 건물 pivot이 메시 바닥이라고 가정. 다른 높이가 필요한 BP는
				// GroundZOffset으로 미세 조정하거나 SceneRoot relative location을 바꾸면 됨.
				BottomOffsetWorld = 0.f;
			}
		}
	}

	if (WorldDiameter <= 0.f) return;

	// 월드 두께 → UV 비율 변환용으로 캐시.
	CachedWorldDiameter = WorldDiameter;

	// 엔진 Plane 기본 100cm × ScaleXY = 링 지름 (월드). SetWorldScale3D로 parent scale 무시.
	const float ScaleXY = WorldDiameter / 100.f;
	SetUsingAbsoluteScale(true);
	SetWorldScale3D(FVector(ScaleXY, ScaleXY, 1.f));

	// RelativeLocation도 parent scale 영향 받으므로 actor scale Z로 역보정.
	const float LocalZ = (BottomOffsetWorld + GroundZOffset) / ActorScaleZ;
	SetRelativeLocation(FVector(0.f, 0.f, LocalZ));
}

void UBVStaticHoverRingComponent::SetHovered(bool bInHovered, EBVTeam OwnerTeam)
{
	HoverTargetValue = bInHovered ? 1.f : 0.f;
	if (!bInHovered) return;

	EBVTeam ViewerTeam = EBVTeam::Neutral;
	if (APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0))
	{
		if (IGenericTeamAgentInterface* TeamAgent = Cast<IGenericTeamAgentInterface>(PC))
		{
			ViewerTeam = static_cast<EBVTeam>(TeamAgent->GetGenericTeamId().GetId());
		}
	}

	if (OwnerTeam == EBVTeam::Neutral)
	{
		HoverTintColor = NeutralColor;
	}
	else if (OwnerTeam == ViewerTeam)
	{
		HoverTintColor = FriendlyColor;
	}
	else
	{
		HoverTintColor = HostileColor;
	}

	if (!HoverRingMID && !HoverRingMaterial.IsNull())
	{
		if (UMaterialInterface* Src = HoverRingMaterial.LoadSynchronous())
		{
			HoverRingMID = UMaterialInstanceDynamic::Create(Src, this);
			SetMaterial(0, HoverRingMID);

			// 월드 두께 → UV 비율. 모든 크기의 링이 동일한 월드 두께로 보이게.
			const float UVThickness = RingWorldThickness / FMath::Max(CachedWorldDiameter, KINDA_SMALL_NUMBER);
			HoverRingMID->SetScalarParameterValue(RingThicknessParamName, UVThickness);
		}
	}
}

void UBVStaticHoverRingComponent::TickComponent(
	float DeltaTime,
	ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	const float Prev = HoverCurrentValue;
	if (bInstantToggle)
	{
		HoverCurrentValue = HoverTargetValue;
	}
	else
	{
		HoverCurrentValue = FMath::FInterpTo(HoverCurrentValue, HoverTargetValue, DeltaTime, HoverEaseSpeed);
	}

	if (HoverCurrentValue <= KINDA_SMALL_NUMBER && Prev <= KINDA_SMALL_NUMBER) return;

	const bool bShouldShow = HoverCurrentValue > 0.01f && HoverRingMID != nullptr;
	if (IsVisible() != bShouldShow)
	{
		SetVisibility(bShouldShow);
		SetHiddenInGame(!bShouldShow);
	}

	if (bShouldShow && HoverRingMID)
	{
		HoverRingMID->SetScalarParameterValue(RingIntensityParamName, HoverCurrentValue);
		HoverRingMID->SetVectorParameterValue(RingTintParamName, HoverTintColor);
		// 월드 두께 → UV 비율. 실시간 튜닝 지원.
		const float UVThickness = RingWorldThickness / FMath::Max(CachedWorldDiameter, KINDA_SMALL_NUMBER);
		HoverRingMID->SetScalarParameterValue(RingThicknessParamName, UVThickness);
	}
}
