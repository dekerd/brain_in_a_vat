// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BVBuildingBase.generated.h"

UCLASS()
class BRAIN_IN_A_VAT_API ABVBuildingBase : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ABVBuildingBase();

	virtual void Tick(float DeltaTime) override;

protected:

	virtual void BeginPlay() override;
	
	// Building Components
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<class UCapsuleComponent> CapsuleComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh")
	TObjectPtr<class UStaticMeshComponent> StaticMeshComponent;

	// Unit Spawning
	UPROPERTY()
	TSubclassOf<class ABVAutobotBase> SpawnUnitClass;

	UPROPERTY(EditAnywhere, Category = "Spawn Unit")
	float RespawnInterval = 10.f;
	float ElapsedTime;
	
	// Respawn Cooltime Widget
	
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<class UWidgetComponent> RespawnWidgetComponent;

	UPROPERTY()
	TSubclassOf<UUserWidget> RespawnWidgetClass;

	UPROPERTY()
	TObjectPtr<class UBVSpawnCooltimeBar> RespawnWidget;

	FTimerHandle SpawnTimerHandle;

	UFUNCTION()
	void SpawnUnit();

	
};
