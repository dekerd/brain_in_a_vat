// Fill out your copyright notice in the Description page of Project Settings.


#include "Buildings/BVBuildingGhost.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"

// Sets default values
ABVBuildingGhost::ABVBuildingGhost()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRootComponent"));
	RootComponent = SceneRootComponent;
	
	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	MeshComponent->SetupAttachment(SceneRootComponent);

	MeshComponent->SetCollisionProfileName(TEXT("NoCollision"));
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	MeshComponent->SetCastShadow(false);
	// ghost는 빌드 반경 데칼이 덮지 않도록 차단.
	MeshComponent->SetReceivesDecals(false);
}

void ABVBuildingGhost::InitGhost(UStaticMesh* InMesh, UMaterialInterface* InGhostMaterial)
{
	if (MeshComponent && InMesh)
	{
		MeshComponent->SetStaticMesh(InMesh);
	}

	if (InGhostMaterial)
	{
		GhostMaterialInstance = UMaterialInstanceDynamic::Create(InGhostMaterial, this);

		if (MeshComponent && GhostMaterialInstance)
		{
			int32 MatNum = MeshComponent->GetNumMaterials();
			for (int32 i=0; i<MatNum; i++)
			{
				MeshComponent->SetMaterial(i, GhostMaterialInstance);
			}
		}
	}
}

void ABVBuildingGhost::SetValid(bool bIsValid)
{
	if (GhostMaterialInstance)
	{
		FLinearColor TargetColor = bIsValid ? FLinearColor::Green : FLinearColor::Red;
		TargetColor.A = 0.5f;
		GhostMaterialInstance->SetVectorParameterValue(TEXT("Color"), TargetColor);
	}
}

