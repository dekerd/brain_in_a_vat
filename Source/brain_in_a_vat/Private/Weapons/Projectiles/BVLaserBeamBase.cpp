// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapons/Projectiles/BVLaserBeamBase.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "GAS/CombatAttributeSet.h"


// Sets default values
ABVLaserBeamBase::ABVLaserBeamBase()
{

	// Niagara
	NiagaraComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("Niagara"));
	NiagaraComponent->SetupAttachment(RootComponent);
	NiagaraComponent->bAutoActivate = true;

	static ConstructorHelpers::FObjectFinder<UNiagaraSystem> NS_LaserRef(TEXT("/Script/Niagara.NiagaraSystem'/Game/VFX/magic/magic2.magic2'"));
	if (NS_LaserRef.Succeeded())
	{
		NiagaraComponent->SetAsset(NS_LaserRef.Object);
	}

	// Sound

	static ConstructorHelpers::FObjectFinder<USoundBase> FireSoundRef(TEXT("/Script/Engine.SoundWave'/Game/SFX/Weapon/LaserBeam/LaserBeam.LaserBeam'"));
	if (FireSoundRef.Succeeded())
	{
		FireSound = FireSoundRef.Object;
	}
	
	// Projectile Movement
	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->InitialSpeed = 10000.f;
	ProjectileMovement->MaxSpeed = 10000.f;
	ProjectileMovement->bRotationFollowsVelocity = true;
	ProjectileMovement->bShouldBounce = false;
	ProjectileMovement->ProjectileGravityScale = 0.f;

	InitialLifeSpan = 2.f;
}

void ABVLaserBeamBase::InitBeamEnd(const FVector& StartLocation, const FVector& EndLocation)
{
	if (!NiagaraComponent) return;

	NiagaraComponent->SetNiagaraVariablePosition(TEXT("BeamStart"), StartLocation);
	NiagaraComponent->SetNiagaraVariablePosition(TEXT("BeamEnd"), EndLocation);
}
