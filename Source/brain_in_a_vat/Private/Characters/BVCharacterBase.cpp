// Fill out your copyright notice in the Description page of Project Settings.


#include "Characters/BVCharacterBase.h"

#include "Components/CapsuleComponent.h"
#include "BVPlayerState.h"
#include "Animations/BVAnimInstance.h"
#include "Collision/BVCollision.h"
#include "Components/WidgetComponent.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Widget/BVHealthBarWidget.h"
#include "Widget/BVUnitNameWidget.h"
#include "BVPlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Components/BVStaticHoverRingComponent.h"


// Sets default values
ABVCharacterBase::ABVCharacterBase()
{

	// Capsule
	GetCapsuleComponent()->InitCapsuleSize(30.f, 42.0f);

	// Mesh and Collision
	float CapsuleHalfHeight = GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight();
	GetMesh()->SetRelativeLocationAndRotation(FVector(0.0f, 0.0f, -CapsuleHalfHeight), FRotator(0.0f, -90.0f, 0.0f));
	GetMesh()->SetAnimationMode(EAnimationMode::AnimationBlueprint);
	GetMesh()->SetCollisionProfileName(TEXT("Hoverable"));
	// 유닛이 다른 액터의 호버 링 데칼을 몸에 받지 않도록 차단.
	GetMesh()->SetReceivesDecals(false);

	// [Perf] fog로 숨겨진 유닛은 렌더링되지 않으므로 포즈/본 refresh를 스킵.
	// OnlyTickMontagesWhenNotRendered: 몽타주(+노티파이)는 계속 tick해서
	// PerformAttackHit / FireProjectile 같은 gameplay 노티파이는 정상 발동 — 대신 본 transform
	// 업데이트는 스킵돼 큰 CPU 절약. URO는 화면에 보이되 멀리 있는 유닛의 tick 간격도 자동 조절.
	GetMesh()->bEnableUpdateRateOptimizations = true;
	GetMesh()->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::OnlyTickMontagesWhenNotRendered;

	// <------------ Hover Ring ------------>
	// StaticMesh 기반 링. 캐릭터 Transform에 1:1 attach되어 잔상 없음.
	StaticHoverRingComponent = CreateDefaultSubobject<UBVStaticHoverRingComponent>(TEXT("StaticHoverRingComponent"));
	StaticHoverRingComponent->SetupAttachment(RootComponent);

	// <------------ Widgets ------------>
	// UnitNameWidget
	/*
	UnitNameWidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("UnitNameWidgetComponent"));
	static ConstructorHelpers::FClassFinder<UBVUnitNameWidget> UnitNameWidgetRef(TEXT("/Script/UMGEditor.WidgetBlueprint'/Game/HUD/Widget/WBP_UnitNameWidget.WBP_UnitNameWidget_C'"));
	if (UnitNameWidgetRef.Class != nullptr)
	{
		UnitNameWidgetClass = UnitNameWidgetRef.Class;
		UnitNameWidgetComponent->SetWidgetClass(UnitNameWidgetClass);
	}
	UnitNameWidgetComponent->SetupAttachment(RootComponent);
	UnitNameWidgetComponent->SetRelativeLocation(FVector(0.0f, 0.0f, 50.f));
	UnitNameWidgetComponent->SetWidgetSpace(EWidgetSpace::Screen);
	*/
}

void ABVCharacterBase::BeginPlay()
{
	Super::BeginPlay();

	// Disable Jitter Effect of Hovering
	UKismetSystemLibrary::ExecuteConsoleCommand(GetWorld(), TEXT("r.CustomDepthTemporalAAJitter 0"));

	GetMesh()->SetCollisionProfileName(TEXT("Hoverable"));

	// 스켈레탈 메시는 애니메이션으로 미묘하게 움직여서 커서 트레이스가 간헐적으로 놓침 → 호버 깜빡임.
	// 캡슐도 MouseHover를 블록하게 해서 메시 애니메이션과 무관하게 안정적으로 잡히도록.
	if (UCapsuleComponent* Cap = GetCapsuleComponent())
	{
		Cap->SetCollisionResponseToChannel(ECC_MouseHover, ECR_Block);
	}

	// 유닛 전체가 다른 호버 링 데칼을 받지 않도록 일괄 차단.
	// (메인 메시 외에 무기/부착 메시가 있어도 커버. BP에서 우연히 켜진 경우도 덮어씀.)
	TArray<UPrimitiveComponent*> AllPrims;
	GetComponents<UPrimitiveComponent>(AllPrims);
	for (UPrimitiveComponent* Prim : AllPrims)
	{
		if (Prim)
		{
			Prim->SetReceivesDecals(false);
		}
	}

	// StaticHoverRingComponent의 크기/위치는 자체 BeginPlay에서 캡슐 기반으로 자동 계산됨.

	// 보급 tick: 1초마다. ASC/CombatAttributeSet이 준비되지 않은 프레임엔 handler가 알아서 skip.
	// (AutobotBase는 Super::BeginPlay 이후 ApplyInitStatFromDataAsset에서 Supply 초기값을 세팅)
	if (SupplyDecayPerSecond > 0.f || StarvationDamagePerSecond > 0.f)
	{
		GetWorldTimerManager().SetTimer(
			SupplyTickTimerHandle,
			this,
			&ABVCharacterBase::HandleSupplyTick,
			1.0f,
			true,
			1.0f);
	}

	/*
	// Widget Setting
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
	*/
}

void ABVCharacterBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

FGenericTeamId ABVCharacterBase::GetTeamId_Implementation() const
{
	return GetGenericTeamId();
}

void ABVCharacterBase::SetSelected_Implementation(bool bInSelected)
{
	bIsHovered = bInSelected;
	// 선택 링만 실제 렌더링. Decal 버전은 호출하지 않아 visibility=false 유지.
	if (StaticHoverRingComponent)
	{
		StaticHoverRingComponent->SetHovered(bInSelected, TeamType);
	}
}

void ABVCharacterBase::SetHovered_Implementation(bool bInHovered)
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

void ABVCharacterBase::PlayInteractionSound()
{
}

void ABVCharacterBase::HandleSupplyTick()
{
	// ASC는 서브클래스(AutobotBase 등)만 가짐. 런타임에 interface로 획득 — 없으면 no-op.
	IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(this);
	if (!ASI) return;

	UAbilitySystemComponent* ASCRef = ASI->GetAbilitySystemComponent();
	if (!ASCRef) return;

	UCombatAttributeSet* Attrs = const_cast<UCombatAttributeSet*>(
		ASCRef->GetSet<UCombatAttributeSet>());
	if (!Attrs) return;

	const float CurSupply = Attrs->GetSupply();

	if (CurSupply > 0.f)
	{
		if (SupplyDecayPerSecond > 0.f)
		{
			// PreAttributeChange가 [0, MaxSupply]로 클램프해줌.
			const float NewSupply = CurSupply - SupplyDecayPerSecond;
			ASCRef->SetNumericAttributeBase(UCombatAttributeSet::GetSupplyAttribute(), NewSupply);
		}
		return;
	}

	// Supply == 0 → 아사. Health 직접 감소.
	// (기존 데미지 파이프라인/이펙트를 타지 않는 단순 감소. PreAttributeChange가 0 이하 진입을 막음.)
	if (StarvationDamagePerSecond <= 0.f) return;

	const float CurHealth = Attrs->GetHealth();
	if (CurHealth <= 0.f) return; // 이미 사망 중이면 추가 tick 무시.

	const float NewHealth = CurHealth - StarvationDamagePerSecond;
	ASCRef->SetNumericAttributeBase(UCombatAttributeSet::GetHealthAttribute(), NewHealth);
}

