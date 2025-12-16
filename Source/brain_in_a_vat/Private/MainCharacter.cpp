// Fill out your copyright notice in the Description page of Project Settings.


#include "MainCharacter.h"

#include <ThirdParty/ShaderConductor/ShaderConductor/External/DirectXShaderCompiler/include/dxc/DXIL/DxilConstants.h>

#include "Components/SkeletalMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "InputMappingContext.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Blueprint/AIBlueprintHelperLibrary.h"
#include "DrawDebugHelpers.h"
#include "Components/SphereComponent.h"
#include "Autobots/BVAutobotBase.h"
#include "Weapons/Projectiles/BVProjectileBase.h"

// Sets default values
AMainCharacter::AMainCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// Capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.f);
	GetCapsuleComponent()->SetCollisionProfileName(TEXT("Pawn"));

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
	GetMesh()->SetCollisionProfileName(TEXT("CharacterMesh"));

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
	
	// --> For Quarter View
	CameraBoom->bInheritPitch = false;
	CameraBoom->bInheritYaw   = false;
	CameraBoom->bInheritRoll  = false;

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	Camera->bUsePawnControlRotation = false;

	// Input

	static ConstructorHelpers::FObjectFinder<UInputMappingContext> InputMappingContextRef(TEXT("/Script/EnhancedInput.InputMappingContext'/Game/Inputs/IMC_Player.IMC_Player'"));
	if (InputMappingContextRef.Succeeded())
	{
		InputMappingContext = InputMappingContextRef.Object;
	}

	static ConstructorHelpers::FObjectFinder<UInputAction> InputActionMoveRef(TEXT("/Script/EnhancedInput.InputAction'/Game/Inputs/IA_Move.IA_Move'"));
	if (InputActionMoveRef.Succeeded())
	{
		MoveAction = InputActionMoveRef.Object;
	}

	static ConstructorHelpers::FObjectFinder<UInputAction> InputActionJumpRef(TEXT("/Script/EnhancedInput.InputAction'/Game/Inputs/IA_Jump.IA_Jump'"));
	if (InputActionJumpRef.Succeeded())
	{
		JumpAction = InputActionJumpRef.Object;
	}

	static ConstructorHelpers::FObjectFinder<UInputAction> RightClickMoveActionRef(TEXT("/Script/EnhancedInput.InputAction'/Game/Inputs/IA_RightClickMove.IA_RightClickMove'"));
	if (RightClickMoveActionRef.Succeeded())
	{
		RightClickMoveAction = RightClickMoveActionRef.Object;
	}

	// Attack Sphere

	AttackRangeSphere = CreateDefaultSubobject<USphereComponent>(TEXT("AttackRange"));
	AttackRangeSphere->SetupAttachment(RootComponent);
	AttackRangeSphere->InitSphereRadius(AttackRange);

	/*
	AttackRangeSphere->SetHiddenInGame(false);
	AttackRangeSphere->bHiddenInGame = false;
	AttackRangeSphere->SetVisibility(true);
	AttackRangeSphere->ShapeColor = FColor::Green;
	AttackRangeSphere->bDrawOnlyIfSelected = false;
	*/
	
	AttackRangeSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	AttackRangeSphere->SetCollisionObjectType(ECC_WorldDynamic);
	AttackRangeSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	AttackRangeSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	AttackRangeSphere->SetGenerateOverlapEvents(true);
	
}

// Called when the game starts or when spawned
void AMainCharacter::BeginPlay()
{
	Super::BeginPlay();

	TObjectPtr<APlayerController> PlayerController = CastChecked<APlayerController>(GetController());
	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
	{
		Subsystem->AddMappingContext(InputMappingContext, 0);
	}

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

void AMainCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	TObjectPtr<UEnhancedInputComponent> EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(PlayerInputComponent);

	EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AMainCharacter::Move);
	EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Triggered, this, &AMainCharacter::Jump);
	EnhancedInputComponent->BindAction(RightClickMoveAction, ETriggerEvent::Started, this, &AMainCharacter::MoveToLocation);
}

void AMainCharacter::Move(const FInputActionValue& Value)
{
	// Quarter View
	FVector2D MovementVector = Value.Get<FVector2D>();
	float Speed = MovementVector.Length();
	MovementVector.Normalize();

	FVector MoveDirection = FVector(MovementVector.X, MovementVector.Y, 0);
	
	AddMovementInput(MoveDirection, Speed);
	
	/* Shoulder View
	FVector2D MovementVector = Value.Get<FVector2D>();

	const FRotator Rotation = Controller->GetControlRotation();
	const FRotator YawRotation(0, Rotation.Yaw, 0);

	const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
	const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);

	AddMovementInput(ForwardDirection, MovementVector.Y);
	AddMovementInput(RightDirection, MovementVector.X);
	*/
}

void AMainCharacter::Look(const FInputActionValue& Value)
{
	
}

void AMainCharacter::MoveToLocation(const FInputActionValue& Value)
{
	// Right Click Move

	APlayerController* PlayerController = Cast<APlayerController>(GetController());
	if (!PlayerController) return;
	
	FHitResult Hit;
	bool bHit = PlayerController->GetHitResultUnderCursor(ECC_Visibility, false, Hit);
	if (!bHit) return;

	const FVector DestLocation = Hit.ImpactPoint;

	// Debug
	DrawDebugSphere(GetWorld(), DestLocation, 25.0f,  12, FColor::Red, false, 1.0f);
	
	UAIBlueprintHelperLibrary::SimpleMoveToLocation(Controller, DestLocation);
	
}

FGenericTeamId AMainCharacter::GetGenericTeamId() const
{
	if (const IGenericTeamAgentInterface* TeamAgent = Cast<IGenericTeamAgentInterface>(GetController()))
	{
		return TeamAgent->GetGenericTeamId();
	}
	return IGenericTeamAgentInterface::GetGenericTeamId();
}

void AMainCharacter::OnAttackRangeBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	ABVAutobotBase* Unit = Cast<ABVAutobotBase>(OtherActor);
	if (!Unit) return;

	if (Unit->GetTeamFlag() == TeamFlag) return;
	if (Unit->bIsDead == true) return;

	EnemiesInRange.AddUnique(Unit);

	UE_LOG(LogTemp, Warning, TEXT("Unit added [%s]"), *Unit->GetName());
}

void AMainCharacter::OnAttackRangeEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	ABVAutobotBase* Unit = Cast<ABVAutobotBase>(OtherActor);
	if (!Unit) return;

	EnemiesInRange.Remove(Unit);
}

void AMainCharacter::AutoFire(float DeltaSecond)
{

	TimeSinceLastShot += DeltaSecond;
	if (TimeSinceLastShot < FireInterval) return;

	ABVAutobotBase* Target = FindNearestEnemyInRange();
	if (!Target) return;

	FireToTarget(Target);
	TimeSinceLastShot = 0.f;
}

void AMainCharacter::FireToTarget(ABVAutobotBase* Target)
{
	
	if (!Target ) return;

	UWorld* World = GetWorld();
	if (!World) return;

	const FVector FireLocation = GetActorLocation();
	const FVector TargetLocation = Target->GetActorLocation();

	FVector FireDir = (TargetLocation - FireLocation).GetSafeNormal();
	if (FireDir.IsNearlyZero()) return;

	FActorSpawnParameters Params;
	Params.Owner = this;
	Params.Instigator = this;

	ABVProjectileBase* Projectile = World->SpawnActor<ABVProjectileBase>(ABVProjectileBase::StaticClass(), FireLocation, FireDir.Rotation(), Params);
	if (Projectile)
	{
		Projectile->InitVelocity(FireDir);
		Projectile->InitBeamEnd(FireLocation, TargetLocation);

	}
	
}

class ABVAutobotBase* AMainCharacter::FindNearestEnemyInRange() const
{
	const FVector MyLocation = GetActorLocation();

	ABVAutobotBase* Nearest = nullptr;
	float BestDistSq = FLT_MAX;

	for (const TWeakObjectPtr<ABVAutobotBase>& WeakUnit : EnemiesInRange)
	{
		ABVAutobotBase* Unit = WeakUnit.Get();
		if (!IsValid(Unit)) continue;
		if (Unit->bIsDead == true) continue;
		if (Unit->GetTeamFlag() == TeamFlag) continue;

		const float DistSq = FVector::DistSquared(MyLocation, Unit->GetActorLocation());
		if (DistSq < BestDistSq)
		{
			BestDistSq = DistSq;
			Nearest = Unit;
		}
		
	}

	return Nearest;
	
}

