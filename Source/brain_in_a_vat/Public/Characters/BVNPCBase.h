// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BVCharacterBase.h"
#include "BVNPCBase.generated.h"

UCLASS()
class BRAIN_IN_A_VAT_API ABVNPCBase : public ABVCharacterBase
{
	GENERATED_BODY()

// Static mesh for non-moving character
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "mesh")
	TObjectPtr<class UStaticMeshComponent> StaticMeshComponent;

protected:

public:
	// Sets default values for this character's properties
	ABVNPCBase();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	
};
