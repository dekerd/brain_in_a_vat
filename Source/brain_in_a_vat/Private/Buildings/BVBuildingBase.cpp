// Fill out your copyright notice in the Description page of Project Settings.


#include "Buildings/BVBuildingBase.h"
#include "Autobots/BVAutobotBase.h"
#include "Components/WidgetComponent.h"
#include "Widget/BVSpawnCooltimeBar.h"
#include "DrawDebugHelpers.h"

// Sets default values
ABVBuildingBase::ABVBuildingBase()
{
	// Widget

	RespawnWidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("RespawnWidget"));

	static ConstructorHelpers::FClassFinder<UBVSpawnCooltimeBar> SpawnWidgetRef(TEXT("/Script/UMGEditor.WidgetBlueprint'/Game/HUD/Widget/WBP_SpawnCooltimeBar.WBP_SpawnCooltimeBar_C'"));
	if (SpawnWidgetRef.Class != nullptr)
	{
		RespawnWidgetClass = SpawnWidgetRef.Class;
		RespawnWidgetComponent->SetWidgetClass(RespawnWidgetClass);
	}
	
	RespawnWidgetComponent->SetupAttachment(RootComponent);
	RespawnWidgetComponent->SetWidgetSpace(EWidgetSpace::Screen);

}

void ABVBuildingBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (RespawnInterval <= 0.f) return;

	ElapsedTime += DeltaTime;

	float CycleTime = FMath::Fmod(ElapsedTime, RespawnInterval);
	float Percent = CycleTime / RespawnInterval;

	if (RespawnWidget)
	{
		RespawnWidget->SetProgress(Percent);
	}
}

// Called when the game starts or when spawned
void ABVBuildingBase::BeginPlay()
{
	Super::BeginPlay();

	// Get actual actor size
	const FBoxSphereBounds Bounds = StaticMeshComponent->CalcBounds(StaticMeshComponent->GetComponentTransform());
	const float TopZ = Bounds.Origin.Z + Bounds.BoxExtent.Z;

	RespawnWidgetComponent->SetWorldLocation(FVector(Bounds.Origin.X, Bounds.Origin.Y, TopZ + 50.f));
	RespawnWidgetComponent->SetDrawSize(FVector2D(200.f, 10.f));


	if (UUserWidget* UserWidget = RespawnWidgetComponent->GetUserWidgetObject())
	{
		RespawnWidget = Cast<UBVSpawnCooltimeBar>(UserWidget);
	}

	ElapsedTime = 0.0f;

	if (SpawnUnitClass && RespawnInterval > 0.f)
	{
		GetWorldTimerManager().SetTimer(
			SpawnTimerHandle,
			this,
			&ABVBuildingBase::SpawnUnit,
			RespawnInterval,
			true,
			0.0f
			);
	}
	
}

void ABVBuildingBase::SpawnUnit()
{
	
	UE_LOG(LogTemp, Warning, TEXT("SpawnUnit called!"))
	if (!SpawnUnitClass) return;

	UWorld* World = GetWorld();
	if (!World) return;

	// Choosing spawn location
	FVector SpawnLocation = GetActorLocation() + GetActorForwardVector()*200.f;
	const FRotator SpawnRotation = FRotator::ZeroRotator;
	
	DrawDebugSphere(GetWorld(), SpawnLocation, 25.0f, 12, FColor::Blue, false, 1.0f);

	FActorSpawnParameters Params;
	Params.Owner = this;

	ABVAutobotBase* NewSpawnUnit = World->SpawnActor<ABVAutobotBase>(
		SpawnUnitClass,
		SpawnLocation,
		SpawnRotation,
		Params
		);

	if (NewSpawnUnit)
	{
		// Initialization
	}
}
