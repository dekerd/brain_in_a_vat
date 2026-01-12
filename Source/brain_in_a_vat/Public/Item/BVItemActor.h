// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interface/BVDamageableInterface.h"
#include "BVItemActor.generated.h"

UCLASS()
class BRAIN_IN_A_VAT_API ABVItemActor : public AActor, public IBVDamageableInterface
{
	GENERATED_BODY()

// Constructor and Functions
public:
	ABVItemActor();

protected:
	virtual void BeginPlay() override;
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void Tick(float DeltaTime) override;

// Components
public:
	
protected:

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<class USphereComponent> SphereComponent;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UStaticMeshComponent> MeshComponent;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<class URotatingMovementComponent> RotatingComponent;

// Item Data
public:
	
protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	TObjectPtr<class UBVItemData> ItemData;


// Item Mesh Property and Interaction
public:

protected:
	UPROPERTY(EditAnywhere, Category = "Movement")
	float BobbingAmplitude = 10.f;

	UPROPERTY(EditAnywhere, Category = "Movement")
	float BobbingFrequency = 2.0f;

	UPROPERTY(EditAnywhere, Category = "Movement")
	float RotationRate = 45.f;

	float RunningTime = 0.0f;
	FVector InitialRelativeLocation;

	virtual void SetHovered_Implementation(bool bInHovered) override;
	
	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComponent,
						AActor* OtherActor,
						UPrimitiveComponent* OtherComp,
						int32 OtherBodyIndex,
						bool bFromSweep,
						const FHitResult& SweepResult);


};
