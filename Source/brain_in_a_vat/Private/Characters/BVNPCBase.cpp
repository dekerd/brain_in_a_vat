// Fill out your copyright notice in the Description page of Project Settings.


#include "Characters/BVNPCBase.h"
#include "BVPlayerController.h"
#include "MainCharacter.h"
#include "Kismet/GameplayStatics.h"


// Sets default values
ABVNPCBase::ABVNPCBase()
{
	PrimaryActorTick.bCanEverTick = false;

	GetMesh()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	
}

void ABVNPCBase::Interact(class AMainCharacter* Interactor)
{
	PlayInteractionSound();

	if (ABVPlayerController* BVPC = Cast<ABVPlayerController>(UGameplayStatics::GetPlayerController(GetWorld(), 0)))
	{
		BVPC->OpenShopUI(this);
	}
}

// Called when the game starts or when spawned
void ABVNPCBase::BeginPlay()
{
	Super::BeginPlay();
	
}


