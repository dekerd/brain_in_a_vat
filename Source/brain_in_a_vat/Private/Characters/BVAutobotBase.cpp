// Fill out your copyright notice in the Description page of Project Settings.


#include "Characters/BVAutobotBase.h"
#include "AI/BVAIController.h"
#include "GameFramework/CharacterMovementComponent.h"
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
#include "Data/BVUnitData.h"
#include "Widget/BVUnitOverheadWidget.h"


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
	
}


// Called when the game starts or when spawned
void ABVAutobotBase::BeginPlay()
{
	Super::BeginPlay();

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

		// DrawSize는 UMG 디자인 캔버스 크기로 고정(내용물 잘리지 않게).
		// 실제 보이는 월드 크기는 Scale3D로 조절한다 — 유닛 캡슐 크기에 비례.
		const FVector2D WidgetDesignSize(250.f, 60.f);
		const float UnitDiameter     = GetCapsuleComponent() ? GetCapsuleComponent()->GetScaledCapsuleRadius() * 2.0f : 60.f;
		const float TargetWorldWidth = FMath::Clamp(UnitDiameter * 0.8f, 50.f, 100.f);
		const float WidgetScale      = TargetWorldWidth / WidgetDesignSize.X;
		OverheadWidgetComponent->SetDrawSize(WidgetDesignSize);
		OverheadWidgetComponent->SetRelativeScale3D(FVector(WidgetScale));

		OverheadWidgetComponent->SetPivot(FVector2D(0.5f, 1.0f));
		FVector WidgetLoc = GetActorLocation();
		WidgetLoc.Z = TopZ + 50.0f;
		OverheadWidgetComponent->SetWorldLocation(WidgetLoc);
		
		if (UUserWidget* UserWidget = OverheadWidgetComponent->GetUserWidgetObject())
		{
			if (UBVUnitOverheadWidget* OverheadWidget = Cast<UBVUnitOverheadWidget>(UserWidget))
			{
				if (UnitData)
				{
					OverheadWidget->SetUnitName(FText::FromName(UnitData->UnitName));
				}
				else
				{
					OverheadWidget->SetUnitName(FText::FromString(TEXT("Unknown")));
				}
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
	if (!ASC || !InitStatsEffect || !UnitData) return;

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

	FGameplayEffectContextHandle GEContext = ASC->MakeEffectContext();
	GEContext.AddInstigator(this, this);

	FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(InitStatsEffect, 1.0f, GEContext);
	if (SpecHandle.IsValid())
	{
		SpecHandle.Data.Get()->SetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag(FName("Data.MaxHealth")), UnitData->MaxHealth);
		SpecHandle.Data.Get()->SetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag(FName("Data.HealthRegen")), UnitData->HealthRegen);
		SpecHandle.Data.Get()->SetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag(FName("Data.MaxMana")), UnitData->MaxMana);
		SpecHandle.Data.Get()->SetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag(FName("Data.ManaRegen")), UnitData->ManaRegen);
		SpecHandle.Data.Get()->SetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag(FName("Data.Damage")), UnitData->Damage);
		SpecHandle.Data.Get()->SetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag(FName("Data.Defence")), UnitData->Defence);
		SpecHandle.Data.Get()->SetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag(FName("Data.AttackSpeed")), UnitData->AttackSpeed);
		SpecHandle.Data.Get()->SetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag(FName("Data.AttackRange")), UnitData->AttackRange);
		
		ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
	}
	
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

	// Hide Widget
	if (OverheadWidgetComponent)
	{
		OverheadWidgetComponent->SetVisibility(false);
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
	

	ApplyDamageToTarget(TargetActor);
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
	
}


