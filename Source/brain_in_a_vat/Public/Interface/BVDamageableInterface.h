// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GenericTeamAgentInterface.h"
#include "UObject/Interface.h"
#include "BVDamageableInterface.generated.h"

// This class does not need to be modified.
UINTERFACE()
class UBVDamageableInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class BRAIN_IN_A_VAT_API IBVDamageableInterface
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Combat")
	FGenericTeamId GetTeamId() const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Combat")
	bool IsDestroyed() const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "UI")
	void SetHovered(bool bInHovered);
	
};
