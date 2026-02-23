// Fill out your copyright notice in the Description page of Project Settings.


#include "Buildings/BVDefenseTower.h"

#include "Components/SphereComponent.h"
#include "GAS/GAStags.h"
#include "Weapons/Projectiles/BVLaserBeamBase.h"


// Sets default values
ABVDefenseTower::ABVDefenseTower()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// [GAS] Damage GE
	if (!DamageEffect)
	{
		static ConstructorHelpers::FClassFinder<UGameplayEffect> DamageGEClass(TEXT("/Script/Engine.Blueprint'/Game/GAS/GE/GE_LaserDamage.GE_LaserDamage_C'"));
		if (DamageGEClass.Succeeded())
		{
			DamageEffect = DamageGEClass.Class;
		}
	}

	// Attack Range Component
	AttackRangeSphere = CreateDefaultSubobject<USphereComponent>(TEXT("AttackRangeSphere"));
	AttackRangeSphere->SetupAttachment(RootComponent);
	AttackRangeSphere->InitSphereRadius(AttackRange);
	AttackRangeSphere->SetCollisionProfileName(TEXT("Trigger"));
}

// Called when the game starts or when spawned
void ABVDefenseTower::BeginPlay()
{
	Super::BeginPlay();

	// Attack Range Setting (for towers)

	if (bCanAttack && AttackRangeSphere)
	{
		AttackRangeSphere->SetSphereRadius(AttackRange);
		AttackRangeSphere->OnComponentBeginOverlap.AddDynamic(this, &ABVDefenseTower::OnAttackRangeBeginOverlap);
		AttackRangeSphere->OnComponentEndOverlap.AddDynamic(this, &ABVDefenseTower::OnAttackRangeEndOverlap);

		TArray<AActor*> OverlappingActors;
		AttackRangeSphere->GetOverlappingActors(OverlappingActors);

		for (AActor* Actor : OverlappingActors)
		{
			if (!Actor || Actor == this) continue;

			const IGenericTeamAgentInterface* TargetTeamAgent = Cast<IGenericTeamAgentInterface>(Actor);
			if (!TargetTeamAgent) continue;

			FGenericTeamId OtherTeamId = TargetTeamAgent->GetGenericTeamId();
			FGenericTeamId MyTeamId = GetGenericTeamId();

			if (OtherTeamId != FGenericTeamId::NoTeam && OtherTeamId != MyTeamId)
			{
				EnemiesInRange.AddUnique(Actor);
			}
		}
	}
	else
	{
		AttackRangeSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		
	}
	
}

void ABVDefenseTower::OnAttackRangeBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	
	if (!OtherActor || OtherActor == this) return;
	
	const IGenericTeamAgentInterface* TargetTeamAgent = Cast<IGenericTeamAgentInterface>(OtherActor);
	if (!TargetTeamAgent) return;

	FGenericTeamId OtherTeamId = TargetTeamAgent->GetGenericTeamId();
	FGenericTeamId MyTeamId = GetGenericTeamId();

	if (OtherTeamId != FGenericTeamId::NoTeam && OtherTeamId != MyTeamId)
	{
		EnemiesInRange.AddUnique(OtherActor);
	}
}

void ABVDefenseTower::OnAttackRangeEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (OtherActor)
	{
		EnemiesInRange.Remove(OtherActor);
	}
}

void ABVDefenseTower::FireLaser(float DeltaTime)
{
	TimeSinceLastShot += DeltaTime;
	if (TimeSinceLastShot < FireInterval) return;

	AActor* Target = FindNearestEnemy();
	if (!Target) return;

	// Fire!
	TimeSinceLastShot = 0;

	if (UWorld* World = GetWorld())
	{

		// Set Fire Location
		FVector FireLocation;
		if (StaticMeshComponent && StaticMeshComponent->DoesSocketExist(FireSocketName))
		{
			FireLocation = StaticMeshComponent->GetSocketLocation(FireSocketName);
		}
		else
		{
			FireLocation = GetActorLocation() + FVector(0, 0, 150.f);
		}
		
		FVector TargetLocation = Target->GetActorLocation();

		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = this;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		ABVLaserBeamBase* Laser = World->SpawnActor<ABVLaserBeamBase>(ABVLaserBeamBase::StaticClass(),
																      FireLocation,
																      FRotator::ZeroRotator,
																      SpawnParams);
		if (Laser)
		{
			Laser->InitBeamEnd(FireLocation, TargetLocation);
		}
	}

	ApplyDamage(Target);
}

AActor* ABVDefenseTower::FindNearestEnemy()
{
	AActor* NearestTarget = nullptr;
	float BestDistSq = FLT_MAX;
	FVector MyLoc = GetActorLocation();

	for (int32 i = EnemiesInRange.Num() - 1; i >= 0; --i)
	{
		AActor* Target = EnemiesInRange[i].Get();

		// If the target is invalid or destroyed, then remove from the list
		if (!IsValid(Target) || (Target->Implements<UBVDamageableInterface>() && IBVDamageableInterface::Execute_IsDestroyed(Target)))
		{
			EnemiesInRange.RemoveAt(i);
			continue;
		}

		float DistSq = FVector::DistSquared(MyLoc, Target->GetActorLocation());
		if (DistSq < BestDistSq)
		{
			BestDistSq = DistSq;
			NearestTarget = Target;
		}
	}

	return NearestTarget;
}

void ABVDefenseTower::ApplyDamage(AActor* Target)
{
	if (!ASC || !DamageEffect) return;

	IAbilitySystemInterface* TargetASI = Cast<IAbilitySystemInterface>(Target);
	if (!TargetASI) return;

	UAbilitySystemComponent* TargetASC = TargetASI->GetAbilitySystemComponent();
	if (!TargetASC) return;

	FGameplayEffectContextHandle ContextHandle = ASC->MakeEffectContext();
	ContextHandle.AddSourceObject(this);

	FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(DamageEffect, 1.f, ContextHandle);
	if (SpecHandle.IsValid())
	{
		SpecHandle.Data->SetSetByCallerMagnitude(TAG_Data_Damage, -AttackDamage);
		ASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
	}
}

// Called every frame
void ABVDefenseTower::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Building Defense System
	if (bCanAttack)
	{
		FireLaser(DeltaTime);
	}
}

