// Fill out your copyright notice in the Description page of Project Settings.


#include "Item/BVItemActor.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "MainCharacter.h"
#include "Item/BVItemData.h"

// Sets default values
ABVItemActor::ABVItemActor()
{

	SphereComponent = CreateDefaultSubobject<USphereComponent>(TEXT("SphereComponent"));
	RootComponent = SphereComponent;
	SphereComponent->InitSphereRadius(100.f);
	SphereComponent->SetCollisionProfileName(TEXT("Trigger"));

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	MeshComponent->SetupAttachment(RootComponent);
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	
}

// Called when the game starts or when spawned
void ABVItemActor::BeginPlay()
{
	Super::BeginPlay();

	if (ItemData && ItemData->PickupMesh)
	{
		MeshComponent->SetStaticMesh(ItemData->PickupMesh);
	}

	SphereComponent->OnComponentBeginOverlap.AddDynamic(this, &ABVItemActor::OnOverlapBegin);
	
}

void ABVItemActor::OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{

	if (AMainCharacter* Player = Cast<AMainCharacter>(OtherActor))
	{
		Destroy();
		
		/* TODO : Implement Player->AddItemToInventory
		if (Player->AddItemToInventory(ItemData))
		{
			Destroy();
		}
		*/
	}
	
}
