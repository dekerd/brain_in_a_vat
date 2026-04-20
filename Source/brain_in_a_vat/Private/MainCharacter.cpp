// Fill out your copyright notice in the Description page of Project Settings.


#include "MainCharacter.h"
#include "Components/PrimitiveComponent.h"

#include <ThirdParty/ShaderConductor/ShaderConductor/External/DirectXShaderCompiler/include/dxc/DXIL/DxilConstants.h>

#include "AIController.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "DrawDebugHelpers.h"
#include "Buildings/BVBuildingBase.h"
#include "Buildings/BVConstructionSite.h"
#include "Components/SphereComponent.h"
#include "Characters/BVAutobotBase.h"
#include "Weapons/Projectiles/BVLaserBeamBase.h"
#include "Collision/BVCollision.h"
#include "Item/BVItemData.h"
#include "Kismet/GameplayStatics.h"
#include "Data/BVProjectileData.h"
#include "Weapons/Projectiles/BVProjectileBase.h"
#include "GameFramework/ProjectileMovementComponent.h"

// Sets default values
AMainCharacter::AMainCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// Capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.f);
	GetCapsuleComponent()->SetCollisionProfileName(TEXT("Pawn"));
	GetCapsuleComponent()->SetCollisionObjectType(ECC_Player);
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap); // If Unit is a pawn
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Building, ECR_Overlap);
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Projectile, ECR_Ignore);
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block); // Bloacked by scene components
	GetCapsuleComponent()->SetNotifyRigidBodyCollision(true); // For OnHit

	// Movement

	//// Make the character aligned to the input direction
	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f);
	
	GetCharacterMovement()->JumpZVelocity = 400.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = 400.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;

	// Mesh
	GetMesh()->SetRelativeLocationAndRotation(FVector(0.f, 0.f, -96.f), FRotator(0.f, -90.f, 0.f));
	GetMesh()->SetAnimationMode(EAnimationMode::AnimationBlueprint);
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	static ConstructorHelpers::FObjectFinder<USkeletalMesh> CharacterMeshRef(TEXT("/Script/Engine.SkeletalMesh'/Game/SkeletalMesh/giant_battle_robot_accurig/giant_battle_robot_accurig.giant_battle_robot_accurig'"));
	if (CharacterMeshRef.Object)
	{
		GetMesh()->SetSkeletalMesh(CharacterMeshRef.Object);
	}

	static ConstructorHelpers::FClassFinder<UAnimInstance> AnimInstanceClassRef(TEXT("/Script/Engine.AnimBlueprint'/Game/AnimBP/ABP_GiantRobot.ABP_GiantRobot_C'"));
	if (AnimInstanceClassRef.Class)
	{
		GetMesh()->SetAnimInstanceClass(AnimInstanceClassRef.Class);
	}

	// AI Controller
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
	AIControllerClass = AAIController::StaticClass();

	/*
	// Camera
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);

	CameraBoom->TargetArmLength = 1300.0f;
	CameraBoom->SetRelativeRotation(FRotator(-65.f, 0.f, 0.f));
	CameraBoom->bUsePawnControlRotation = false;
	CameraBoom->bDoCollisionTest = false;
	
	// --> For Quarter View
	CameraBoom->bInheritPitch = false;
	CameraBoom->bInheritYaw   = false;
	CameraBoom->bInheritRoll  = false;

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	Camera->bUsePawnControlRotation = false;
	// Camera->ProjectionMode = ECameraProjectionMode::Orthographic;
	*/

	// Attack Sphere
	VisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("VisionRadius"));
	VisionSphere->SetupAttachment(RootComponent);
	VisionSphere->InitSphereRadius(VisionRadius);
	
	VisionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	VisionSphere->SetCollisionObjectType(ECC_WorldDynamic);
	VisionSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	VisionSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	VisionSphere->SetCollisionResponseToChannel(ECC_Building, ECR_Overlap);
	VisionSphere->SetGenerateOverlapEvents(true);

	// Weapon Cooltime
	// WeaponCoolTime.Init(0.0f, 5);
	
}

// Called when the game starts or when spawned
void AMainCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (VisionSphere)
	{
		VisionSphere->SetSphereRadius(VisionRadius);
		VisionSphere->OnComponentBeginOverlap.AddDynamic(this, &AMainCharacter::OnVisionRadiusBeginOverlap);
		VisionSphere->OnComponentEndOverlap.AddDynamic(this, &AMainCharacter::OnVisionRadiusEndOverlap);
	}
	
}

// Called every frame
void AMainCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	AutoFire(DeltaTime);

}

bool AMainCharacter::EquipItem(class UBVItemData* ItemToEquip)
{
	if (!ItemToEquip || EquippedWeapons.Num() >= MAX_EQUIP_ITEM) return false;
	
	if (InventoryItems.Contains(ItemToEquip))
	{
		InventoryItems.RemoveSingle(ItemToEquip);
		EquippedWeapons.Add(ItemToEquip);
		WeaponCoolTime.Add(0.0f);
		
		OnInventoryUpdated.Broadcast();
		return true;
	}
	return false;
}

bool AMainCharacter::UnequipItem(class UBVItemData* ItemToUnequip)
{
	if (!ItemToUnequip) return false;

	int32 Index = EquippedWeapons.Find(ItemToUnequip);
	if (Index != INDEX_NONE)
	{
		EquippedWeapons.RemoveAt(Index);
		WeaponCoolTime.RemoveAt(Index);
		InventoryItems.Add(ItemToUnequip);
		
		OnInventoryUpdated.Broadcast();
		return true;
	}
	return false;
}

bool AMainCharacter::AddItemToInventory(class UBVItemData* ItemData)
{

	if (!ItemData) return false;
	
	if (ItemData->ItemType == EItemType::Weapon && EquippedWeapons.Num() < MAX_EQUIP_ITEM)
	{
		EquippedWeapons.Add(ItemData);
		WeaponCoolTime.Add(0.0f);
	}
	else
	{
		InventoryItems.Add(ItemData);
	}

	PlayRandomPickupSound();
	OnInventoryUpdated.Broadcast();

	return true;
}

void AMainCharacter::OnVisionRadiusBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
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

void AMainCharacter::OnVisionRadiusEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (OtherActor)
	{
		EnemiesInRange.Remove(OtherActor);
	}
}

void AMainCharacter::AutoFire(float DeltaSecond)
{
	// This function will be called for every tick

	GlobalFireTimer += DeltaSecond;
	// FireDefaultLaserBeam(DeltaSecond);

	// Fire weapons equipped
	for (int32 i = 0; i < EquippedWeapons.Num(); ++i)
	{
		if (EquippedWeapons[i])
		{
			FireWeapons(DeltaSecond, i);
		}
	}
	
}

void AMainCharacter::FireWeapons(float DeltaSecond, int32 WeaponIndex)
{

	UBVItemData* Weapon = EquippedWeapons[WeaponIndex];
	if (!Weapon || !Weapon->WeaponData)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[FireWeapons] idx=%d: Weapon or WeaponData is NULL, skipping."),
			WeaponIndex);
		return;
	}

	const UBVProjectileData* PData = Weapon->WeaponData;

	WeaponCoolTime[WeaponIndex] += DeltaSecond;
	if (WeaponCoolTime[WeaponIndex] < PData->FireInterval) return;
	if (GlobalFireTimer < MinFireInterval) return;

	AActor* Target = FindNearestEnemyInRange();
	UE_LOG(LogTemp, Warning,
		TEXT("[FireWeapons] idx=%d Cool=%.2f/%.2f Global=%.2f/%.2f EnemiesInRange=%d Target=%s"),
		WeaponIndex, WeaponCoolTime[WeaponIndex], PData->FireInterval,
		GlobalFireTimer, MinFireInterval, EnemiesInRange.Num(),
		Target ? *Target->GetName() : TEXT("NULL"));

	if (Target)
	{
		// 사거리 게이트: WeaponData->ProjectileRange만 사용 (0 이하면 발사 금지).
		const float EffectiveRange = PData->ProjectileRange;
		if (EffectiveRange <= 0.f)
		{
			UE_LOG(LogTemp, Warning,
				TEXT("[FireWeapons] %s: ProjectileRange<=0 on %s"),
				*Weapon->GetName(), *PData->GetName());
			return;
		}

		const float Dist = FVector::Dist(GetActorLocation(), Target->GetActorLocation());
		if (Dist > EffectiveRange)
		{
			UE_LOG(LogTemp, Verbose,
				TEXT("[FireWeapons] OUT OF RANGE idx=%d Dist=%.1f > Eff=%.1f"),
				WeaponIndex, Dist, EffectiveRange);
			return;
		}

		FireDefaultMissile(Weapon, Target);
		WeaponCoolTime[WeaponIndex] = 0.0f;
		GlobalFireTimer = 0.0f;
	}

}

void AMainCharacter::FireDefaultMissile(UBVItemData* ItemData, AActor* Target)
{
	if (!ItemData || !Target || !ItemData->WeaponData) return;

	UWorld* World = GetWorld();
	if (!World) return;

	FVector SpawnLocation = GetActorLocation();
	SpawnLocation += FVector(0.0f, 0.0f, 30.0f);

	FVector TargetLocation = Target->GetActorLocation();
	if (const ACharacter* TargetChar = Cast<ACharacter>(Target))
	{
		TargetLocation.Z += TargetChar->GetDefaultHalfHeight() * 0.5f;
	}

	// --- DA에서 궤적/속도 읽기 ---
	const UBVProjectileData* PData = ItemData->WeaponData;
	EBVProjectileTrajectory Trajectory = PData->TrajectoryType;
	float ProjectileSpeed = PData->ProjectileSpeed > 0.f ? PData->ProjectileSpeed : 2500.f;
	float ArcValue = PData->ArcValue;

	// --- Launch 속도 계산 ---
	FVector LaunchVelocity = FVector::ZeroVector;
	if (Trajectory == EBVProjectileTrajectory::Arc)
	{
		const bool bArcOK = UGameplayStatics::SuggestProjectileVelocity_CustomArc(
			World,
			LaunchVelocity,
			SpawnLocation,
			TargetLocation,
			World->GetGravityZ(),
			ArcValue);

		if (!bArcOK || LaunchVelocity.IsNearlyZero())
		{
			// 포물선 계산 실패 시 직선 폴백
			const FVector Direction = (TargetLocation - SpawnLocation).GetSafeNormal();
			LaunchVelocity = Direction * ProjectileSpeed;
			Trajectory = EBVProjectileTrajectory::Straight;
		}
	}
	else
	{
		const FVector Direction = (TargetLocation - SpawnLocation).GetSafeNormal();
		LaunchVelocity = Direction * ProjectileSpeed;
	}

	// --- 스폰 (Deferred: DA 주입 후 FinishSpawning) ---
	const FTransform SpawnXform(LaunchVelocity.Rotation(), SpawnLocation);
	ABVProjectileBase* Projectile = World->SpawnActorDeferred<ABVProjectileBase>(
		ABVProjectileBase::StaticClass(),
		SpawnXform,
		this,
		this,
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn);

	if (!Projectile) return;

	Projectile->InitWithData(ItemData->WeaponData);
	UGameplayStatics::FinishSpawningActor(Projectile, SpawnXform);

	// 중력: 직선이면 제거, 포물선이면 유지
	if (UProjectileMovementComponent* Movement =
		Projectile->FindComponentByClass<UProjectileMovementComponent>())
	{
		Movement->ProjectileGravityScale =
			(Trajectory == EBVProjectileTrajectory::Straight) ? 0.f : 1.f;
	}

	Projectile->SetLaunchVelocity(LaunchVelocity);

	if (UPrimitiveComponent* ProjectileRoot = Cast<UPrimitiveComponent>(Projectile->GetRootComponent()))
	{
		ProjectileRoot->IgnoreActorWhenMoving(this, true);
	}
}

void AMainCharacter::ConstructBuilding(FVector TargetLocation, TSubclassOf<ABVBuildingBase> BuildingClass, float InConstructionTime)
{
	
	if (!BuildingClass || !ConstructionSiteClass) return;

	FTransform SpawnTransform(FRotator::ZeroRotator, TargetLocation);

	ABVConstructionSite* Site = GetWorld()->SpawnActorDeferred<ABVConstructionSite>(
		ConstructionSiteClass,
		SpawnTransform,
		this,
		this,
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	
	if (Site)
	{
		Site->InitConstruction(BuildingClass, GetGenericTeamId(), InConstructionTime);
		UGameplayStatics::FinishSpawningActor(Site, SpawnTransform);
	}
}

void AMainCharacter::PlayRandomMoveSound()
{
	if (MoveSounds.Num() == 0) return;

	float CurrentTime = GetWorld()->GetTimeSeconds();
	if (CurrentTime - LastMoveSoundTime < MoveSoundCooldown) return;

	int32 RandomIndex = FMath::RandRange(0, MoveSounds.Num()-1);
	if (MoveSounds[RandomIndex])
	{
		UGameplayStatics::PlaySoundAtLocation(this, MoveSounds[RandomIndex], GetActorLocation());
		LastMoveSoundTime = CurrentTime;
	}
}

void AMainCharacter::PlayRandomPickupSound()
{
	if (ItemPickupSounds.Num() == 0) return;
	
	int32 RandomIndex = FMath::RandRange(0, ItemPickupSounds.Num()-1);
	if (ItemPickupSounds[RandomIndex])
	{
		UGameplayStatics::PlaySoundAtLocation(this, ItemPickupSounds[RandomIndex], GetActorLocation());
	}
}

void AMainCharacter::PlayFootstepSound()
{
	if (FootstepSounds.Num() > 0)
	{
		int32 RandomIndex = FMath::RandRange(0, FootstepSounds.Num()-1);
		UGameplayStatics::PlaySoundAtLocation(this, FootstepSounds[RandomIndex], GetActorLocation(), 0.045f);
	}
}

void AMainCharacter::FireDefaultLaserBeam(float DeltaSecond)
{

	TimeSinceLastShot += DeltaSecond;
	if (TimeSinceLastShot < FireInterval) return;

	AActor* Target = FindNearestEnemyInRange();
	if (!Target) return;

	UWorld* World = GetWorld();
	if (!World) return;

	const FVector FireLocation = GetActorLocation();
	const FVector TargetLocation = Target->GetActorLocation();

	FVector FireDir = (TargetLocation - FireLocation).GetSafeNormal();
	if (FireDir.IsNearlyZero()) return;

	FActorSpawnParameters Params;
	Params.Owner = this;
	Params.Instigator = this;

	// Default Laser
	ABVLaserBeamBase* Projectile = World->SpawnActor<ABVLaserBeamBase>(ABVLaserBeamBase::StaticClass(), FireLocation, FireDir.Rotation(), Params);
	if (Projectile)
	{
		Projectile->InitVelocity(FireDir);
		Projectile->InitBeamEnd(FireLocation, TargetLocation);
	}

	TimeSinceLastShot = 0.f;
	
}


class AActor* AMainCharacter::FindNearestEnemyInRange() const
{
	const FVector MyLocation = GetActorLocation();

	AActor* Nearest = nullptr;
	float BestDistSq = FLT_MAX;

	for (const TWeakObjectPtr<AActor>& WeakActor : EnemiesInRange)
	{
		AActor* TargetActor = WeakActor.Get();
		if (!IsValid(TargetActor)) continue;
		if (IBVDamageableInterface::Execute_GetTeamId(TargetActor) == GetGenericTeamId()) continue;
		if (IBVDamageableInterface::Execute_IsDestroyed(TargetActor)) continue;

		const float DistSq = FVector::DistSquared(MyLocation, TargetActor->GetActorLocation());
		if (DistSq < BestDistSq)
		{
			BestDistSq = DistSq;
			Nearest = TargetActor;
		}
		
	}

	return Nearest;
	
}

