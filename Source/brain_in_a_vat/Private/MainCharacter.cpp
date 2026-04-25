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
#include "Buildings/BVBuildingBase.h"
#include "Buildings/BVConstructionSite.h"
#include "Components/SphereComponent.h"
#include "Characters/BVAutobotBase.h"
#include "Weapons/Projectiles/BVLaserBeamBase.h"
#include "Collision/BVCollision.h"
#include "Item/BVItemData.h"
#include "Kismet/GameplayStatics.h"
#include "Data/BVProjectileData.h"
#include "Data/BVPlayerData.h"
#include "Weapons/Projectiles/BVProjectileBase.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "AbilitySystemComponent.h"
#include "GAS/CombatAttributeSet.h"
#include "Components/BVHealthComponent.h"
#include "Components/BVStaticHoverRingComponent.h"
#include "Perception/AIPerceptionStimuliSourceComponent.h"
#include "Perception/AISense_Sight.h"
#include "Perception/AISense_Damage.h"
#include "Components/WidgetComponent.h"
#include "Widget/BVUnitOverheadWidget.h"
#include "Blueprint/UserWidget.h"

// Sets default values
AMainCharacter::AMainCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// Capsule — PlayerCharacter 프리셋 (Player 타입, Unit/Building Block, Projectile/Item Overlap 등)
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.f);
	GetCapsuleComponent()->SetCollisionProfileName(TEXT("PlayerCharacter"));
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

	// --- GAS ---
	ASC = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	ASC->SetIsReplicated(false);
	CombatAttributes = CreateDefaultSubobject<UCombatAttributeSet>(TEXT("CombatAttributes"));

	// --- Health component ---
	HealthComponent = CreateDefaultSubobject<UBVHealthComponent>(TEXT("HealthComponent"));

	// --- Hover / Selection Ring ---
	StaticHoverRingComponent = CreateDefaultSubobject<UBVStaticHoverRingComponent>(TEXT("StaticHoverRingComponent"));
	StaticHoverRingComponent->SetupAttachment(RootComponent);

	// --- AI Perception Stimuli (적이 시야로 감지) ---
	StimuliSourceComponent = CreateDefaultSubobject<UAIPerceptionStimuliSourceComponent>(TEXT("StimuliSource"));
	StimuliSourceComponent->bAutoRegister = true;
	StimuliSourceComponent->RegisterForSense(UAISense_Sight::StaticClass());
	StimuliSourceComponent->RegisterForSense(UAISense_Damage::StaticClass());

	// --- Overhead Health Bar ---
	OverheadWidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("OverheadWidgetComponent"));
	OverheadWidgetComponent->SetupAttachment(RootComponent);
	OverheadWidgetComponent->SetWidgetSpace(EWidgetSpace::World);
	OverheadWidgetComponent->SetDrawSize(FVector2D(150.f, 20.f));
	OverheadWidgetComponent->SetRelativeScale3D(FVector(0.5f, 0.5f, 0.5f));
	OverheadWidgetComponent->SetUsingAbsoluteRotation(true);
	OverheadWidgetComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

UAbilitySystemComponent* AMainCharacter::GetAbilitySystemComponent() const
{
	return ASC;
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

		// 바인딩 이전에 이미 겹쳐 있던 액터들(주로 레벨 배치 건물)은 초기 BeginOverlap
		// 이벤트를 놓치므로 여기서 수동으로 처리.
		TArray<AActor*> AlreadyOverlapping;
		VisionSphere->GetOverlappingActors(AlreadyOverlapping);
		for (AActor* Actor : AlreadyOverlapping)
		{
			if (Actor && Actor != this)
			{
				OnVisionRadiusBeginOverlap(VisionSphere, Actor, nullptr, 0, false, FHitResult());
			}
		}
	}

	// --- GAS 초기화 ---
	if (ASC)
	{
		ASC->InitAbilityActorInfo(this, this);
	}

	// --- PlayerData 스탯 주입 ---
	ApplyInitStatFromDataAsset();

	// --- HealthComponent 바인딩 ---
	if (HealthComponent)
	{
		HealthComponent->InitFromGAS(ASC, CombatAttributes);
	}

	// --- Overhead Health Bar Widget ---
	if (OverheadWidgetComponent && PlayerData && PlayerData->OverheadWidgetClass)
	{
		OverheadWidgetComponent->SetWidgetClass(PlayerData->OverheadWidgetClass);
		OverheadWidgetComponent->InitWidget();

		OverheadWidgetComponent->SetWidgetSpace(EWidgetSpace::World);
		OverheadWidgetComponent->SetUsingAbsoluteRotation(true);
		OverheadWidgetComponent->SetWorldRotation(FRotator(65.f, 180.f, 0.f));

		// 캡슐 머리 위로 위치 보정
		const float TopZ = GetCapsuleComponent()
			? GetActorLocation().Z + GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + 30.f
			: GetActorLocation().Z + 120.f;
		FVector WidgetLoc = GetActorLocation();
		WidgetLoc.Z = TopZ;
		OverheadWidgetComponent->SetWorldLocation(WidgetLoc);
		OverheadWidgetComponent->SetPivot(FVector2D(0.5f, 1.0f));

		if (UUserWidget* UserWidget = OverheadWidgetComponent->GetUserWidgetObject())
		{
			if (UBVUnitOverheadWidget* OverheadWidget = Cast<UBVUnitOverheadWidget>(UserWidget))
			{
				OverheadWidget->InitWithHealthComponent(HealthComponent);
			}
		}
	}
}

void AMainCharacter::ApplyInitStatFromDataAsset()
{
	if (!ASC || !CombatAttributes)
	{
		return;
	}

	// PlayerData가 없으면 클래스 기본값으로 폴백
	const float MaxHP   = PlayerData ? PlayerData->MaxHealth     : 500.f;
	const float HPRegen = PlayerData ? PlayerData->HealthRegen   : 0.f;
	const float MaxMP   = PlayerData ? PlayerData->MaxMana       : 100.f;
	const float MPRegen = PlayerData ? PlayerData->ManaRegen     : 0.f;
	const float Dmg     = PlayerData ? PlayerData->Damage        : 20.f;
	const float Def     = PlayerData ? PlayerData->Defense       : 5.f;
	const float AS      = PlayerData ? PlayerData->AttackSpeed   : 1.f;
	const float AR      = PlayerData ? PlayerData->AttackRange   : 1500.f;
	const float MS      = PlayerData ? PlayerData->MovementSpeed : 600.f;

	ASC->SetNumericAttributeBase(UCombatAttributeSet::GetMaxHealthAttribute(),     MaxHP);
	ASC->SetNumericAttributeBase(UCombatAttributeSet::GetHealthAttribute(),        MaxHP);
	ASC->SetNumericAttributeBase(UCombatAttributeSet::GetHealthRegenAttribute(),   HPRegen);
	ASC->SetNumericAttributeBase(UCombatAttributeSet::GetMaxManaAttribute(),       MaxMP);
	ASC->SetNumericAttributeBase(UCombatAttributeSet::GetManaAttribute(),          MaxMP);
	ASC->SetNumericAttributeBase(UCombatAttributeSet::GetManaRegenAttribute(),     MPRegen);
	ASC->SetNumericAttributeBase(UCombatAttributeSet::GetDamageAttribute(),        Dmg);
	ASC->SetNumericAttributeBase(UCombatAttributeSet::GetDefenseAttribute(),       Def);
	ASC->SetNumericAttributeBase(UCombatAttributeSet::GetAttackSpeedAttribute(),   AS);
	ASC->SetNumericAttributeBase(UCombatAttributeSet::GetAttackRangeAttribute(),   AR);
	ASC->SetNumericAttributeBase(UCombatAttributeSet::GetMovementSpeedAttribute(), MS);

	if (GetCharacterMovement())
	{
		GetCharacterMovement()->MaxWalkSpeed = MS;
	}
}

void AMainCharacter::HandleDeath()
{
	if (bIsDead) return;
	bIsDead = true;

	// 입력 / 이동 차단
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		DisableInput(PC);
	}
	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->StopMovementImmediately();
		MoveComp->DisableMovement();
	}
	// 콜리전 off
	if (UCapsuleComponent* Caps = GetCapsuleComponent())
	{
		Caps->SetCollisionEnabled(ECollisionEnabled::NoCollision);
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

	// 팀 필터를 overlap 시점에 걸지 않는다.
	// 도시 점령처럼 사거리 안에서 팀이 바뀌는 케이스에서 BeginOverlap이 다시 발사되지 않아
	// 옛 팀 기준의 결정이 그대로 굳어 버리는 문제가 생긴다 (예: 아군→적 도시 점령 후 미사일 미발사).
	// IGenericTeamAgentInterface를 구현하는 액터만 등록하고, 적/아군 분기는 발사 시점에 한다
	// (FindNearestEnemyInRange가 Damageable 인터페이스로 매 틱 판정).
	if (!Cast<IGenericTeamAgentInterface>(OtherActor)) return;

	EnemiesInRange.AddUnique(OtherActor);

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
		return;
	}

	const UBVProjectileData* PData = Weapon->WeaponData;

	WeaponCoolTime[WeaponIndex] += DeltaSecond;
	if (WeaponCoolTime[WeaponIndex] < PData->FireInterval) return;
	if (GlobalFireTimer < MinFireInterval) return;

	AActor* Target = FindNearestEnemyInRange();

	if (Target)
	{
		// 사거리 게이트: WeaponData->ProjectileRange만 사용 (0 이하면 발사 금지).
		const float EffectiveRange = PData->ProjectileRange;
		if (EffectiveRange <= 0.f)
		{
			return;
		}

		const float Dist = FVector::Dist(GetActorLocation(), Target->GetActorLocation());
		if (Dist > EffectiveRange)
		{
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
	float ArcGravityScale = 1.f; // Arc 궤적에서 ProjectileSpeed에 맞춰 조정
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
		else
		{
			// CustomArc는 ArcValue와 중력만으로 속도를 결정하므로 ProjectileSpeed가 무시된다.
			// 자연 속도를 ProjectileSpeed에 맞춰 스케일하고, 같은 착지점을 유지하려면
			// 중력을 k^2 배로 스케일해야 포물선 모양이 보존된다 (시간이 1/k 로 줄어듦).
			const float NaturalSpeed = LaunchVelocity.Size();
			if (NaturalSpeed > KINDA_SMALL_NUMBER && ProjectileSpeed > 0.f)
			{
				const float k = ProjectileSpeed / NaturalSpeed;
				LaunchVelocity *= k;
				ArcGravityScale = k * k;
			}
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
			(Trajectory == EBVProjectileTrajectory::Straight) ? 0.f : ArcGravityScale;
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

void AMainCharacter::SetSelected_Implementation(bool bInSelected)
{
	// 선택 링 렌더링.
	if (StaticHoverRingComponent)
	{
		StaticHoverRingComponent->SetHovered(bInSelected, TeamType);
	}
}

void AMainCharacter::SetHovered_Implementation(bool bInHovered)
{
	USkeletalMeshComponent* CharacterMesh = GetMesh();
	if (!CharacterMesh) return;

	uint8 Stencil = 0;
	if (bInHovered)
	{
		EBVTeam ViewerTeam = EBVTeam::Player;
		if (APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0))
		{
			if (const IGenericTeamAgentInterface* TeamAgentPC = Cast<IGenericTeamAgentInterface>(PC))
			{
				ViewerTeam = static_cast<EBVTeam>(TeamAgentPC->GetGenericTeamId().GetId());
			}
		}
		Stencil = (TeamType == ViewerTeam) ? 1 : 2;
	}

	CharacterMesh->SetRenderCustomDepth(bInHovered);
	CharacterMesh->SetCustomDepthStencilValue(Stencil);
}

