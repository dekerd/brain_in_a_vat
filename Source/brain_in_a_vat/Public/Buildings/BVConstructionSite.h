// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GenericTeamAgentInterface.h"
#include "GameFramework/Actor.h"
#include "BVConstructionSite.generated.h"

class UBoxComponent;
class ABVBuildingBase;

UCLASS()
class BRAIN_IN_A_VAT_API ABVConstructionSite : public AActor, public IGenericTeamAgentInterface
{
	GENERATED_BODY()

// Initialization
public:
	// Sets default values for this actor's properties
	ABVConstructionSite();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

// Components
public:

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> SceneRootComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UBoxComponent> BoxComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* MeshComponent;


	TSubclassOf<ABVBuildingBase> TargetBuildingClass;

	float ConstructionTime = 10.f;

	FGenericTeamId TeamId;

	void InitConstruction(TSubclassOf<ABVBuildingBase> InBuildingClass, FGenericTeamId InTeamId);
	
	virtual void SetGenericTeamId(const FGenericTeamId& NewTeamId) override { TeamId = NewTeamId; }
	virtual FGenericTeamId GetGenericTeamId() const override { return TeamId; }

private:
	float CurrentProgress = 0.0f;
	bool bIsBuilding = false;

	// 진짜 건물 소환 함수
	void SpawnRealBuilding();
};
