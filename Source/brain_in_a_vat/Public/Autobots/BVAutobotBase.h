// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GenericTeamAgentInterface.h"
#include "GameFramework/Character.h"
#include "BVAutobotBase.generated.h"

UCLASS()
class BRAIN_IN_A_VAT_API ABVAutobotBase : public ACharacter, public IGenericTeamAgentInterface
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	ABVAutobotBase();
	
	UPROPERTY()
	uint32 TeamFlag;

	virtual FGenericTeamId GetGenericTeamId() const override;
	
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UPROPERTY()
	bool bHasTarget = false;
	
};
