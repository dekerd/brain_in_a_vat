// Fill out your copyright notice in the Description page of Project Settings.


#include "Autobots/BVAutobotBase.h"

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
#include "Widget/BVUnitNameWidget.h"
#include "BVPlayerController.h"


// Sets default values
ABVAutobotBase::ABVAutobotBase()
{
	// AI 
	AIControllerClass = ABVAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	// Capsule
	GetCapsuleComponent()->InitCapsuleSize(30.f, 42.0f);

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
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f);
	
	GetCharacterMovement()->JumpZVelocity = 400.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = 150.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;

	GetCharacterMovement()->bUseRVOAvoidance = true;
	GetCharacterMovement()->AvoidanceConsiderationRadius = 200.f;

	// Unit Stats
	static ConstructorHelpers::FObjectFinder<UDataTable> DT_UnitStats(TEXT("/Script/Engine.DataTable'/Game/Data/UnitStats.UnitStats'"));
	if (DT_UnitStats.Succeeded())
	{
		UnitStatTable = DT_UnitStats.Object;
	}

	// HealthComponent
	HealthComponent = CreateDefaultSubobject<UBVHealthComponent>(TEXT("HealthComponent"));

	// <------------ Widgets ------------>
	// HealthBar Widget
	HealthBarWidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("HealthBarWidgetComponent"));
	HealthBarWidgetComponent->SetupAttachment(RootComponent);
	HealthBarWidgetComponent->SetWidgetSpace(EWidgetSpace::Screen);
	HealthBarWidgetComponent->SetDrawSize(FVector2D(60.f, 5.f));
	HealthBarWidgetComponent->SetRelativeLocation(FVector(0.f, 0.f, 100.f));

	static ConstructorHelpers::FClassFinder<UUserWidget> HealthBarWidgetClassRef(TEXT("/Script/UMGEditor.WidgetBlueprint'/Game/HUD/Widget/WBP_HealthBar.WBP_HealthBar_C'"));
	if (HealthBarWidgetClassRef.Succeeded())
	{
		HealthBarWidgetClass = HealthBarWidgetClassRef.Class;
		HealthBarWidgetComponent->SetWidgetClass(HealthBarWidgetClass);
	}
	
	// UnitNameWidget
	UnitNameWidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("UnitNameWidgetComponent"));
	static ConstructorHelpers::FClassFinder<UBVUnitNameWidget> UnitNameWidgetRef(TEXT("/Script/UMGEditor.WidgetBlueprint'/Game/HUD/Widget/WBP_UnitNameWidget.WBP_UnitNameWidget_C'"));
	if (UnitNameWidgetRef.Class != nullptr)
	{
		UnitNameWidgetClass = UnitNameWidgetRef.Class;
		UnitNameWidgetComponent->SetWidgetClass(UnitNameWidgetClass);
	}
	UnitNameWidgetComponent->SetupAttachment(RootComponent);
	UnitNameWidgetComponent->SetWidgetSpace(EWidgetSpace::Screen);
	
	
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

FGenericTeamId ABVAutobotBase::GetGenericTeamId() const
{
	if (const IGenericTeamAgentInterface* TeamAgent = Cast<IGenericTeamAgentInterface>(GetController()))
	{
		return TeamAgent->GetGenericTeamId();
	}
	return IGenericTeamAgentInterface::GetGenericTeamId();
}

// Called when the game starts or when spawned
void ABVAutobotBase::BeginPlay()
{
	Super::BeginPlay();

	// Disable Jitter Effect of Hovering
	UKismetSystemLibrary::ExecuteConsoleCommand(GetWorld(), TEXT("r.CustomDepthTemporalAAJitter 0"));

	GetMesh()->SetCollisionProfileName(TEXT("Hoverable"));

	UE_LOG(LogTemp, Warning, TEXT("MeshProfile=%s, MouseHoverResponse=%d"), *GetMesh()->GetCollisionProfileName().ToString(), (int32)GetMesh()->GetCollisionResponseToChannel(ECC_MouseHover));

	// Setting Team Information
	AAIController* AIController = Cast<AAIController>(GetController());
	if (AIController)
	{
		AIController->SetGenericTeamId(FGenericTeamId(TeamFlag));
	}

	// [GAS] Initialize ASC
	if (ASC && CombatAttributes)
	{
		ASC->InitAbilityActorInfo(this, this);

		// Initialize Health Component
		if (HealthComponent)
		{
			HealthComponent->InitFromGAS(ASC, CombatAttributes);
		}

		// Initialize GAS stats from DataTable
		ApplyInitStatFromDataTable();
		
	}

	// Setting Widget
	if (HealthBarWidgetComponent)
	{
		if (UUserWidget* Widget = HealthBarWidgetComponent->GetUserWidgetObject())
		{
			if (UBVHealthBarWidget* HealthBar = Cast<UBVHealthBarWidget>(Widget))
			{
				HealthBar->InitWithHealthComponent(HealthComponent);
			}
		}
	}

	if (UnitNameWidgetComponent)
	{
		UUserWidget* WidgetObject = UnitNameWidgetComponent->GetUserWidgetObject();
		if (WidgetObject)
		{
			UBVUnitNameWidget* NameWidget = Cast<UBVUnitNameWidget>(WidgetObject);
			if (NameWidget)
			{
				NameWidget->SetUnitName(UnitName);
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

void ABVAutobotBase::SetHovered_Implementation(bool bInHovered)
{
	bIsHovered = bInHovered;
	if (USkeletalMeshComponent* CharacterMesh = GetMesh())
	{
		uint8 Stencil = 0;

		if (bIsHovered)
		{
			const bool bIsEnemy = (TeamFlag != 1);
			Stencil = bIsEnemy ? 2 : 1;
		}
		
		CharacterMesh->SetRenderCustomDepth(bIsHovered);
		CharacterMesh->SetCustomDepthStencilValue(Stencil);
		
		FString DebugMsg = FString::Printf(TEXT("[%s] is hovered! Stencil : %d"), *GetName(), Stencil);
		GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Orange, DebugMsg);
	}
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

const FUnitStats* ABVAutobotBase::GetUnitStats() const
{
	if (!UnitStatTable || UnitStatRowName.IsNone()) return nullptr;

	return UnitStatTable->FindRow<FUnitStats>(UnitStatRowName, TEXT("UnitStatLookup"));
}

void ABVAutobotBase::ApplyInitStatFromDataTable()
{

	if (!ASC) return;
	if (!InitStatsEffect) return;


	
	const FUnitStats* UnitStats = GetUnitStats();
	if (!UnitStats) return;

	FGameplayEffectContextHandle GEContext = ASC->MakeEffectContext();
	GEContext.AddSourceObject(this);
	
	FGameplayEffectSpecHandle GESpec = ASC->MakeOutgoingSpec(InitStatsEffect, 1.f, GEContext);
	if (!GESpec.IsValid()) return;

	GESpec.Data->SetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag(TEXT("Data.MaxHealth")), UnitStats->MaxHealth);
	GESpec.Data->SetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag(TEXT("Data.Health")), UnitStats->MaxHealth);
	GESpec.Data->SetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag(TEXT("Data.Damage")), UnitStats->Damage);

	ASC->ApplyGameplayEffectSpecToSelf(*GESpec.Data.Get());
	
}

void ABVAutobotBase::Attack()
{
	if (!AttackMontage) return;
	
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();

	AnimInstance->OnMontageEnded.AddDynamic(this, &ABVAutobotBase::OnAttackMontageEnded);
	
	if (AnimInstance && !AnimInstance->Montage_IsPlaying(AttackMontage))
	{
		AnimInstance->Montage_Play(AttackMontage, AttackSpeed);
	}
	
}

void ABVAutobotBase::Dead()
{

	if (bIsDead) return;
	bIsDead = true;

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
	HealthBarWidgetComponent->SetVisibility(false);
	
	// Destroy this object 
	FTimerHandle DeadTimerHandle;
	SetLifeSpan(4.0f);

	// Death Sound

	if (DeathSounds)
	{
		UGameplayStatics::PlaySoundAtLocation(this, DeathSounds, GetActorLocation(), DeathSoundVolume);
	}

	// If Enemy, pay the rewards
	if (TeamFlag == 2)
	{
		ABVPlayerController* PC = Cast<ABVPlayerController>(GetWorld()->GetFirstPlayerController());
		if (PC)
		{
			ABVPlayerState* PS = PC->GetPlayerState<ABVPlayerState>();
			const FUnitStats* Stats = GetUnitStats();

			if (PS && Stats)
			{
				FVector PopupLocation = GetActorLocation() + FVector(0.f, 0.f, 100.f);

				PC->ShowGoldReward(Stats->GoldReward, PopupLocation);
				PS->AddRewards(Stats->GoldReward, Stats->ExpReward);
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

	const float AttackDamage = CombatAttributes->GetAttackDamage();

	SpecHandle.Data->SetSetByCallerMagnitude(TAG_Data_Damage, -AttackDamage);

	ASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);

	UE_LOG(LogTemp, Log, TEXT("%s attacks %s for %.1f damage"), *GetName(), *TargetActor->GetName(), AttackDamage);
	
}


