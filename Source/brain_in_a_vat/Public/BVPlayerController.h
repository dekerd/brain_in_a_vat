// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GenericTeamAgentInterface.h"
#include "InputActionValue.h"
#include "GameFramework/PlayerController.h"
#include "BVPlayerController.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSelectionChanged, AActor*, NewSelectedActor);

/**
 * 
 */
UCLASS()
class BRAIN_IN_A_VAT_API ABVPlayerController : public APlayerController, public IGenericTeamAgentInterface
{
	GENERATED_BODY()

public:
	virtual FGenericTeamId GetGenericTeamId() const override { return TeamID; }
	virtual void SetGenericTeamId(const FGenericTeamId& InTeamID) override { TeamID = InTeamID; }

	virtual ETeamAttitude::Type GetTeamAttitudeTowards(const AActor& Other) const override;

	ABVPlayerController();
	
protected:
	virtual void BeginPlay() override;

	virtual void PlayerTick(float DeltaTime) override;

// Input Setting
protected:

	virtual void SetupInputComponent() override;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	TObjectPtr<class UInputMappingContext> InputMappingContext; 

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, Meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class UInputAction> MoveAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, Meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class UInputAction> SelectAction;
	
	UPROPERTY(BlueprintReadOnly, Category = "Selection")
	TObjectPtr<AActor> SelectedActor;

	UPROPERTY(BlueprintAssignable, Category = "Selection")
	FOnSelectionChanged OnSelectionChanged;

	void MoveToLocation(const FInputActionValue& Value);
	void SelectObject();

	// Player Team setting
private:
	FGenericTeamId TeamID = FGenericTeamId(1);


// Construction Mode
public:
	UFUNCTION(BlueprintCallable, Category = "Build")
	void EnterConstructionMode(TSubclassOf<ABVBuildingBase> InBuildingClass);

protected:
	UPROPERTY()
	TSubclassOf<ABVBuildingBase> CurrentBuildingClass;
	
// Mouse Hovering
protected:
	UPROPERTY()
	TObjectPtr<class AActor> HoveredObject;

	UPROPERTY(EditAnywhere, Category = "Audio")
	TObjectPtr<class USoundBase> HoverSound;

	UPROPERTY(EditAnywhere, Category = "Audio")
	float HoverSoundVolume = 0.5f;

// <--------------- Widgets --------------->
// Main HUD Widget
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI")
	TSubclassOf<class UUserWidget> MainHUDWidgetClass;
	
	UPROPERTY()
	TObjectPtr<class UUserWidget> MainHUDWidget;
	
// Gold Popup Widget
public:
	void ShowGoldReward(int32 Amount, FVector WorldLocation);

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI")
	TSubclassOf<class UUserWidget> GoldPopupWidgetClass;

	
// Background Music
public:

protected:
	UPROPERTY(EditAnywhere, Category = "Audio")
	TArray<TObjectPtr<class USoundBase>> BGMPlaylist;

	UPROPERTY(EditAnywhere, Category = "Audio")
	float Volume = 1.0f;
	
	UPROPERTY()
	TObjectPtr<class UAudioComponent> BGMComponent;

// Sound Effects
public:

protected:

	UPROPERTY(EditAnywhere, Category = "Audio")
	TObjectPtr<class USoundBase> GoldPickupSound;

	UPROPERTY(EditAnywhere, Category = "Audio")
	float GoldPickupSoundVolume = 1.0f;
	
};
