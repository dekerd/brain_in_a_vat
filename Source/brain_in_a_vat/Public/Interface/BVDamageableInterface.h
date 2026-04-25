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

	// 선택 상태(발밑 원 링). 실제 선택에 의해서만 토글.
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "UI")
	void SetSelected(bool bInSelected);

	// 호버 상태(외곽선 아웃라인). 마우스 커서가 올라가있거나 드래그 박스 안에 들어있을 때 ON.
	// 선택과 독립적으로 동시 활성 가능.
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "UI")
	void SetHovered(bool bInHovered);
	
};
