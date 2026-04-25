// Fill out your copyright notice in the Description page of Project Settings.


#include "Characters/BVAutobotBase.h"
#include "AI/BVAIController.h"
#include "Navigation/PathFollowingComponent.h" // FPathFollowingRequestResult 완전 정의 (MoveTo 반환형)
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
#include "Buildings/BVCityBase.h"
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

	// Capsule — Unit 프리셋으로 통일 (Pawn 타입, Building/Player Block, Projectile Overlap 등)
	GetCapsuleComponent()->InitCapsuleSize(30.f, 42.0f);
	GetCapsuleComponent()->SetCollisionProfileName(TEXT("Unit"));

	// Mesh — 마우스 호버 전용
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

	// [Perf] URO + OnlyTickMontagesWhenNotRendered는 BVCharacterBase 생성자에서 설정됨.
	// fog로 가려진 유닛은 렌더링 안 돼 본 refresh가 스킵된다.

	// Get the height of the autobot
	float TopZ = GetActorLocation().Z;
	if (GetCapsuleComponent())
	{
		TopZ += GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	}
	

	// Disable Jitter Effect of Hovering
	UKismetSystemLibrary::ExecuteConsoleCommand(GetWorld(), TEXT("r.CustomDepthTemporalAAJitter 0"));

	GetMesh()->SetCollisionProfileName(TEXT("Hoverable"));

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

	// 도시→도시 출격 중이면 도착 판정. (DispatchedToCity가 비어있으면 즉시 return해 거의 무비용.)
	TickDispatchArrival(DeltaTime);
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

	// 전투 포즈 해제 — ForceIdle만 OFF. 몽타주는 자연 완주하도록 건드리지 않는다.
	// (Montage_Stop을 매 tick 걸어버리면 방금 시작된 공격 몽타주가 즉시 잘림)
	auto ExitCombatPose = [&]()
	{
		if (AnimInst) AnimInst->SetForceIdle(false);
	};

	// 죽음 시에만 확실히 몽타주 중단
	if (bIsDead)
	{
		ExitCombatPose();
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
		return;
	}

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
	const bool bInFireRange = (DistToTarget <= FireRange);

	// --- 쿨다운/조준 윈도우 계산 ---
	const float ShotsPerSecond = (CombatAttributes && CombatAttributes->GetAttackSpeed() > 0.f)
		? CombatAttributes->GetAttackSpeed()
		: 1.f;
	const float FireInterval = 1.f / ShotsPerSecond;

	// 조준 윈도우: 발사 직전(쿨다운 완료 근처) + 공격 몽타주 재생 중에는 정지.
	// 몽타주가 끝나면 다음 쿨다운까지 전진 → 산개 효과.
	const bool bReady = (FireCooldownTimer >= FireInterval);

	// 공격 몽타주 재생 여부 — 미끄러짐 방지의 핵심
	bool bAttackMontagePlaying = false;
	if (AttackMontage)
	{
		if (UAnimInstance* Anim = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr)
		{
			bAttackMontagePlaying = Anim->Montage_IsPlaying(AttackMontage);
		}
	}

	// 몽타주가 재생 중이면 사거리/쿨다운과 무관하게 무조건 정지.
	// (미끄러짐 방지 — 몽타주가 끝난 뒤에만 전진 가능)
	const bool bShouldHold = bAttackMontagePlaying || (bInFireRange && bReady);

	if (bShouldHold)
	{
		// 제자리 + 타겟 바라보기.
		// ForceIdle은 쓰지 않는다 — 공격 몽타주 슬롯 출력을 막아버리기 때문.
		if (ABVAIController* AICtl = Cast<ABVAIController>(GetController()))
		{
			AICtl->StopMovement();
			AICtl->SetFocus(Target);

			// BT의 MoveTo 가 이전 프레임에 세팅된 TargetLocation 으로 계속
			// 경로 재계산하면서 캐릭터를 끌고 가는 걸 막는다.
			// 자기 자신 위치로 덮어쓰면 MoveTo 는 "이미 도착"으로 간주.
			if (UBlackboardComponent* BB = AICtl->GetBlackboardComponent())
			{
				BB->SetValueAsVector(TEXT("TargetLocation"), GetActorLocation());
			}
		}
		if (UCharacterMovementComponent* Move = GetCharacterMovement())
		{
			Move->StopMovementImmediately();
			// 관성 제거 — 몽타주 재생 중에 미끄러지는 주 원인.
			Move->Velocity = FVector::ZeroVector;
		}
		if (AnimInst) AnimInst->SetForceIdle(false);
	}
	else
	{
		// 쿨다운이 돌고 있는 동안에는 적 쪽으로 전진.
		// Focus 풀고, BB의 TargetLocation을 적 위치로 밀어서 BT MoveTo가 전진시키게 한다.
		if (ABVAIController* AICtl = Cast<ABVAIController>(GetController()))
		{
			AICtl->ClearFocus(EAIFocusPriority::Gameplay);
			if (UBlackboardComponent* BB = AICtl->GetBlackboardComponent())
			{
				// 적 위치로 직접 이동 — perception의 hold-distance를 이 프레임에서 덮어쓴다.
				BB->SetValueAsVector(TEXT("TargetLocation"), Target->GetActorLocation());
			}
		}
		ExitCombatPose();
	}

	// --- 발사: 쿨다운 + 사거리 안 ---
	if (!bReady) return;
	if (!bInFireRange) return;

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
}

void ABVAutobotBase::Attack()
{
	if (!AttackMontage) return;
	
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();

	AnimInstance->OnMontageEnded.AddDynamic(this, &ABVAutobotBase::OnAttackMontageEnded);
	
	if (AnimInstance && !AnimInstance->Montage_IsPlaying(AttackMontage))
	{
		// 몽타주가 발사 주기(1/AttackSpeed초) 안에 정확히 끝나도록 재생률 산출.
		// PlayRate = MontageLength * AttackSpeed * Multiplier
		const float MontageLen = AttackMontage->GetPlayLength();
		const float Atk = GetAttackSpeed();
		const float Mult = (UnitData && UnitData->AttackMontagePlayRateMultiplier > 0.f)
			? UnitData->AttackMontagePlayRateMultiplier : 1.f;
		const float PlayRate = (MontageLen > KINDA_SMALL_NUMBER && Atk > 0.f)
			? MontageLen * Atk * Mult
			: FMath::Max(Atk * Mult, 0.1f);
		AnimInstance->Montage_Play(AttackMontage, PlayRate);
	}

}

void ABVAutobotBase::Dead()
{

	if (bIsDead) return;
	bIsDead = true;

	// 출격 중 사망 — 목표 도시 약참조 정리. (AddToGarrison이 bIsDead로도 막지만 명시 정리.)
	DispatchedToCity.Reset();

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
	
	// 죽자마자 페이드 시작 → 2초 안에 사라짐
	StartFadeOut();

	// Destroy this object
	FTimerHandle DeadTimerHandle;
	SetLifeSpan(2.0f);

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

	if (bIsDead || !IsValid(this)) return;

	ABVAIController* AIController = Cast<ABVAIController>(GetController());
	if (!AIController)
	{
		return;
	}

	UBlackboardComponent* BB = AIController->GetBlackboardComponent();
	if (!BB)
	{
		return;
	}

	static const FName TargetKeyName(TEXT("AttackTargetActor"));
	AActor* BBTarget = Cast<AActor>(BB->GetValueAsObject(TargetKeyName));

	// 원거리: 노티파이 시점에 실제 투사체 스폰 → 몽타주와 발사 타이밍 동기화
	if (UnitData && UnitData->WeaponData)
	{
		// 노티파이가 도착했으므로 폴백 타이머는 취소 (이중 스폰 방지)
		GetWorldTimerManager().ClearTimer(FireFallbackTimerHandle);

		// PendingFireTarget 이 유효할 때만 발사.
		// 비어있다면 이미 폴백 타이머가 처리했거나 유닛이 더 이상 사격할 상태가 아닌 것이므로
		// BBTarget 으로 폴백하지 않는다 (이중 스폰 방지).
		AActor* FireTarget = PendingFireTarget.Get();
		if (IsValid(FireTarget))
		{
			SpawnProjectileAtTarget(FireTarget);
		}
		PendingFireTarget.Reset();
		return;
	}

	// 근접: BB 타겟 필수
	if (!IsValid(BBTarget))
	{
		return;
	}
	ApplyDamageToTarget(BBTarget);
}

void ABVAutobotBase::FireProjectile(AActor* TargetActor)
{
	if (!TargetActor || !UnitData || !UnitData->WeaponData) return;

	UWorld* World = GetWorld();
	if (!World) return;

	// 실제 스폰은 PerformAttackHit 노티파이 시점까지 미룬다.
	// 여기서는 공격 몽타주만 재생하고 타겟을 기억해둔다.
	PendingFireTarget = TargetActor;

	if (AttackMontage)
	{
		if (UAnimInstance* AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr)
		{
			// 몽타주가 발사 주기 안에 정확히 끝나도록 재생률 산출 (* DA 배수)
			const float MontageLen = AttackMontage->GetPlayLength();
			const float Atk = GetAttackSpeed();
			const float Mult = (UnitData && UnitData->AttackMontagePlayRateMultiplier > 0.f)
				? UnitData->AttackMontagePlayRateMultiplier : 1.f;
			const float PlayRate = (MontageLen > KINDA_SMALL_NUMBER && Atk > 0.f)
				? MontageLen * Atk * Mult
				: FMath::Max(Atk * Mult, 0.1f);
			// 발사 간격이 몽타주 길이보다 짧을 수 있으니 매번 재시작 (Montage_Play가 알아서 처리)
			AnimInstance->Montage_Play(AttackMontage, PlayRate);

			// 안전장치: 몽타주에 PerformAttackHit 노티파이가 배치돼 있지 않으면
			// 노티파이가 영원히 안 온다. 몽타주 길이의 90% 시점에 폴백 스폰을 예약.
			// 노티파이가 먼저 오면 PerformAttackHit 안에서 이 타이머를 취소한다.
			const float FallbackDelay = (MontageLen > KINDA_SMALL_NUMBER && PlayRate > 0.f)
				? (MontageLen / PlayRate) * 0.9f
				: 0.3f;
			GetWorldTimerManager().SetTimer(
				FireFallbackTimerHandle,
				this,
				&ABVAutobotBase::FireFallbackSpawn,
				FMath::Max(FallbackDelay, 0.05f),
				false);
		}
	}
	else
	{
		// 몽타주가 없으면 노티파이가 올 수 없으므로 즉시 스폰
		SpawnProjectileAtTarget(TargetActor);
		PendingFireTarget.Reset();
	}
}

void ABVAutobotBase::FireFallbackSpawn()
{
	// 노티파이가 이미 처리했으면 PendingFireTarget은 비어있다.
	if (!PendingFireTarget.IsValid()) return;

	AActor* FireTarget = PendingFireTarget.Get();
	if (IsValid(FireTarget))
	{
		SpawnProjectileAtTarget(FireTarget);
	}
	PendingFireTarget.Reset();
}

void ABVAutobotBase::SpawnProjectileAtTarget(AActor* TargetActor)
{
	if (!IsValid(TargetActor) || !IsValid(UnitData) || !IsValid(UnitData->WeaponData)) return;
	if (bIsDead) return;

	UWorld* World = GetWorld();
	if (!World || !World->IsGameWorld()) return;

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
			const FVector Direction = (TargetLocation - SpawnLocation).GetSafeNormal();
			LaunchVelocity = Direction * ProjectileSpeed;
			Trajectory = EBVProjectileTrajectory::Straight;
		}
		else
		{
			// CustomArc는 ArcValue + 중력만으로 속도를 결정 → ProjectileSpeed 반영 필요.
			// 속도를 k배 스케일하고 중력을 k^2 배 스케일하면 같은 지점에 착탄한다.
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
			(Trajectory == EBVProjectileTrajectory::Straight) ? 0.f : ArcGravityScale;
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
		return;
	}

	UAbilitySystemComponent* TargetASC = TargetASI->GetAbilitySystemComponent();
	if (!TargetASC)
	{
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

void ABVAutobotBase::TickDispatchArrival(float DeltaTime)
{
	// 출격 중이 아니면 비용 0.
	ABVCityBase* TargetCity = DispatchedToCity.Get();
	if (!TargetCity)
	{
		// GC됐을 수 있으니 약참조도 확실히 비워둠.
		if (DispatchedToCity.IsStale()) DispatchedToCity.Reset();
		return;
	}

	if (bIsDead)
	{
		DispatchedToCity.Reset();
		return;
	}

	// 폴링 간격 누적.
	DispatchArrivalCheckTimer += DeltaTime;
	if (DispatchArrivalCheckTimer < DispatchArrivalCheckInterval) return;
	DispatchArrivalCheckTimer = 0.f;

	// 목표 도시 팀 변경 감지 → 출격 자체 무효화.
	// (Neutral도 합류 불가 — AddToGarrison이 어차피 false 반환할 텐데 미리 차단해 lingering 방지.)
	if (TargetCity->GetGenericTeamId() != GetGenericTeamId())
	{
		DispatchedToCity.Reset();
		return;
	}

	// 도착 판정: 목표 도시 BuildRadius 안 진입 = 도착.
	if (!TargetCity->IsLocationWithinBuildRadius(GetActorLocation()))
	{
		return; // 아직 도착 안 함 — 다음 폴링에 재체크.
	}

	// 등록 시도.
	const bool bRegistered = TargetCity->AddToGarrison(this);

	if (bRegistered)
	{
		// 합류 성공 — 도시 내부 RallyPoint로 자연스럽게 흡수.
		DispatchedToCity.Reset();
		SetRallyDestination(TargetCity->RallyPoint);
	}
	// else: 게리슨 Full / 팀 변동 등 — DispatchedToCity 유지하고 다음 폴링에 재시도.
	//       유닛은 도시 영역 안에서 lingering 상태로 머무름.
}

void ABVAutobotBase::SetRallyDestination(const FVector& NewRallyPoint, bool bManualCommand)
{
	bHasRallyDestination = true;
	RallyDestination = NewRallyPoint;

	ABVAIController* AICon = Cast<ABVAIController>(GetController());
	if (!AICon) return;

	// 플레이어의 수동 명령: BB 키만 세팅하고 BT가 흐름을 전담.
	// BT에 bManualMoveActive 조건의 최상위 분기가 있어, 이 키가 true면 공격/이동 모두 중단하고
	// 수동 이동 분기로 선점 전환. MoveTo 완료 후 ClearManualMoveFlag 태스크가 키를 해제.
	if (bManualCommand)
	{
		if (UBlackboardComponent* BB = AICon->GetBlackboardComponent())
		{
			BB->SetValueAsObject(TEXT("AttackTargetActor"), nullptr);
			BB->SetValueAsBool(TEXT("bIsAttacking"), false);
			BB->SetValueAsBool(TEXT("bManualMoveActive"), true);
			BB->SetValueAsVector(TEXT("TargetLocation"), NewRallyPoint);
		}
		return;
	}

	// 자동 경로: 전투 중이 아닐 때만 BB/MoveTo 갱신.
	// 전투 중이라면 Perception이 끝나면서 자연스럽게 SetTargetLocationFromLaneState/Rally로 돌아오게 두자.
	if (UBlackboardComponent* BB = AICon->GetBlackboardComponent())
	{
		const bool bAttacking = BB->GetValueAsBool(TEXT("bIsAttacking"));
		if (!bAttacking)
		{
			BB->SetValueAsVector(TEXT("TargetLocation"), NewRallyPoint);

			// BT MoveTo가 latent 상태라면 즉시 갱신을 못 받을 수 있으니 명시적 MoveTo 한 번 더.
			FAIMoveRequest Req;
			Req.SetGoalLocation(NewRallyPoint);
			Req.SetAcceptanceRadius(80.f);
			Req.SetUsePathfinding(true);
			Req.SetAllowPartialPath(true);
			AICon->MoveTo(Req);
		}
	}
}
