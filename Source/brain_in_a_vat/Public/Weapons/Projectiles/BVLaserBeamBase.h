// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BVProjectileBase.h"
#include "BVLaserBeamBase.generated.h"

UCLASS()
class BRAIN_IN_A_VAT_API ABVLaserBeamBase : public ABVProjectileBase
{
	GENERATED_BODY()

public:

	ABVLaserBeamBase();

	void InitBeamEnd(const FVector& StartLocation, const FVector& EndLocation);

protected:
	
	UPROPERTY(VisibleAnywhere, Category = "Projectile")
	TObjectPtr<class UNiagaraComponent> NiagaraComponent;
};
