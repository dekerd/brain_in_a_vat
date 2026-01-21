// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapons/Projectiles/BVProjectileBase.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Collision/BVCollision.h"
#include "Autobots/BVAutobotBase.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystemComponent.h"
#include "GAS/GAStags.h"
#include "Components/PrimitiveComponent.h"
#include "MainCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "Weapons/Projectiles/BVLaserBeamBase.h"
#include "Particles/ParticleSystem.h"


// Sets default values
ABVProjectileBase::ABVProjectileBase()
{
	PrimaryActorTick.bCanEverTick = false;

	// Collision
	CollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
	RootComponent = CollisionComponent;
	CollisionComponent->InitSphereRadius(10.f);
	CollisionComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CollisionComponent->SetCollisionObjectType(ECC_Projectile);
	CollisionComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
	CollisionComponent->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap); // If Unit is a pawn
	CollisionComponent->SetCollisionResponseToChannel(ECC_Building, ECR_Overlap);
	CollisionComponent->SetCollisionResponseToChannel(ECC_Player, ECR_Ignore); 
	CollisionComponent->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block); // Bloacked by scene components
	CollisionComponent->SetNotifyRigidBodyCollision(true); // For OnHit

	CollisionComponent->OnComponentBeginOverlap.AddDynamic(this, &ABVProjectileBase::OnCollisionBeginOverlap);

	// Mesh
	StaticMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMesh"));
	StaticMeshComponent->SetupAttachment(RootComponent);
	StaticMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// Projectile Movement
	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->bRotationFollowsVelocity = true;
	
	// GAS
	static ConstructorHelpers::FClassFinder<UGameplayEffect> DamageGEClass(TEXT("/Script/Engine.Blueprint'/Game/GAS/GE/GE_LaserDamage.GE_LaserDamage_C'"));
	if (DamageGEClass.Succeeded())
	{
		DamageEffect = DamageGEClass.Class;
	}
	
}

void ABVProjectileBase::BeginPlay()
{
	Super::BeginPlay();

	if (FireSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, FireSound, GetActorLocation(), FireSoundVolume);
	}
}

void ABVProjectileBase::InitVelocity(const FVector& FireDir)
{
	if (ProjectileMovement)
	{
		ProjectileMovement->Velocity = FireDir.GetSafeNormal() * ProjectileMovement->InitialSpeed;
	}
}

void ABVProjectileBase::SetLaunchVelocity(const FVector& LaunchVelocity)
{
	if (ProjectileMovement)
	{
		ProjectileMovement->Velocity = LaunchVelocity;
	}
}

void ABVProjectileBase::OnCollisionBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
                                                UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	
	if (!OtherActor || OtherActor == this || OtherActor == GetOwner()) return;
	if (!OtherActor->Implements<UBVDamageableInterface>()) return;
	
	if (IBVDamageableInterface::Execute_IsDestroyed(OtherActor)) return;

	if (const AMainCharacter* MainPlayer = Cast<AMainCharacter>(GetOwner()))
	{
		if (IBVDamageableInterface::Execute_GetTeamId(OtherActor) == MainPlayer->GetTeamFlag())
		{
			return;
		}
	}

	if (!DamageEffect) return;

	IAbilitySystemInterface* TargetASI = Cast<IAbilitySystemInterface>(OtherActor);
	if (!TargetASI)
	{
		return;
	}

	UAbilitySystemComponent* TargetASC = TargetASI->GetAbilitySystemComponent();
	if (!TargetASC)
	{
		return;
	}
	

	FGameplayEffectContextHandle ContextHandle = TargetASC->MakeEffectContext();
	ContextHandle.AddSourceObject(this);

	FGameplayEffectSpecHandle SpecHandle = TargetASC->MakeOutgoingSpec(DamageEffect, 1.f, ContextHandle);
	if (!SpecHandle.IsValid()) return;

	SpecHandle.Data->SetSetByCallerMagnitude(TAG_Data_Damage, -DamageAmount);
	TargetASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());

	if (ABVLaserBeamBase* LaserBeam = Cast<ABVLaserBeamBase>(this))
	{
		return;
	}

	if (HitSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, HitSound, GetActorLocation(), HitSoundVolume);
	}

	if (HitEffect)
	{
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), HitEffect, GetActorLocation(), GetActorRotation());
	}
	
	Destroy();

}

