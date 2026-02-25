// Fill out your copyright notice in the Description page of Project Settings.


#include "Characters/BVNPCBase.h"

#include "BVPlayerController.h"
#include "MainCharacter.h"


// Sets default values
ABVNPCBase::ABVNPCBase()
{
	PrimaryActorTick.bCanEverTick = false;

	GetMesh()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	
}

void ABVNPCBase::Interact(class AMainCharacter* Interactor)
{
	FString DebugMsg = FString::Printf(TEXT("%s 와(과) 상호작용 했습니다!"), *GetName());
	GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Green, DebugMsg);

	PlayInteractionSound();

	if (ABVPlayerController* BVPC = Cast<ABVPlayerController>(Interactor->GetController()))
	{
		BVPC->OpenShopUI(this);
	}
}

// Called when the game starts or when spawned
void ABVNPCBase::BeginPlay()
{
	Super::BeginPlay();
	
}


