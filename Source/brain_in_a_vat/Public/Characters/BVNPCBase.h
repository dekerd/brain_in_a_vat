// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BVCharacterBase.h"
#include "BVNPCBase.generated.h"

UCLASS()
class BRAIN_IN_A_VAT_API ABVNPCBase : public ABVCharacterBase
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	ABVNPCBase();

	UFUNCTION(BlueprintCallable, Category = "Interaction")
	virtual void Interact(class AMainCharacter* Interactor);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction")
	TObjectPtr<class UTexture2D> NPCPortrait;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	
};
