// Fill out your copyright notice in the Description page of Project Settings.


#include "Characters/BVAutobotBase.h"
#include "AI/BVAIController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "GAS/CombatAttributeSet.h"
#include "Components/CapsuleComponent.h"
#include "AbilitySystemInterface.h"
#include "GAS/GASTags.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BrainComponent.h"
#include "BVPlayerState.h"
#include "Animations/BVAnimInstance.h"
#include "Collision/BVCollision.h"
#include "Components/WidgetComponent.h"
#include "Components/BVHealthComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Widget/BVHealthBarWidget.h"
#include "BVPlayerController.h"
#include "Buildings/BVBuildingBase.h"
#include "Data/BVUnitData.h"
#include "Interface/BVDamageableInterface.h"
#include "Weapons/Projectiles/BVProjectileBase.h"
#include "Widget/BVUnitOverheadWidget.h"
#include "EngineUtils.h"
#include "Data/BVProjectileData.h"


// Sets default values
ABVAutobotBase::ABVAutobotBase()
{
	// AI
	AIControllerClass = ABVAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	// Capsule
	GetCapsuleComponent()->InitCapsuleSize(30.f, 42.0f);
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Item, ECR_Ignore);

	// Mesh and Collision
	float CapsuleHalfHeight = GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight();
	GetMesh()->SetRelativeLocationAndRotation(FVector(0.0f, 0.0f, -CapsuleHalfHeight), FRotator(0.0f, -90.0f, 0.0f));
	GetMesh()->SetAnimationMode(EAnimationMode::AnimationBlueprint);
	GetMesh()->SetCollisionProfileName(TEXT("Hoverable"));
	
	// Movement
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->bUseControllerDesiredRotation = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f);
	
	GetCharacterMovement()->JumpZVelocity = 400.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = 150.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;

	GetCharacterMovement()->bUseRVOAvoidance = true;
	GetCharacterMovement()->AvoidanceConsiderationRadius = 200.f;

	// HealthComponent
	HealthComponent = CreateDefaultSubobject<UBVHealthComponent>(TEXT("HealthComponent"));

	// <------------ Widgets ------------>
	// Unit Overhead Widget
	OverheadWidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("OverheadWidgetComponent"));
	OverheadWidgetComponent->SetupAttachment(RootComponent);
	
	OverheadWidgetComponent->SetWidgetSpace(EWidgetSpace::World);
	OverheadWidgetComponent->SetDrawSize(FVector2D(150.f, 20.f));
	OverheadWidgetComponent->SetRelativeScale3D(FVector(0.5f, 0.5f, 0.5f)); // 이 값을 조절하여 게임 내 위젯 크기를 맞추세요.
	OverheadWidgetComponent->SetUsingAbsoluteRotation(true); // 유닛이 회전해도 위젯은 돌아가지 않도록 고정!

	// Gameplay Ability System (GAS)
	ASC = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("ASC"));
	ASC->SetIsReplicated(true);
	ASC->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);
	
	CombatAttributes = CreateDefaultSubobject<UCombatAttributeSet>(TEXT("CombatAttributes"));

	static ConstructorHelpers::FClassFinder<UGameplayEffect> InitStatGEClass(TEXT("/Script/Engine.Blueprint'/Game/GAS/GE/GE_InitStat.GE_InitStat_C'"));
	if (InitStatGEClass.Succeeded())
	{
		InitStatsEffect = InitStatGEClass.Class;
	}

	static ConstructorHelpers::FClassFinder<UGameplayEffect> DamageGEClass(TEXT("/Script/Engine.Blueprint'/Game/GAS/GE/GE_MeleeDamage.GE_MeleeDamage_C'"));
	if (DamageGEClass.Succeeded())
	{
		DamageEffect = DamageGEClass.Class;
	}

}

void ABVAutobotBase::FixPivotToBottom()
{
	USkeletalMeshComponent* MeshComp = GetMesh();
	UCapsuleComponent* Capsule = GetCapsuleComponent();
	if (!MeshComp || !Capsule || !MeshComp->GetSkeletalMeshAsset()) return;

	const float HalfHeight = Capsule->GetUnscaledCapsuleHalfHeight();
	const FBoxSphereBounds& MeshBounds = MeshComp->GetSkeletalMeshAsset()->GetBounds();
	const float MeshMinZ = MeshBounds.Origin.Z - MeshBounds.BoxExtent.Z;
	const float ZOffset = -HalfHeight - MeshMinZ;

	const FVector CurrentLoc = MeshComp->GetRelativeLocation();
	MeshComp->SetRelativeLocation(FVector(CurrentLoc.X, CurrentLoc.Y, ZOffset));

#if WITH_EDITOR
	Modify();
	MeshComp->Modify();
#endif
}

void ABVAutobotBase::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	// 에디터 뷰포트에서 UnitData가 할당되어 있다면 메쉬와 애니메이션을 즉시 적용!
	if (UnitData)
	{
		if (UnitData->UnitMesh)
		{
			GetMesh()->SetSkeletalMeshAsset(UnitData->UnitMesh);
		}

		if (UnitData->AnimClass)
		{
			GetMesh()->SetAnimInstanceClass(UnitData->AnimClass);
		}
	}

	// 메시의 발을 캡슐 바닥에 맞춰 자동 보정.
	// 피벗이 발에 있든(Min.Z=0) 중앙에 있든(Min.Z<0) 올바른 Z 오프셋을 계산.
	// mesh.Z = -CapsuleHalfHeight - MeshBounds.Min.Z
	if (USkeletalMeshComponent* MeshComp = GetMesh())
	{
		if (USkeletalMesh* SkelMesh = MeshComp->GetSkeletalMeshAsset())
		{
			if (UCapsuleComponent* Capsule = GetCapsuleComponent())
			{
				const float HalfHeight = Capsule->GetUnscaledCapsuleHalfHeight();
				const FBoxSphereBounds& MeshBounds = SkelMesh->GetBounds();
				const float MeshMinZ = MeshBounds.Origin.Z - MeshBounds.BoxExtent.Z;
				const float ZOffset = -HalfHeight - MeshMinZ;

				const FVector CurrentLoc = MeshComp->GetRelativeLocation();
				MeshComp->SetRelativeLocation(FVector(CurrentLoc.X, CurrentLoc.Y, ZOffset));
			}
		}
	}
}


// Called when the game starts or when spawned
void ABVAutobotBase::BeginPlay()
{
	Super::BeginPlay();

	// [Perf] URO는 생성자에서 bEnableUpdateRateOptimizations=true로 켜둠.
	// 화면 안 보이는 유닛은 본/포즈 tick 스킵 (VisibilityBasedAnimTickOption 설정됨).

	// Get the height of the autobot
	float TopZ = GetActorLocation().Z;
	if (GetCapsuleComponent())
	{
		TopZ += GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	}
	

	// Disable Jitter Effect of Hovering
	UKismetSystemLibrary::ExecuteConsoleCommand(GetWorld(), TEXT("r.CustomDepthTemporalAAJitter 0"));

	GetMesh()->SetCollisionProfileName(TEXT("Hoverable"));

	UE_LOG(LogTemp, Warning, TEXT("MeshProfile=%s, MouseHoverResponse=%d"), *GetMesh()->GetCollisionProfileName().ToString(), (int32)GetMesh()->GetCollisionResponseToChannel(ECC_MouseHover));

	// Setting Team Information
	AAIController* AIController = Cast<AAIController>(GetController());

	// [GAS] Initialize ASC
	if (ASC && CombatAttributes)
	{
		ASC->InitAbilityActorInfo(this, this);

		// Initialize Health Component
		if (HealthComponent)
		{
			HealthComponent->InitFromGAS(ASC, CombatAttributes);
		}

		// Initialize GAS stats from DataAsset
		ApplyInitStatFromDataAsset();
		
	}

	// Setting Widget
	if (OverheadWidgetClass && OverheadWidgetComponent)
	{
		OverheadWidgetComponent->SetWidgetClass(OverheadWidgetClass);
		OverheadWidgetComponent->InitWidget();
	}

	if (OverheadWidgetComponent)
	{
		// [수정된 부분] BP 설정을 무시하고 BeginPlay에서 강제 덮어쓰기!
		OverheadWidgetComponent->SetWidgetSpace(EWidgetSpace::World);
		OverheadWidgetComponent->SetUsingAbsoluteRotation(true); 
		OverheadWidgetComponent->SetWorldRotation(FRotator(65.f, 180.f, 0.f)); // 카메라 정면 응시

		// 두께는 WBP의 WorldThickness 프로퍼티로 결정, 길이만 유닛 지름에 비례.
		const FVector2D WidgetDesignSize(120.f, 12.f);

		float WorldThickness = 12.f;
		if (UUserWidget* UserWidget = OverheadWidgetComponent->GetUserWidgetObject())
		{
			if (UBVUnitOverheadWidget* UnitWidget = Cast<UBVUnitOverheadWidget>(UserWidget))
			{
				WorldThickness = UnitWidget->WorldThickness;
			}
		}
		const float BaseScale = WorldThickness / WidgetDesignSize.Y;

		const float UnitDiameter     = GetCapsuleComponent() ? GetCapsuleComponent()->GetScaledCapsuleRadius() * 2.0f : 60.f;
		const float TargetWorldWidth = FMath::Max(UnitDiameter * 1.0f, 40.f);
		const float TargetDrawWidth  = TargetWorldWidth / BaseScale;
		OverheadWidgetComponent->SetDrawSize(FVector2D(TargetDrawWidth, WidgetDesignSize.Y));
		OverheadWidgetComponent->SetWorldScale3D(FVector(BaseScale));

		OverheadWidgetComponent->SetPivot(FVector2D(0.5f, 1.0f));
		FVector WidgetLoc = GetActorLocation();
		WidgetLoc.Z = TopZ + 20.0f;  // 이름이 없으니 머리 바로 위로 가깝게
		OverheadWidgetComponent->SetWorldLocation(WidgetLoc);
		
		if (UUserWidget* UserWidget = OverheadWidgetComponent->GetUserWidgetObject())
		{
			if (UBVUnitOverheadWidget* OverheadWidget = Cast<UBVUnitOverheadWidget>(UserWidget))
			{
				OverheadWidget->InitWithHealthComponent(HealthComponent);
			}
		}

		// Tab으로 전역 off 상태라면 스폰 즉시 위젯 숨기기
		if (ABVPlayerController* BVPC = Cast<ABVPlayerController>(GetWorld()->GetFirstPlayerController()))
		{
			if (!BVPC->AreOverheadWidgetsVisible())
			{
				OverheadWidgetComponent->SetVisibility(false);
			}
		}
	}
	
	// Setting Material
	
	FadeMIDs.Empty();
	USkeletalMeshComponent* MeshComp = GetMesh();
	int32 MatCount = MeshComp->GetNumMaterials();
	for (int32 i = 0; i < MatCount; ++i)
	{
		UMaterialInstanceDynamic* MID = MeshComp->CreateAndSetMaterialInstanceDynamic(i);
		if (MID)
		{
			MID->SetScalarParameterValue(TEXT("Opacity"), 1.0f);
			FadeMIDs.Add(MID);
		}
	}
}

void ABVAutobotBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bIsFading)
	{
		FadeElapsed += DeltaTime;
		float Alpha = FMath::Clamp(FadeElapsed/FadeDuration, 0.0f, 1.0f);

		float NewOpacity = 1.0f - Alpha;
		for (UMaterialInstanceDynamic* MID : FadeMIDs)
		{
			if (MID) {MID->SetScalarParameterValue(TEXT("Opacity"), NewOpacity);}
		}

	}

	// 원거리 유닛은 쿨다운 기반으로 자동 발사 (애님 노티파이와 무관)
	TickRangedAttack(DeltaTime);
}

AActor* ABVAutobotBase::GetBBAttackTarget() const
{
	ABVAIController* AIController = Cast<ABVAIController>(GetController());
	if (!AIController) return nullptr;

	UBlackboardComponent* BB = AIController->GetBlackboardComponent();
	if (!BB) return nullptr;

	static const FName TargetKeyName(TEXT("AttackTargetActor"));
	return Cast<AActor>(BB->GetValueAsObject(TargetKeyName));
}

void ABVAutobotBase::TickRangedAttack(float DeltaTime)
{
	// 원거리 설정이 없는 유닛은 스킵 (기존 근접 플로우 유지)
	if (!UnitData || !UnitData->WeaponData) return;

	UBVAnimInstance* AnimInst = Cast<UBVAnimInstance>(GetMesh() ? GetMesh()->GetAnimInstance() : nullptr);

	// 전투 포즈 해제 — idle 강제 OFF + 재생 중인 공격 몽타주 정지.
	// 사거리 안에서 사격 중이면 아래에서 다시 idle ON / 몽타주는 FireProjectile에서 재생.
	auto ExitCombatPose = [&]()
	{
		if (AnimInst) AnimInst->SetForceIdle(false);
		if (AttackMontage)
		{
			if (UAnimInstance* Anim = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr)
			{
				if (Anim->Montage_IsPlaying(AttackMontage))
				{
					Anim->Montage_Stop(0.2f, AttackMontage);
				}
			}
		}
	};

	if (bIsDead) { ExitCombatPose(); return; }

	// --- 사거리: DA의 ProjectileRange (투사체 최대 비행 거리) ---
	float ProjectileRange = 0.f;
	if (UnitData->WeaponData && UnitData->WeaponData->ProjectileRange > 0.f)
	{
		ProjectileRange = UnitData->WeaponData->ProjectileRange;
	}

	// 감지 범위 = 시야(VisionRadius). 발사 범위 = min(시야, 사거리).
	const float ScanRange = VisionRadius;
	const float FireRange = (ProjectileRange > 0.f) ? FMath::Min(VisionRadius, ProjectileRange) : VisionRadius;

	const auto IsValidTarget = [this](AActor* A) -> bool
	{
		if (!IsValid(A) || A == this) return false;
		if (!A->Implements<UBVDamageableInterface>()) return false;
		if (IBVDamageableInterface::Execute_IsDestroyed(A)) return false;
		if (IBVDamageableInterface::Execute_GetTeamId(A) == GetGenericTeamId()) return false;
		return true;
	};

	// --- 타겟 선정 ---
	// 1) BT가 골라둔 BB 타겟 우선
	AActor* Target = GetBBAttackTarget();

	// BB 타겟이 발사 범위 밖이면 무효 처리
	if (IsValidTarget(Target))
	{
		const float DistToBBTarget = FVector::Dist(GetActorLocation(), Target->GetActorLocation());
		if (DistToBBTarget > FireRange)
		{
			Target = nullptr;
		}
	}

	// 2) BB 타겟이 없거나 사거리 밖이면, 내 주변에서 가장 가까운 적 스캔
	if (!IsValidTarget(Target))
	{
		Target = nullptr;

		const float ScanRangeSq = FMath::Square(ScanRange);
		const FVector MyLoc = GetActorLocation();

		float BestDistSq = FLT_MAX;

		for (TActorIterator<ABVAutobotBase> It(GetWorld()); It; ++It)
		{
			ABVAutobotBase* Other = *It;
			if (!IsValidTarget(Other)) continue;
			const float DistSq = FVector::DistSquared(MyLoc, Other->GetActorLocation());
			if (DistSq <= ScanRangeSq && DistSq < BestDistSq)
			{
				BestDistSq = DistSq;
				Target = Other;
			}
		}

		for (TActorIterator<ABVBuildingBase> It(GetWorld()); It; ++It)
		{
			ABVBuildingBase* Other = *It;
			if (!IsValidTarget(Other)) continue;
			const float DistSq = FVector::DistSquared(MyLoc, Other->GetActorLocation());
			if (DistSq <= ScanRangeSq && DistSq < BestDistSq)
			{
				BestDistSq = DistSq;
				Target = Other;
			}
		}
	}

	// --- 쿨다운 누적 ---
	FireCooldownTimer += DeltaTime;

	if (!Target) { ExitCombatPose(); return; }

	const float DistToTarget = FVector::Dist(GetActorLocation(), Target->GetActorLocation());

	// --- 사거리 안이면 AI 이동 강제 정지 + Velocity 즉시 0 + AnimBP idle 강제 ---
	const bool bInFireRange = (DistToTarget <= FireRange);
	if (bInFireRange)
	{
		if (AAIController* AIController = Cast<AAIController>(GetController()))
		{
			AIController->StopMovement();
			AIController->SetFocus(Target);
		}
		if (UCharacterMovementComponent* Move = GetCharacterMovement())
		{
			Move->StopMovementImmediately();
		}
		if (AnimInst) AnimInst->SetForceIdle(true);
	}
	else
	{
		// 사거리 밖: 전투 포즈 해제 → locomotion SM이 정상 작동 (걷기 모션)
		ExitCombatPose();
	}

	// --- 발사: 쿨다운 + 사거리 안 ---
	const float ShotsPerSecond = (CombatAttributes && CombatAttributes->GetAttackSpeed() > 0.f)
		? CombatAttributes->GetAttackSpeed()
		: 1.f;
	const float FireInterval = 1.f / ShotsPerSecond;

	if (FireCooldownTimer < FireInterval) return;
	if (DistToTarget > FireRange) return;

	FireProjectile(Target);
	FireCooldownTimer = 0.f;
}

void ABVAutobotBase::StartFadeOut()
{
	if (bIsFading) return;
	
	bIsFading = true;
	FadeElapsed = 0.0f;
	
}

void ABVAutobotBase::OnAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
	{
		AnimInstance->OnMontageEnded.RemoveDynamic(this, &ABVAutobotBase::OnAttackMontageEnded);
	}
	
	AAIController* AIController = Cast<AAIController>(GetController());
	OnAttackFinished.Broadcast(AIController);
}

void ABVAutobotBase::PlayFootstepSound()
{
	if (FootstepSounds.Num() > 0)
	{
		int32 RandomIndex = FMath::RandRange(0, FootstepSounds.Num()-1);
		UGameplayStatics::PlaySoundAtLocation(this, FootstepSounds[RandomIndex], GetActorLocation(), FootstepSoundVolume);
	}
}

void ABVAutobotBase::PlayAttackSound()
{
	if (AttackSounds.Num() > 0)
	{
		int32 RandomIndex = FMath::RandRange(0, AttackSounds.Num()-1);
		UGameplayStatics::PlaySoundAtLocation(this, AttackSounds[RandomIndex], GetActorLocation(), AttackSoundVolume);
	}
}

UAbilitySystemComponent* ABVAutobotBase::GetAbilitySystemComponent() const
{
	return ASC;
}

FGenericTeamId ABVAutobotBase::GetTeamId_Implementation() const
{
	return GetGenericTeamId();
}

bool ABVAutobotBase::IsDestroyed_Implementation() const
{
	return bIsDead;
}

void ABVAutobotBase::ApplyInitStatFromDataAsset()
{
	if (!ASC || !UnitData || !CombatAttributes)
	{
		UE_LOG(LogTemp, Error,
			TEXT("[InitStat] %s SKIPPED  ASC=%d UnitData=%d CombatAttributes=%d"),
			*GetName(), ASC != nullptr, UnitData != nullptr, CombatAttributes != nullptr);
		return;
	}

	if (UnitData->OverheadWidgetClass)
	{
		OverheadWidgetClass = UnitData->OverheadWidgetClass;
	}

	if (GetGenericTeamId() == FGenericTeamId::NoTeam && UnitData->TeamType != EBVTeam::Neutral)
	{
		SetGenericTeamId(FGenericTeamId(static_cast<uint8>(UnitData->TeamType)));
	}

	if (UnitData->UnitMesh)
	{
		GetMesh()->SetSkeletalMeshAsset(UnitData->UnitMesh);
	}

	if (UnitData->AnimClass)
	{
		GetMesh()->SetAnimInstanceClass(UnitData->AnimClass);
	}

	VisionRadius = UnitData->VisionRadius;
	AttackMontage = UnitData->AttackMontage;
	DeathMontage  = UnitData->DeathMontage;

	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->MaxWalkSpeed = UnitData->MovementSpeed;
	}

	// --- 초기 Attribute 세팅: GE SetByCaller 체인 대신 직접 Base Value 세팅 ---
	// MaxHealth를 먼저 세팅해야 Health Clamp가 올바르게 동작함.
	ASC->SetNumericAttributeBase(UCombatAttributeSet::GetMaxHealthAttribute(),   UnitData->MaxHealth);
	ASC->SetNumericAttributeBase(UCombatAttributeSet::GetMaxManaAttribute(),     UnitData->MaxMana);
	ASC->SetNumericAttributeBase(UCombatAttributeSet::GetHealthAttribute(),      UnitData->MaxHealth);
	ASC->SetNumericAttributeBase(UCombatAttributeSet::GetManaAttribute(),        UnitData->MaxMana);
	ASC->SetNumericAttributeBase(UCombatAttributeSet::GetHealthRegenAttribute(), UnitData->HealthRegen);
	ASC->SetNumericAttributeBase(UCombatAttributeSet::GetManaRegenAttribute(),   UnitData->ManaRegen);
	ASC->SetNumericAttributeBase(UCombatAttributeSet::GetDamageAttribute(),      UnitData->Damage);
	ASC->SetNumericAttributeBase(UCombatAttributeSet::GetDefenseAttribute(),     UnitData->Defense);
	ASC->SetNumericAttributeBase(UCombatAttributeSet::GetAttackSpeedAttribute(), UnitData->AttackSpeed);
	ASC->SetNumericAttributeBase(UCombatAttributeSet::GetAttackRangeAttribute(), UnitData->AttackRange);
	ASC->SetNumericAttributeBase(UCombatAttributeSet::GetMovementSpeedAttribute(), UnitData->MovementSpeed);

	UE_LOG(LogTemp, Warning,
		TEXT("[InitStat] %s: MaxHP=%.1f HP=%.1f Dmg=%.1f Def=%.1f AS=%.2f"),
		*GetName(),
		CombatAttributes->GetMaxHealth(),
		CombatAttributes->GetHealth(),
		CombatAttributes->GetDamage(),
		CombatAttributes->GetDefense(),
		CombatAttributes->GetAttackSpeed());
}

void ABVAutobotBase::Attack()
{
	if (!AttackMontage) return;
	
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();

	AnimInstance->OnMontageEnded.AddDynamic(this, &ABVAutobotBase::OnAttackMontageEnded);
	
	if (AnimInstance && !AnimInstance->Montage_IsPlaying(AttackMontage))
	{
		AnimInstance->Montage_Play(AttackMontage, GetAttackSpeed());
	}
	
}

void ABVAutobotBase::Dead()
{

	if (bIsDead) return;
	bIsDead = true;

	// Stop Any playing montage
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance)
	{
		AnimInstance->Montage_Stop(0.2f);
	}

	// Stop Movement
	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->DisableMovement();
	}
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// Set AnimInstance bIsDead to 1
	if (UBVAnimInstance* BVAnim = Cast<UBVAnimInstance>(GetMesh()->GetAnimInstance()))
	{
		BVAnim->SetIsDead();
	}

	// Stop AI
	if (ABVAIController* AIController = Cast<ABVAIController>(GetController()) )
	{
		if (UBrainComponent* BrainComp = AIController->GetBrainComponent())
		{
			BrainComp->StopLogic(TEXT("Dead"));
		}

		AIController->StopMovement();
	}

	// Hide Widget — 쓰러지는 즉시 제거
	if (OverheadWidgetComponent)
	{
		OverheadWidgetComponent->SetVisibility(false, true);
		OverheadWidgetComponent->SetHiddenInGame(true, true);
		OverheadWidgetComponent->DestroyComponent();
		OverheadWidgetComponent = nullptr;
	}
	
	// Destroy this object 
	FTimerHandle DeadTimerHandle;
	SetLifeSpan(4.0f);

	// Death Sound

	if (DeathSounds)
	{
		UGameplayStatics::PlaySoundAtLocation(this, DeathSounds, GetActorLocation(), DeathSoundVolume);
	}

	// If Enemy, pay the rewards
	ABVPlayerController* PC = Cast<ABVPlayerController>(GetWorld()->GetFirstPlayerController());

	if (PC)
	{
		if (PC->GetTeamAttitudeTowards(*this) == ETeamAttitude::Hostile)
		{
			ABVPlayerState* PS = PC->GetPlayerState<ABVPlayerState>();
			if (PS && UnitData)
			{
				FVector PopupLocation = GetActorLocation() + FVector(0.f, 0.f, 100.f);

				PC->ShowGoldReward(UnitData->GoldReward, PopupLocation);
				PS->AddRewards(UnitData->GoldReward, UnitData->ExpReward);
			}
		}
	}
}

void ABVAutobotBase::PerformAttackHit()
{
	// This function is called by anim montage notifier
	// This function finds and validates the attack target
	// And call GAS function to finalize the damage application
	
	ABVAIController* AIController = Cast<ABVAIController>(GetController());
	if (!AIController)
	{
		UE_LOG(LogTemp, Warning, TEXT("PerformAttackHit() - No AIController"));
		return;
	}

	UBlackboardComponent* BB = AIController->GetBlackboardComponent();
	if (!BB)
	{
		UE_LOG(LogTemp, Warning, TEXT("PerformAttackHit() - No BB"));
		return;
	}

	static const FName TargetKeyName(TEXT("AttackTargetActor"));
	AActor* TargetActor = Cast<AActor>(BB->GetValueAsObject(TargetKeyName));

	if (!TargetActor)
	{
		UE_LOG(LogTemp, Warning, TEXT("PerformAttackHit() - No Target Actor!"));
		return;
	}

	// 원거리 유닛은 애님 노티파이로 때리지 않는다. Tick 쿨다운이 FireProjectile을 호출한다.
	if (UnitData && UnitData->WeaponData)
	{
		return;
	}

	// 근접: 기존 경로 유지
	ApplyDamageToTarget(TargetActor);
}

void ABVAutobotBase::FireProjectile(AActor* TargetActor)
{
	if (!TargetActor || !UnitData || !UnitData->WeaponData) return;

	UWorld* World = GetWorld();
	if (!World) return;

	// 발사할 때마다 공격 몽타주 재생 (AttackMontage가 슬롯을 덮어쓰므로 ForceIdle 상태와도 호환)
	if (AttackMontage)
	{
		if (UAnimInstance* AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr)
		{
			// 발사 간격이 몽타주 길이보다 짧을 수 있으니 매번 재시작 (Montage_Play가 알아서 처리)
			AnimInstance->Montage_Play(AttackMontage, GetAttackSpeed());
		}
	}

	// --- 스폰 위치: 메시 소켓 우선, 없으면 액터 로컬 오프셋 ---
	FVector SpawnLocation;
	if (USkeletalMeshComponent* MeshComp = GetMesh();
		MeshComp && UnitData->MuzzleSocketName != NAME_None && MeshComp->DoesSocketExist(UnitData->MuzzleSocketName))
	{
		SpawnLocation = MeshComp->GetSocketLocation(UnitData->MuzzleSocketName);
	}
	else
	{
		SpawnLocation = GetActorLocation() + GetActorRotation().RotateVector(UnitData->MuzzleFallbackOffset);
	}

	// --- 타겟 지점: 발 밑 대신 허리 높이 근처 ---
	FVector TargetLocation = TargetActor->GetActorLocation();
	if (const ACharacter* TargetChar = Cast<ACharacter>(TargetActor))
	{
		TargetLocation.Z += TargetChar->GetDefaultHalfHeight() * 0.5f;
	}

	// --- DA에서 궤적/속도 읽기 ---
	const UBVProjectileData* PData = UnitData->WeaponData;
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

	// --- 스폰 (Deferred: DA 주입 후 FinishSpawning에서 BeginPlay가 DA 자동 적용) ---
	const FTransform SpawnXform(LaunchVelocity.Rotation(), SpawnLocation);
	ABVProjectileBase* Projectile = World->SpawnActorDeferred<ABVProjectileBase>(
		ABVProjectileBase::StaticClass(),
		SpawnXform,
		this,
		this,
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn);

	if (!Projectile) return;

	Projectile->InitWithData(UnitData->WeaponData);
	UGameplayStatics::FinishSpawningActor(Projectile, SpawnXform);

	// 유닛의 현재 Damage 스탯으로 투사체 데미지 덮어쓰기
	if (CombatAttributes)
	{
		Projectile->SetDamageAmount(CombatAttributes->GetDamage());
	}

	// 중력/속도 최종 세팅 (DA에서 이미 설정됐지만 궤적 폴백이 발생했을 수 있으므로 확정)
	if (UProjectileMovementComponent* Movement =
		Projectile->FindComponentByClass<UProjectileMovementComponent>())
	{
		Movement->ProjectileGravityScale =
			(Trajectory == EBVProjectileTrajectory::Straight) ? 0.f : 1.f;
	}

	Projectile->SetLaunchVelocity(LaunchVelocity);

	// 자신과는 충돌하지 않도록
	if (UPrimitiveComponent* Root = Cast<UPrimitiveComponent>(Projectile->GetRootComponent()))
	{
		Root->IgnoreActorWhenMoving(this, true);
	}
}

void ABVAutobotBase::ApplyDamageToTarget(AActor* TargetActor)
{
	
	if (!ASC || !DamageEffect || !CombatAttributes || !TargetActor) return;

	IAbilitySystemInterface* TargetASI = Cast<IAbilitySystemInterface>(TargetActor);
	if (!TargetASI)
	{
		UE_LOG(LogTemp, Warning, TEXT("ApplyDamageToTarget() - No Target ASI!"));
		return;
	}

	UAbilitySystemComponent* TargetASC = TargetASI->GetAbilitySystemComponent();
	if (!TargetASC)
	{
		UE_LOG(LogTemp, Warning, TEXT("ApplyDamageToTarget() - No Target ASC!"));
		return;
	}

	FGameplayEffectContextHandle ContextHandle = ASC->MakeEffectContext();
	ContextHandle.AddSourceObject(this);

	FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(DamageEffect, 1.f, ContextHandle);
	if (!SpecHandle.IsValid()) return;

	const float Damage = CombatAttributes->GetDamage();

	SpecHandle.Data->SetSetByCallerMagnitude(TAG_Data_Damage, -Damage);

	ASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);

	// 레인 전투 위치 기록: 공격자/피격자 중간 지점을 "전투 현장"으로 PC에 보고
	if (UWorld* World = GetWorld())
	{
		if (ABVPlayerController* PC = Cast<ABVPlayerController>(UGameplayStatics::GetPlayerController(World, 0)))
		{
			const FVector AttackerLoc = GetActorLocation();
			const FVector TargetLoc = TargetActor->GetActorLocation();
			const FVector CombatMid = (AttackerLoc + TargetLoc) * 0.5f;
			PC->ReportCombatLocation(CombatMid);
		}
	}
}


