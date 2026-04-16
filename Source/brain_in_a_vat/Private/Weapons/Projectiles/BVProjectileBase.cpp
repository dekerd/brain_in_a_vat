// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapons/Projectiles/BVProjectileBase.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Collision/BVCollision.h"
#include "Characters/BVAutobotBase.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystemComponent.h"
#include "GAS/GAStags.h"
#include "Components/PrimitiveComponent.h"
#include "MainCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "Weapons/Projectiles/BVLaserBeamBase.h"
#include "Particles/ParticleSystem.h"
#include "Data/BVProjectileData.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"


// Sets default values
ABVProjectileBase::ABVProjectileBase()
{
	PrimaryActorTick.bCanEverTick = true;

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
	CollisionComponent->OnComponentHit.AddDynamic(this, &ABVProjectileBase::OnCollisionHit);
	InitialLifeSpan = 5.0f;

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

void ABVProjectileBase::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	// 에디터 뷰포트에서도 DA가 할당되면 즉시 메시/스케일/충돌반경 반영.
	// (사운드/Niagara/속도/궤적 등 런타임 전용 항목은 BeginPlay에서 처리)
	if (ProjectileData)
	{
		if (StaticMeshComponent && ProjectileData->ProjectileMesh)
		{
			StaticMeshComponent->SetStaticMesh(ProjectileData->ProjectileMesh);
			StaticMeshComponent->SetRelativeScale3D(FVector(ProjectileData->MeshScale));
		}
		if (CollisionComponent && ProjectileData->CollisionRadius > 0.f)
		{
			CollisionComponent->SetSphereRadius(ProjectileData->CollisionRadius);
		}
	}
}

void ABVProjectileBase::BeginPlay()
{
	Super::BeginPlay();

	SpawnOrigin = GetActorLocation();

	// DA가 BP 디폴트에 할당돼 있으면 자동으로 적용 (메시/VFX/사운드/속도 등)
	ApplyDataAsset();

	if (FireSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, FireSound, GetActorLocation(), FireSoundVolume);
	}
}

void ABVProjectileBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (MaxTravelDistance > 0.f)
	{
		const float DistTraveled = FVector::Dist(SpawnOrigin, GetActorLocation());
		if (DistTraveled >= MaxTravelDistance)
		{
			Explode();
		}
	}
}

void ABVProjectileBase::Explode()
{
	if (HitEffect)
	{
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), HitEffect, GetActorLocation(), GetActorRotation());
	}
	if (HitSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, HitSound, GetActorLocation(), HitSoundVolume);
	}
	Destroy();
}

void ABVProjectileBase::ApplyDataAsset()
{
	if (!ProjectileData) return;
	const UBVProjectileData* PData = ProjectileData;

	// --- Visual: Mesh ---
	if (StaticMeshComponent && PData->ProjectileMesh)
	{
		StaticMeshComponent->SetStaticMesh(PData->ProjectileMesh);
		StaticMeshComponent->SetRelativeScale3D(FVector(PData->MeshScale));
	}

	// --- Trail VFX (Niagara) ---
	if (PData->TrailEffect)
	{
		USceneComponent* AttachParent = StaticMeshComponent
			? static_cast<USceneComponent*>(StaticMeshComponent.Get())
			: GetRootComponent();

		UNiagaraFunctionLibrary::SpawnSystemAttached(
			PData->TrailEffect,
			AttachParent,
			NAME_None,
			FVector::ZeroVector,
			FRotator::ZeroRotator,
			EAttachLocation::KeepRelativeOffset,
			true  /*bAutoActivate*/,
			true  /*bAutoDestroy*/);
	}

	// --- Audio (DA 값으로 덮어쓰기) ---
	if (PData->FireSound)  { FireSound = PData->FireSound; }
	FireSoundVolume = PData->FireSoundVolume;
	if (PData->HitSound)   { HitSound = PData->HitSound; }
	HitSoundVolume = PData->HitSoundVolume;

	// --- Hit VFX ---
	if (PData->HitEffect) { HitEffect = PData->HitEffect; }

	// --- Gameplay ---
	if (PData->DamageOverride > 0.f)
	{
		DamageAmount = PData->DamageOverride;
	}
	if (PData->Lifespan > 0.f)
	{
		SetLifeSpan(PData->Lifespan);
	}
	if (CollisionComponent && PData->CollisionRadius > 0.f)
	{
		CollisionComponent->SetSphereRadius(PData->CollisionRadius);
	}

	// --- Projectile Range (최대 비행 거리) ---
	if (PData->ProjectileRange > 0.f)
	{
		MaxTravelDistance = PData->ProjectileRange;
	}

	// --- Trajectory ---
	if (ProjectileMovement && PData->ProjectileSpeed > 0.f)
	{
		ProjectileMovement->InitialSpeed = PData->ProjectileSpeed;
		ProjectileMovement->MaxSpeed = PData->ProjectileSpeed;

		ProjectileMovement->ProjectileGravityScale =
			(PData->TrajectoryType == EBVProjectileTrajectory::Straight) ? 0.f : 1.f;
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

	FGenericTeamId MyTeamId = FGenericTeamId::NoTeam;
	if (IGenericTeamAgentInterface* OwnerAgent = Cast<IGenericTeamAgentInterface>(GetInstigator()))
	{
		MyTeamId = OwnerAgent->GetGenericTeamId();
	}

	const IGenericTeamAgentInterface* TargetAgent = Cast<IGenericTeamAgentInterface>(OtherActor);
	if (!TargetAgent) return;

	FGenericTeamId TargetTeamId = TargetAgent->GetGenericTeamId();
	if (TargetTeamId == MyTeamId) return;
	
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

void ABVProjectileBase::OnCollisionHit(UPrimitiveComponent* HitComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (HitEffect)
	{
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), HitEffect, GetActorLocation(), GetActorRotation());
	}

	if (HitSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, HitSound, GetActorLocation(), HitSoundVolume);
	}
    
	Destroy();
}

