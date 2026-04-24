// Fill out your copyright notice in the Description page of Project Settings.


#include "Item/BVItemActor.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "GameFramework/RotatingMovementComponent.h"
#include "MainCharacter.h"
#include "Item/BVItemData.h"
#include "Collision/BVCollision.h"

// Sets default values
ABVItemActor::ABVItemActor()
{

	PrimaryActorTick.bCanEverTick = true;

	// Sphere Component
	SphereComponent = CreateDefaultSubobject<USphereComponent>(TEXT("SphereComponent"));
	RootComponent = SphereComponent;
	SphereComponent->InitSphereRadius(100.f);
	SphereComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	SphereComponent->SetGenerateOverlapEvents(true);
	SphereComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
	SphereComponent->SetCollisionObjectType(ECC_Item);
	SphereComponent->SetCanEverAffectNavigation(false);
	
	SphereComponent->SetCollisionResponseToChannel(ECC_MouseHover, ECR_Block);
	SphereComponent->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Overlap);
	SphereComponent->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	SphereComponent->SetCollisionResponseToChannel(ECC_Player, ECR_Overlap);
	SphereComponent->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	
	// Mesh
	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	MeshComponent->SetupAttachment(RootComponent);
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	// 아이템 메시는 월드 데칼(City 빌드 반경 등)을 받지 않음. 지면에만 깔리게.
	MeshComponent->SetReceivesDecals(false);

	// Auto-Rotation Component
	RotatingComponent = CreateDefaultSubobject<URotatingMovementComponent>(TEXT("RotatingMovementComponent"));
	RotatingComponent->RotationRate = FRotator(0.f, RotationRate, 0.f);
	
}

// Called when the game starts or when spawned
void ABVItemActor::BeginPlay()
{
	Super::BeginPlay();

	InitialRelativeLocation = MeshComponent->GetRelativeLocation();
	// AddUniqueDynamic: BeginPlay가 어떤 이유로 중복 호출되어도 같은 핸들러가 두 번 등록되지 않도록 가드.
	// 일반 AddDynamic은 SparseDynamicDelegate에서 중복 등록 시 ensure를 발생시킨다.
	SphereComponent->OnComponentBeginOverlap.AddUniqueDynamic(this, &ABVItemActor::OnOverlapBegin);
}

void ABVItemActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	if (ItemData && ItemData->PickupMesh)
	{
		MeshComponent->SetStaticMesh(ItemData->PickupMesh);

		FBox MeshBounds = ItemData->PickupMesh->GetBoundingBox();
		FVector Extent = MeshBounds.GetExtent();

		float NewRadius = Extent.GetMax();
		if (SphereComponent)
		{
			SphereComponent->SetSphereRadius(NewRadius + 10.f);
		}
	}
	
}

void ABVItemActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	RunningTime += DeltaTime;
	float DeltaZ = FMath::Sin(RunningTime * BobbingFrequency) * BobbingAmplitude;

	FVector NewLocation = InitialRelativeLocation;
	NewLocation.Z += DeltaZ;

	MeshComponent->SetRelativeLocation(NewLocation);
}

void ABVItemActor::SetHovered_Implementation(bool bInHovered)
{
	if (MeshComponent)
	{
		MeshComponent->SetRenderCustomDepth(bInHovered);
		MeshComponent->SetCustomDepthStencilValue(bInHovered? 1:0);
	}
}

void ABVItemActor::OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
                                  UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (AMainCharacter* Player = Cast<AMainCharacter>(OtherActor))
	{
		Player->AddItemToInventory(ItemData);
		Destroy();
	}
}
