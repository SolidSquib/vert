// Copyright Inside Out Games Limited 2017

#include "WeaponSpawnVolume.h"
#include "Particles/ParticleSystemComponent.h"

// Sets default values
AWeaponSpawnVolume::AWeaponSpawnVolume()
	: TriggerVolume(CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerVolume")))
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	TriggerVolume->SetCollisionObjectType(ECC_StreamingBounds);
	TriggerVolume->SetCollisionResponseToAllChannels(ECR_Ignore);
	TriggerVolume->SetCollisionResponseToChannel(ECC_CameraPlaceholder, ECR_Overlap);
	SetRootComponent(TriggerVolume);
}

//************************************
// Method:    BeginPlay
// FullName:  AWeaponSpawnVolume::BeginPlay
// Access:    virtual protected 
// Returns:   void
// Qualifier:
//************************************
void AWeaponSpawnVolume::BeginPlay()
{
	Super::BeginPlay();
	
	for (int32 i = 0; i < FMath::Min(NumberOfPods, TargetSpawnPoints.Num()); ++i)
	{
		int32 randomNumber = FMath::RandRange(0, TargetSpawnPoints.Num() - 1);
		while (mChosenTargets.Find(randomNumber) != INDEX_NONE) // ensure that each chosen pod location is unique
		{
			randomNumber = FMath::RandRange(0, TargetSpawnPoints.Num() - 1);
		}
		mChosenTargets.Add(randomNumber);
	}
}

//************************************
// Method:    PostInitializeComponents
// FullName:  AWeaponSpawnVolume::PostInitializeComponents
// Access:    virtual protected 
// Returns:   void
// Qualifier:
//************************************
void AWeaponSpawnVolume::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	TriggerVolume->OnComponentBeginOverlap.AddDynamic(this, &AWeaponSpawnVolume::OnTriggerOverlap);
}

//************************************
// Method:    EndPlay
// FullName:  AWeaponSpawnVolume::EndPlay
// Access:    virtual protected 
// Returns:   void
// Qualifier:
// Parameter: const EEndPlayReason::Type EndPlayReason
//************************************
void AWeaponSpawnVolume::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	GetWorldTimerManager().ClearAllTimersForObject(this);
}

//************************************
// Method:    OnTriggerOverlap_Implementation
// FullName:  AWeaponSpawnVolume::OnTriggerOverlap_Implementation
// Access:    public 
// Returns:   void
// Qualifier:
// Parameter: UPrimitiveComponent * overlappedComponent
// Parameter: AActor * otherActor
// Parameter: UPrimitiveComponent * otherComp
// Parameter: int32 otherBodyIndex
// Parameter: bool fromSweep
// Parameter: const FHitResult & sweepResult
//************************************
void AWeaponSpawnVolume::OnTriggerOverlap_Implementation(UPrimitiveComponent* overlappedComponent, AActor* otherActor, UPrimitiveComponent* otherComp, int32 otherBodyIndex, bool fromSweep, const FHitResult& sweepResult)
{
	AVertPlayerCameraActor* camera = Cast<AVertPlayerCameraActor>(otherActor);
	if (!mTriggered && camera && camera->IsCameraActive())
	{
		InitiateDroppodsRecursive();
		mTriggered = true;
	}
}

//************************************
// Method:    InitiateDroppods
// FullName:  AWeaponSpawnVolume::InitiateDroppods
// Access:    virtual protected 
// Returns:   void
// Qualifier:
//************************************
void AWeaponSpawnVolume::InitiateDroppodsRecursive()
{
	CallSinglePod(mCurrentIndex++);
	
	if (mCurrentIndex >= NumberOfPods)
		return;

	FTimerHandle delayTimer;
	GetWorldTimerManager().SetTimer(delayTimer, [this](void) {
		InitiateDroppodsRecursive();
	}, TimeBetweenSpawns, false);
}

//************************************
// Method:    CallPod
// FullName:  AWeaponSpawnVolume::CallPod
// Access:    protected 
// Returns:   void
// Qualifier:
// Parameter: int32 targetIndex
//************************************
void AWeaponSpawnVolume::CallSinglePod(int32 targetIndex)
{
	if (targetIndex >= mChosenTargets.Num())
		return;

	if (mChosenTargets[targetIndex] >= TargetSpawnPoints.Num())
		return;

	FVector podTarget = TargetSpawnPoints[targetIndex]->GetActorLocation();
	int32 randomSeed = FMath::Rand();
	FRandomStream PodRandomStream(randomSeed);
	FVector direction = PodRandomStream.VRandCone(FVector::UpVector, FMath::DegreesToRadians(30.f), FMath::DegreesToRadians(30.f));
	direction.Y = 0;
	FTransform spawnTransform(FQuat::Identity.Rotator(), podTarget + (direction * 5000.f));

	UParticleSystemComponent* particleSystem = UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), WarningFX, spawnTransform);
	if (particleSystem)
	{
		particleSystem->SetVectorParameter(WarningBeamEndParam, podTarget);
	}

	FTimerHandle warningTimer;
	GetWorldTimerManager().SetTimer(warningTimer, [this, spawnTransform, direction](void) mutable {

		if (!this)
			return;
		
		AVertGameMode* gameMode = GetWorld()->GetAuthGameMode<AVertGameMode>();
		if (gameMode)
		{
			TArray<TSubclassOf<AActor>> payload;
			for (int32 i = 0; i < NumberOfItemsPerPod; ++i)
			{
				payload.Add(gameMode->AvailableItemSpawns[FMath::RandRange(0, gameMode->AvailableItemSpawns.Num() - 1)]);
			}
			gameMode->PrepareDroppod(payload, spawnTransform, -direction);
		}
	}, WarningTime, false);
}