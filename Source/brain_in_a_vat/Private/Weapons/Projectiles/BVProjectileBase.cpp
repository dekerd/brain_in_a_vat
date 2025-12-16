// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapons/Projectiles/BVProjectileBase.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"

#include "Autobots/BVAutobotBase.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystemComponent.h"
#include "GAS/CombatAttributeSet.h"
#include "GAS/GAStags.h"
#include "Components/PrimitiveComponent.h"
#include "MainCharacter.h"


// Sets default values
ABVProjectileBase::ABVProjectileBase()
{
	PrimaryActorTick.bCanEverTick = false;

	// Collision
	CollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
	RootComponent = CollisionComponent;
	CollisionComponent->InitSphereRadius(10.f);
	CollisionComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CollisionComponent->SetCollisionObjectType(ECC_WorldDynamic);
	CollisionComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
	CollisionComponent->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap); // If Unit is a pawn
	CollisionComponent->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block); // Bloacked by scene components
	CollisionComponent->SetNotifyRigidBodyCollision(true); // For OnHit

	CollisionComponent->OnComponentBeginOverlap.AddDynamic(this, &ABVProjectileBase::OnCollisionBeginOverlap);

	// Niagara
	NiagaraComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("Niagara"));
	NiagaraComponent->SetupAttachment(RootComponent);
	NiagaraComponent->bAutoActivate = true;

	static ConstructorHelpers::FObjectFinder<UNiagaraSystem> NS_LaserRef(TEXT("/Script/Niagara.NiagaraSystem'/Game/VFX/magic/magic2.magic2'"));
	if (NS_LaserRef.Succeeded())
	{
		NiagaraComponent->SetAsset(NS_LaserRef.Object);
	}
	
	// Projectile Movement
	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->InitialSpeed = 10000.f;
	ProjectileMovement->MaxSpeed = 10000.f;
	ProjectileMovement->bRotationFollowsVelocity = true;
	ProjectileMovement->bShouldBounce = false;
	ProjectileMovement->ProjectileGravityScale = 0.f;

	InitialLifeSpan = 2.f;

	// GAS
	static ConstructorHelpers::FClassFinder<UGameplayEffect> DamageGEClass(TEXT("/Script/Engine.Blueprint'/Game/GAS/GE/GE_LaserDamage.GE_LaserDamage_C'"));
	if (DamageGEClass.Succeeded())
	{
		DamageEffect = DamageGEClass.Class;
	}
	
}

void ABVProjectileBase::InitBeamEnd(const FVector& StartLocation, const FVector& EndLocation)
{
	if (!NiagaraComponent) return;

	NiagaraComponent->SetNiagaraVariablePosition(TEXT("BeamStart"), StartLocation);
	NiagaraComponent->SetNiagaraVariablePosition(TEXT("BeamEnd"), EndLocation);
}

void ABVProjectileBase::InitVelocity(const FVector& FireDir)
{
	if (ProjectileMovement)
	{
		ProjectileMovement->Velocity = FireDir.GetSafeNormal() * ProjectileMovement->InitialSpeed;
	}
}

void ABVProjectileBase::OnCollisionBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{

	UE_LOG(LogTemp, Warning, TEXT("OnCollisionBeginOverlap"))
	if (!OtherActor || OtherActor == this || OtherActor == GetOwner()) return;

	ABVAutobotBase* HitUnit = Cast<ABVAutobotBase>(OtherActor);
	if (!HitUnit)
	{
		UE_LOG(LogTemp, Warning, TEXT("Laser : No hit unit"))
		return;
	}
	
	if (HitUnit->bIsDead) return;

	if (const AMainCharacter* MainPlayer = Cast<AMainCharacter>(GetOwner()))
	{
		if (HitUnit->GetTeamFlag() == MainPlayer->GetTeamFlag()) return;
	}

	UE_LOG(LogTemp, Warning, TEXT("Laser : Checkpoint 2"))

	if (!DamageEffect)
	{
		UE_LOG(LogTemp, Warning, TEXT("Projectile has no DamageEffect set"));
		return;
	}

	IAbilitySystemInterface* TargetASI = Cast<IAbilitySystemInterface>(HitUnit);
	if (!TargetASI)
	{
		UE_LOG(LogTemp, Warning, TEXT("Projectile hit actor has no AbilitySystemInterface"));
		return;
	}

	UAbilitySystemComponent* TargetASC = TargetASI->GetAbilitySystemComponent();
	if (!TargetASC)
	{
		UE_LOG(LogTemp, Warning, TEXT("Projectile hit actor has no ASC"));
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("Laser : GE is about to work"))

	FGameplayEffectContextHandle ContextHandle = TargetASC->MakeEffectContext();
	ContextHandle.AddSourceObject(this);

	FGameplayEffectSpecHandle SpecHandle = TargetASC->MakeOutgoingSpec(DamageEffect, 1.f, ContextHandle);
	if (!SpecHandle.IsValid()) return;

	SpecHandle.Data->SetSetByCallerMagnitude(TAG_Data_Damage, -DamageAmount);
	TargetASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());

}

