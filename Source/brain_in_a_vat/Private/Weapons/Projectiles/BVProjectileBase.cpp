// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapons/Projectiles/BVProjectileBase.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Collision/BVCollision.h"
#include "Buildings/BVBuildingBase.h"
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

	// Collision — Projectile 프리셋 (Pawn/Building/Player Overlap, WorldStatic Block)
	CollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
	RootComponent = CollisionComponent;
	CollisionComponent->InitSphereRadius(10.f);
	CollisionComponent->SetCollisionProfileName(TEXT("Projectile"));
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

void ABVProjectileBase::SpawnHitVFX(AActor* HitActor, const FHitResult* SurfaceHit)
{
	FVector SpawnLoc = GetActorLocation();

	// 1순위: Overlap/Sweep에서 직접 받은 ImpactPoint. 표면 정확.
	if (SurfaceHit && SurfaceHit->bBlockingHit && !SurfaceHit->ImpactPoint.IsNearlyZero())
	{
		SpawnLoc = SurfaceHit->ImpactPoint;
	}
	else if (HitActor)
	{
		// 2순위: 타겟의 콜리전 프리미티브에 대해 "가장 가까운 표면 점"을 찾음.
		// 큰 타겟(거점 등)이라서 투사체가 내부로 파고든 상황에서도 표면에 VFX가 뜸.
		FVector BestPoint = SpawnLoc;
		float BestDistSq = TNumericLimits<float>::Max();
		bool bFoundSurface = false;

		TArray<UPrimitiveComponent*> Prims;
		HitActor->GetComponents<UPrimitiveComponent>(Prims);
		for (UPrimitiveComponent* Prim : Prims)
		{
			if (!Prim) continue;
			if (Prim->GetCollisionEnabled() == ECollisionEnabled::NoCollision) continue;

			FVector Point;
			const float Dist = Prim->GetClosestPointOnCollision(GetActorLocation(), Point);
			if (Dist < 0.f) continue; // 이 프리미티브는 질의 불가

			// Dist == 0 은 투사체가 내부에 있음 → 같은 점 반환이라 사용 불가. 다른 프리미티브 시도.
			if (Dist <= KINDA_SMALL_NUMBER) continue;

			const float DistSq = FVector::DistSquared(GetActorLocation(), Point);
			if (DistSq < BestDistSq)
			{
				BestDistSq = DistSq;
				BestPoint = Point;
				bFoundSurface = true;
			}
		}

		if (bFoundSurface)
		{
			SpawnLoc = BestPoint;
		}
		else
		{
			// 3순위 (폴백): 투사체 속도 반대방향으로 line trace해서 표면을 찾음.
			// 내부에 완전히 들어간 케이스 대응.
			FVector TraceDir = -GetActorForwardVector();
			if (UProjectileMovementComponent* Move = FindComponentByClass<UProjectileMovementComponent>())
			{
				if (!Move->Velocity.IsNearlyZero())
				{
					TraceDir = -Move->Velocity.GetSafeNormal();
				}
			}

			const FVector TraceStart = GetActorLocation() + TraceDir * 1000.f;
			const FVector TraceEnd   = GetActorLocation();
			FHitResult BackHit;
			FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(ProjectileBackTrace), false, this);
			QueryParams.AddIgnoredActor(this);
			if (GetWorld()->LineTraceSingleByChannel(BackHit, TraceStart, TraceEnd, ECC_Visibility, QueryParams)
				&& BackHit.GetActor() == HitActor)
			{
				SpawnLoc = BackHit.ImpactPoint;
			}
			// 그래도 못 찾으면 현재 위치 유지 (기존 동작). 내부 폭발보다 덜 나쁜 선택지 없음.
		}
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
		return;
	}
	bDataAssetApplied = true;

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

	// 빌딩이면 데미지 훅 호출(가해자 팀 기록 + 거점 점령 진행도 반영 등).
	if (ABVBuildingBase* TargetBuilding = Cast<ABVBuildingBase>(OtherActor))
	{
		TargetBuilding->HandleDamageReceived(GetInstigator(), DamageAmount);
	}

	TargetASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());

	if (ABVLaserBeamBase* LaserBeam = Cast<ABVLaserBeamBase>(this))
	{
		return;
	}

	if (HitSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, HitSound, GetActorLocation(), HitSoundVolume);
	}

	SpawnHitVFX(OtherActor, bFromSweep ? &SweepResult : nullptr);

	Destroy();

}

void ABVProjectileBase::OnCollisionHit(UPrimitiveComponent* HitComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	SpawnHitVFX(OtherActor, &Hit);

	if (HitSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, HitSound, GetActorLocation(), HitSoundVolume);
	}

	Destroy();
}

