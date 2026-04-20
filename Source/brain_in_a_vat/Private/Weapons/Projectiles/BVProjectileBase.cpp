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
	CollisionComponent->SetCollisionResponseToChannel(ECC_Player, ECR_Overlap);
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
			StaticMeshComponent->SetRelativeRotation(ProjectileData->MeshRotation);
			StaticMeshComponent->SetRelativeLocation(ProjectileData->MeshLocationOffset);
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

void ABVProjectileBase::SpawnHitVFX(AActor* HitActor)
{
	// 피격 대상이 있으면, 충돌 지점(투사체 위치)과 대상 중심 사이를 70% 보간.
	// "너무 바깥도, 너무 정중앙도 아닌" 자연스러운 위치.
	FVector SpawnLoc = GetActorLocation();
	if (HitActor)
	{
		SpawnLoc = FMath::Lerp(GetActorLocation(), HitActor->GetActorLocation(), 0.7f);
	}
	const FRotator SpawnRot = GetActorRotation();

	if (HitNiagaraEffect)
	{
		UNiagaraComponent* NC = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			GetWorld(), HitNiagaraEffect, SpawnLoc, SpawnRot);

		// DA의 재생속도 배수 적용 (CustomTimeDilation으로 시뮬레이션 속도 스케일)
		if (NC && ProjectileData && ProjectileData->HitNiagaraPlayRate > 0.f
			&& !FMath::IsNearlyEqual(ProjectileData->HitNiagaraPlayRate, 1.f))
		{
			NC->SetCustomTimeDilation(ProjectileData->HitNiagaraPlayRate);
		}
	}
	else if (HitEffect)
	{
		UGameplayStatics::SpawnEmitterAtLocation(
			GetWorld(), HitEffect, SpawnLoc, SpawnRot);
	}
}

void ABVProjectileBase::Explode()
{
	SpawnHitVFX();
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

	// 중복 호출 가드 — 혹시라도 두 번 호출되면 Trail 이 두 번 스폰된다.
	if (bDataAssetApplied)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[Projectile] ApplyDataAsset called twice on %s — skipping second call"),
			*GetName());
		return;
	}
	bDataAssetApplied = true;

	UE_LOG(LogTemp, Warning,
		TEXT("[Projectile] ApplyDataAsset on %s: Trail=%s"),
		*GetName(),
		PData->TrailEffect ? *PData->TrailEffect->GetName() : TEXT("NULL"));

	// --- Visual: Mesh ---
	if (StaticMeshComponent && PData->ProjectileMesh)
	{
		StaticMeshComponent->SetStaticMesh(PData->ProjectileMesh);
		StaticMeshComponent->SetRelativeScale3D(FVector(PData->MeshScale));
		StaticMeshComponent->SetRelativeRotation(PData->MeshRotation);
		StaticMeshComponent->SetRelativeLocation(PData->MeshLocationOffset);
	}

	// --- Trail VFX (Niagara) ---
	// 트레일은 투사체 Root(= 발사 방향) 에 직접 붙인다.
	// StaticMesh 에 붙이면 MeshRotation 과 TrailRotation 이 이중으로 적용되어
	// 엉뚱한 방향으로 트레일이 뻗을 수 있음.
	//
	// [중복 방지] BP 에 NiagaraComponent 가 자식으로 붙어 있으면 DA가 스폰하는
	// Trail 과 이중으로 재생되어 "trail 2개 (하나는 하늘로)" 버그가 발생한다.
	// DA가 Trail 을 관리하므로, 런타임에 기존 NiagaraComponent 들은 전부 비활성/파괴.
	{
		TArray<UNiagaraComponent*> ExistingNCs;
		GetComponents<UNiagaraComponent>(ExistingNCs);
		for (UNiagaraComponent* NC : ExistingNCs)
		{
			if (!NC) continue;
			UE_LOG(LogTemp, Warning,
				TEXT("[Projectile] Stripping pre-existing NiagaraComponent '%s' on %s (DA manages trail)"),
				*NC->GetName(), *GetName());
			NC->Deactivate();
			NC->DestroyComponent();
		}
	}

	if (PData->TrailEffect)
	{
		USceneComponent* AttachParent = GetRootComponent();
		if (AttachParent)
		{
			UNiagaraFunctionLibrary::SpawnSystemAttached(
				PData->TrailEffect,
				AttachParent,
				NAME_None,
				PData->TrailLocationOffset,
				PData->TrailRotation,
				EAttachLocation::KeepRelativeOffset,
				true  /*bAutoActivate*/,
				true  /*bAutoDestroy*/);
		}
	}

	// --- Audio (DA 값으로 덮어쓰기) ---
	if (PData->FireSound)  { FireSound = PData->FireSound; }
	FireSoundVolume = PData->FireSoundVolume;
	if (PData->HitSound)   { HitSound = PData->HitSound; }
	HitSoundVolume = PData->HitSoundVolume;

	// --- Hit VFX (Niagara 우선, Cascade 폴백) ---
	if (PData->HitNiagaraEffect) { HitNiagaraEffect = PData->HitNiagaraEffect; }
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

	SpawnHitVFX(OtherActor);

	Destroy();

}

void ABVProjectileBase::OnCollisionHit(UPrimitiveComponent* HitComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	SpawnHitVFX(OtherActor);

	if (HitSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, HitSound, GetActorLocation(), HitSoundVolume);
	}

	Destroy();
}

