// Fill out your copyright notice in the Description page of Project Settings.


#include "MainCharacter.h"
#include "Components/PrimitiveComponent.h"

#include <ThirdParty/ShaderConductor/ShaderConductor/External/DirectXShaderCompiler/include/dxc/DXIL/DxilConstants.h>

#include "Components/SkeletalMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "DrawDebugHelpers.h"
#include "Components/SphereComponent.h"
#include "Autobots/BVAutobotBase.h"
#include "Weapons/Projectiles/BVLaserBeamBase.h"
#include "Collision/BVCollision.h"
#include "Item/BVItemData.h"
#include "Kismet/GameplayStatics.h"

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

	// Attack Sphere
	AttackRangeSphere = CreateDefaultSubobject<USphereComponent>(TEXT("AttackRange"));
	AttackRangeSphere->SetupAttachment(RootComponent);
	AttackRangeSphere->InitSphereRadius(AttackRange);
	
	AttackRangeSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	AttackRangeSphere->SetCollisionObjectType(ECC_WorldDynamic);
	AttackRangeSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	AttackRangeSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	AttackRangeSphere->SetCollisionResponseToChannel(ECC_Building, ECR_Overlap);
	AttackRangeSphere->SetGenerateOverlapEvents(true);

	// Weapon Cooltime
	WeaponCoolTime.Init(0.0f, 5);
	
}

// Called when the game starts or when spawned
void AMainCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (AttackRangeSphere)
	{
		AttackRangeSphere->SetSphereRadius(AttackRange);
		AttackRangeSphere->OnComponentBeginOverlap.AddDynamic(this, &AMainCharacter::OnAttackRangeBeginOverlap);
		AttackRangeSphere->OnComponentEndOverlap.AddDynamic(this, &AMainCharacter::OnAttackRangeEndOverlap);
	}
	
}

// Called every frame
void AMainCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	AutoFire(DeltaTime);

}

FGenericTeamId AMainCharacter::GetGenericTeamId() const
{
	if (const IGenericTeamAgentInterface* TeamAgent = Cast<IGenericTeamAgentInterface>(GetController()))
	{
		return TeamAgent->GetGenericTeamId();
	}
	return IGenericTeamAgentInterface::GetGenericTeamId();
}

bool AMainCharacter::AddItemToInventory(class UBVItemData* ItemData)
{
	if (ItemData)
	{
		InventoryItems.Add(ItemData);

		PlayRandomPickupSound();
		OnInventoryUpdated.Broadcast();
		
		FString DebugMsg = FString::Printf(TEXT("Item Added to Inventory: %s"), *ItemData->ItemName.ToString());
		GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Blue, DebugMsg);
		
		return true;
	}

	return false;
}

void AMainCharacter::OnAttackRangeBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
                                               UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{

	UE_LOG(LogTemp, Warning, TEXT("Target actor adding start : [%s]"), *OtherActor->GetName());
	if (!OtherActor) return;
	if (!OtherActor->Implements<UBVDamageableInterface>()) return;

	const uint8 OtherActorTeamId = IBVDamageableInterface::Execute_GetTeamId(OtherActor);
	if (OtherActorTeamId == TeamFlag) return;
	
	if (IBVDamageableInterface::Execute_IsDestroyed(OtherActor) == true) return;

	EnemiesInRange.AddUnique(OtherActor);

	UE_LOG(LogTemp, Warning, TEXT("Target actor added : [%s]"), *OtherActor->GetName());
}

void AMainCharacter::OnAttackRangeEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
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
	// Fire Default Laser Beam
	FireDefaultLaserBeam(DeltaSecond);

	// Fire weapons equipped
	for (int32 i = 0; i < InventoryItems.Num(); ++i)
	{
		if (InventoryItems[i])
		{
			FireWeapons(DeltaSecond, i);
		}
	}
	
}

void AMainCharacter::FireWeapons(float DeltaSecond, int32 WeaponIndex)
{

	UBVItemData* Weapon = InventoryItems[WeaponIndex];
	
	WeaponCoolTime[WeaponIndex] += DeltaSecond;
	if (WeaponCoolTime[WeaponIndex] < Weapon->FireInterval) return;
	if (GlobalFireTimer < MinFireInterval) return;

	AActor* Target = FindNearestEnemyInRange();
	if (Target)
	{
		FireDefaultMissile(Weapon, Target);
		WeaponCoolTime[WeaponIndex] = 0.0f;
		GlobalFireTimer = 0.0f;
	}

}

void AMainCharacter::FireDefaultMissile(UBVItemData* ItemData, AActor* Target)
{
	FString DebugMsg = FString::Printf(TEXT("Fire %s to %s"), *ItemData->ItemName.ToString(), *Target->GetName());
	GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Blue, DebugMsg);

	if (!ItemData || !Target || !ItemData->ProjectileClass) return;

	UWorld* World = GetWorld();
	if (!World) return;

	FVector SpawnLocation = GetActorLocation();
	FVector ZOffset = FVector(0.0f, 0.0f, 30.0f);
	SpawnLocation += ZOffset;
	
	const FVector TargetLocation = Target->GetActorLocation();
	FVector FireDir = (TargetLocation - SpawnLocation).GetSafeNormal();

	FVector LaunchVelocity;
	float ArcValue = 0.5f;

	bool bHaveResult = UGameplayStatics::SuggestProjectileVelocity_CustomArc(
		this,
		LaunchVelocity,
		SpawnLocation,
		TargetLocation,
		GetWorld()->GetGravityZ(),
		ArcValue);

	FActorSpawnParameters Params;
	Params.Owner = this;
	Params.Instigator = this;

	if (bHaveResult)
	{
		ABVProjectileBase* Projectile = World->SpawnActor<ABVProjectileBase>(
			ItemData->ProjectileClass,
			SpawnLocation,
			LaunchVelocity.Rotation(),
			Params);

		if (Projectile)
		{
			Projectile->SetLaunchVelocity(LaunchVelocity);
			if (UPrimitiveComponent* ProjectileRoot = Cast<UPrimitiveComponent>(Projectile->GetRootComponent()))
			{
				ProjectileRoot->IgnoreActorWhenMoving(this, true);
			}
		}
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
		if (IBVDamageableInterface::Execute_GetTeamId(TargetActor) == TeamFlag) continue;
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

