// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BVBuildingGhost.generated.h"

UCLASS()
class BRAIN_IN_A_VAT_API ABVBuildingGhost : public AActor
{
	GENERATED_BODY()

public:
	ABVBuildingGhost();

	void InitGhost(UStaticMesh* InMesh, UMaterialInterface* InGhostMaterial);
	
	void SetValid(bool bIsValid);

protected:

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<class UStaticMeshComponent> MeshComponent;

	UPROPERTY()
	TObjectPtr<class UMaterialInstanceDynamic> GhostMaterialInstance;
};
