// Copyright Inside Out Games Ltd. 2017

#include "DropPod.h"
#include "Interactives/Interactive.h"
#include "VertGameMode.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Components/BoxComponent.h"
#include "Components/SphereComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Weapons/BaseWeapon.h"
#include "Engine/VertUtilities.h"
#include "Engine/VertGlobals.h"
#include "AkGameplayStatics.h"

DECLARE_LOG_CATEGORY_CLASS(LogDropPod, All, Log);

ADropPod::ADropPod()
	: MeshComponent(CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("DroppodMesh"))),
	ProjectileMovement(CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovementComponent"))),
	PodCollision(CreateDefaultSubobject<UBoxComponent>(TEXT("PodCollision"))),
	CorrectionSphere(CreateDefaultSubobject<USphereComponent>(TEXT("CorrectionSphere")))
{
	PodCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	PodCollision->SetCollisionObjectType(ECC_WorldDynamic);
	PodCollision->SetBoxExtent({ 50.f, 50.f, 50.f });
	PodCollision->SetCanEverAffectNavigation(false);
	RootComponent = PodCollision;

	CorrectionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CorrectionSphere->SetCollisionObjectType(ECC_WorldDynamic);
	CorrectionSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	CorrectionSphere->SetupAttachment(PodCollision);

	MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	MeshComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
	MeshComponent->SetupAttachment(RootComponent);
	MeshComponent->SetCanEverAffectNavigation(false);

	ProjectileMovement->SetUpdatedComponent(RootComponent);
	ProjectileMovement->bInitialVelocityInLocalSpace = true;
	ProjectileMovement->SetCanEverAffectNavigation(false);

	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
}

//************************************
// Method:    BeginPlay
// FullName:  ADropPod::BeginPlay
// Access:    virtual protected 
// Returns:   void
// Qualifier:
//************************************
void ADropPod::BeginPlay()
{
	Super::BeginPlay();
	
	UpdateMeshColours(FLinearColor::Black);

	PodCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PodCollision->SetCollisionResponseToChannels(mSavedCollisionProfile);
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	OnPoolBeginPlay.AddDynamic(this, &ADropPod::PoolBeginPlay);
	OnPoolEndPlay.AddDynamic(this, &ADropPod::PoolEndPlay);

	if (!AutoDrop)
	{
		ProjectileMovement->Deactivate();
		ProjectileMovement->SetUpdatedComponent(NULL);
	}
}

void ADropPod::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
	GetWorldTimerManager().ClearAllTimersForObject(this);
}

//************************************
// Method:    PostInitializeComponents
// FullName:  ADropPod::PostInitializeComponents
// Access:    virtual protected 
// Returns:   void
// Qualifier:
//************************************
void ADropPod::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	if (MeshComponent)
	{
		// create material instance for setting team colors (3rd person view)
		for (int32 iMat = 0; iMat < MeshComponent->GetNumMaterials(); iMat++)
		{
			MeshMIDs.Add(MeshComponent->CreateAndSetMaterialInstanceDynamic(iMat));
		}
	}

	ProjectileMovement->OnProjectileStop.AddUniqueDynamic(this, &ADropPod::OnPodHit);
	PodCollision->OnComponentBeginOverlap.AddUniqueDynamic(this, &ADropPod::OnPodOverlap);

	mSavedCollisionProfile = PodCollision->GetCollisionResponseToChannels();
}

//************************************
// Method:    Tick
// FullName:  ADropPod::Tick
// Access:    virtual public 
// Returns:   void
// Qualifier:
// Parameter: float DeltaSeconds
//************************************
void ADropPod::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if ((AutoManageCollision || mCollisionTriggered) && UVertUtilities::IsActorInFrustum(GetWorld(), this))
	{
		PodCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		mPassedScreen = true;
	}
	else if (mPassedScreen)
	{
		DisableAndDestroy();
	}

	if(mMadeImpact)
		SetActorLocation(FMath::VInterpTo(GetActorLocation(), mDigTargetLocation, DeltaSeconds, DigInterpSpeed));
}

//************************************
// Method:    FellOutOfWorld
// FullName:  ADropPod::FellOutOfWorld
// Access:    virtual public 
// Returns:   void
// Qualifier:
// Parameter: const class UDamageType & dmgType
//************************************
void ADropPod::FellOutOfWorld(const class UDamageType& dmgType)
{
	DisableAndDestroy();
}

//************************************
// Method:    DisableAndDestroy
// FullName:  ADropPod::DisableAndDestroy
// Access:    protected 
// Returns:   void
// Qualifier:
//************************************
void ADropPod::DisableAndDestroy()
{
	if (!mEjectedPayload && mPayloadPawnControllers.Num() > 0)
	{
		for (auto iter : mPayloadPawnControllers)
		{
			if (iter.Value.IsValid())
			{
				iter.Value->FailedToSpawnPawn();
			}
		}
	}

	GetWorldTimerManager().ClearAllTimersForObject(this);
	MeshComponent->SetVisibility(true, true);

	if (DespawnFX)
	{
		UParticleSystemComponent* psc = UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), DespawnFX, GetActorTransform());
	}

	ReturnToPool();
}

//************************************
// Method:    StartDespawnTimer
// FullName:  ADropPod::StartDespawnTimer
// Access:    protected 
// Returns:   void
// Qualifier:
//************************************
void ADropPod::StartDespawnTimer()
{
	GetWorldTimerManager().SetTimer(mTimerHandle_Despawn, this, &ADropPod::PrepareForDespawn, DespawnTriggerTime, false);
}

//************************************
// Method:    PrepareForDespawn
// FullName:  ADropPod::PrepareForDespawn
// Access:    protected 
// Returns:   void
// Qualifier:
//************************************
void ADropPod::PrepareForDespawn()
{
	GetWorldTimerManager().SetTimer(mTimerHandle_DespawnFlash, this, &ADropPod::DespawnFlash, FlashSpeed, true);
	GetWorldTimerManager().SetTimer(mTimerHandle_DespawnFinish, this, &ADropPod::DisableAndDestroy, FlashForTimeBeforeDespawning, false);
}

//************************************
// Method:    DespawnFlash
// FullName:  ADropPod::DespawnFlash
// Access:    protected 
// Returns:   void
// Qualifier:
//************************************
void ADropPod::DespawnFlash()
{
	if (MeshComponent->IsVisible())
		MeshComponent->SetVisibility(false, true);
	else
		MeshComponent->SetVisibility(true, true);
}

//************************************
// Method:    CancelDespawn
// FullName:  ADropPod::CancelDespawn
// Access:    protected 
// Returns:   void
// Qualifier:
//************************************
void ADropPod::CancelDespawn()
{
	if (GetWorldTimerManager().IsTimerActive(mTimerHandle_Despawn))
		GetWorldTimerManager().ClearTimer(mTimerHandle_Despawn);

	if (GetWorldTimerManager().IsTimerActive(mTimerHandle_DespawnFlash))
		GetWorldTimerManager().ClearTimer(mTimerHandle_DespawnFlash);

	if (GetWorldTimerManager().IsTimerActive(mTimerHandle_DespawnFinish))
		GetWorldTimerManager().ClearTimer(mTimerHandle_DespawnFinish);

	MeshComponent->SetVisibility(true, true);
}

//************************************
// Method:    EnablePodCollision
// FullName:  ADropPod::EnablePodCollision
// Access:    public 
// Returns:   void
// Qualifier:
//************************************
void ADropPod::EnablePodCollision()
{
	mCollisionTriggered = true;
}

//************************************
// Method:    GetCollisionBoxHafExtents
// FullName:  ADropPod::GetCollisionBoxHafExtents
// Access:    public 
// Returns:   FVector
// Qualifier: const
//************************************
FVector ADropPod::GetCollisionBoxExtents() const
{
	return PodCollision->GetScaledBoxExtent();
}

float ADropPod::GetCorrectionSphereRadius() const
{
	return CorrectionSphere->GetScaledSphereRadius();
}

//************************************
// Method:    PoolBeginPlay
// FullName:  ADropPod::PoolBeginPlay
// Access:    virtual protected 
// Returns:   void
// Qualifier:
//************************************
void ADropPod::PoolBeginPlay()
{
	ProjectileMovement->Activate();
	ProjectileMovement->SetUpdatedComponent(RootComponent);

	PodCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PodCollision->SetCollisionResponseToChannels(mSavedCollisionProfile);
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	SetActorTickEnabled(true);

	if (AVertGameMode* gameMode = GetWorld()->GetAuthGameMode<AVertGameMode>())
	{
		gameMode->RegisterActorToFollow(this);
	}

	if (!AutoDrop)
	{
		ProjectileMovement->Deactivate();
		ProjectileMovement->SetUpdatedComponent(NULL);
	}
}

//************************************
// Method:    PoolEndPlay
// FullName:  ADropPod::PoolEndPlay
// Access:    virtual protected 
// Returns:   void
// Qualifier:
//************************************
void ADropPod::PoolEndPlay()
{
	GetWorldTimerManager().ClearAllTimersForObject(this);

	UpdateMeshColours(FLinearColor::Black);
	mMadeImpact = false;
	mEjectedPayload = false;
	mPassedScreen = false;

	PodCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PodCollision->SetCollisionResponseToChannels(mSavedCollisionProfile);
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	PayloadActors.Empty();

	if (AVertGameMode* gameMode = GetWorld()->GetAuthGameMode<AVertGameMode>())
	{
		gameMode->UnregisterActorToFollow(this);
	}

	OwningController = nullptr;
}

//************************************
// Method:    UpdateMeshColours
// FullName:  ADropPod::UpdateMeshColours
// Access:    public 
// Returns:   void
// Qualifier:
// Parameter: const FLinearColor & newColour
//************************************
void ADropPod::UpdateMeshColours(const FLinearColor& newColour)
{
	for (auto id : MeshMIDs)
	{
		id->SetVectorParameterValue(TEXT("Drop_Player_Colour"), newColour);
	}
}

void ADropPod::AddPayloadPawn(TSubclassOf<APawn> pawnClass, AController* owningController)
{
	int32 index = PayloadActors.Add(pawnClass);
	mPayloadPawnControllers.Add(index, owningController);
	
	OwningController = owningController;
}

//************************************
// Method:    AddPayloadActor
// FullName:  ADropPod::AddPayloadActor
// Access:    public 
// Returns:   void
// Qualifier:
// Parameter: AActor * actor
//************************************
void ADropPod::AddPayloadActor(TSubclassOf<AActor> actorClass)
{
	PayloadActors.Add(actorClass);
}

//************************************
// Method:    InitPodVelocity
// FullName:  ADropPod::InitPodVelocity
// Access:    public 
// Returns:   void
// Qualifier:
// Parameter: const FVector & velocity
//************************************
void ADropPod::InitPodVelocity(const FVector& direction)
{
	if (ProjectileMovement)
	{
		FRotator rotator = FRotator(90, 0, 0);
		FRotator desiredRotation = rotator.RotateVector(direction).Rotation();
		desiredRotation.Yaw = GetActorRotation().Yaw;
		desiredRotation.Roll = GetActorRotation().Roll;
		SetActorRotation(desiredRotation);

		ProjectileMovement->Activate();
		ProjectileMovement->SetUpdatedComponent(RootComponent);
		OnPodInitVelocity();
		ProjectileMovement->Velocity = direction * ProjectileMovement->InitialSpeed;
	}
}

//************************************
// Method:    EjectPayload
// FullName:  ADropPod::EjectPayload
// Access:    virtual protected 
// Returns:   void
// Qualifier:
//************************************
void ADropPod::EjectPayload()
{
	if (!mEjectedPayload)
	{
		mEjectedPayload = true;

		if (EjectAnimation)
		{
			MeshComponent->PlayAnimation(EjectAnimation, false);
		}

		for (int32 i = 0; i < PayloadActors.Num(); ++i)
		{
			float randomSeed = FMath::Rand();
			FRandomStream randomStream(randomSeed);
			const float spread = 30.f;
			const float coneHalfAngle = FMath::DegreesToRadians(spread * 0.5f);
			FVector impulseDirection = randomStream.VRandCone(FVector::UpVector, coneHalfAngle, coneHalfAngle);
			impulseDirection.Y = 0.f;

			AActor* actor = SpawnActorFromPod(i);
			APawn* pawn = Cast<APawn>(actor);

			if (actor)
			{
				if (ACharacter* character = Cast<ACharacter>(actor))
				{
					character->LaunchCharacter(FVector::UpVector * 5000.f, true, true);
				}
				else if (ABaseWeapon* weapon = Cast<ABaseWeapon>(actor))
				{
					weapon->SetActorRotation((-FVector::RightVector).Rotation());
				}

				OnActorSpawned.Broadcast(actor);
				if (pawn)
				{
					OnPlayerPawnSpawned.Broadcast(pawn);
				}
				else if (mPayloadPawnControllers.Find(i) && !pawn)
				{
					OnPlayerPawnSpawnFailed.Broadcast();
				}

				UE_LOG(LogDropPod, Log, TEXT("Ejected payload member %s"), *PayloadActors[i]->GetName());
			}
		}

		if (OnEjectSound)
		{
			UAkGameplayStatics::PostEvent(OnEjectSound, this);
		}
	}
	else
	{
		UE_LOG(LogDropPod, Log, TEXT("Droppod already ejecting payload. Second call invalid."));
	}

	StartDespawnTimer();
}

//************************************
// Method:    EjectSingleActor
// FullName:  ADropPod::EjectSingleActor
// Access:    virtual protected 
// Returns:   void
// Qualifier:
//************************************
AActor* ADropPod::SpawnActorFromPod(int32 index)
{
	if (index < PayloadActors.Num())
	{
		FVector location = GetActorLocation();
		FTransform spawnTransform(GetActorRotation(), { location.X, location.Y, location.Z + DigTargetDepth });

		TSubclassOf<AActor> spawnee = PayloadActors[index];
		if (spawnee->IsChildOf(AInteractive::StaticClass()))
		{
			if (AVertGameMode* gm = GetWorld()->GetAuthGameMode<AVertGameMode>())
			{
				TSubclassOf<AInteractive> itemClass = *spawnee;
				AInteractive* item = gm->RequestItemSpawn(itemClass, spawnTransform);
				if (item)
				{
					return item;
				}
			}
		}
		else
		{
			AActor* actor = UGameplayStatics::BeginDeferredActorSpawnFromClass(this, spawnee, spawnTransform, ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
			if (actor)
			{
				UGameplayStatics::FinishSpawningActor(actor, spawnTransform);
				return actor;
			}
		}

	}

	return nullptr;
}

//************************************
// Method:    DespawnPod
// FullName:  ADropPod::DespawnPod
// Access:    virtual protected 
// Returns:   void
// Qualifier:
//************************************
void ADropPod::DespawnPod()
{
	ReturnToPool();
}

//************************************
// Method:    OnPodHit
// FullName:  ADropPod::OnPodHit
// Access:    protected 
// Returns:   void
// Qualifier:
// Parameter: UPrimitiveComponent * hitComponent
// Parameter: AActor * otherActor
// Parameter: UPrimitiveComponent * otherComp
// Parameter: FVector normalImpulse
// Parameter: const FHitResult & hit
//************************************
void ADropPod::OnPodHit_Implementation(const FHitResult& hit)
{
	if (!mMadeImpact)
	{
		mMadeImpact = true;
		OnPodMadeImpact();
		PodCollision->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Overlap);
		PodCollision->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
		mDigTargetLocation = GetActorLocation() + ((-hit.ImpactNormal) * DigTargetDepth);

		if (OnImpactSound)
		{
			UAkGameplayStatics::PostEvent(OnImpactSound, this);
		}

		GetWorldTimerManager().SetTimer(mTimerHandle_SpawnTimer, this, &ADropPod::EjectPayload, .5f, false);
	}
}

//************************************
// Method:    OnPodOverlap
// FullName:  ADropPod::OnPodOverlap
// Access:    protected 
// Returns:   void
// Qualifier:
// Parameter: UPrimitiveComponent * overlappedComponent
// Parameter: AActor * otherActor
// Parameter: UPrimitiveComponent * otherComp
// Parameter: int32 otherBodyIndex
// Parameter: bool fromSweep
// Parameter: const FHitResult & sweepResult
//************************************
void ADropPod::OnPodOverlap_Implementation(UPrimitiveComponent* overlappedComponent, AActor* otherActor, UPrimitiveComponent* otherComp, int32 otherBodyIndex, bool fromSweep, const FHitResult& sweepResult)
{
	if (!mMadeImpact)
	{
		if (otherActor)
		{
			FHitResult hit;
			FCollisionQueryParams params;
			const bool didHit = GetWorld()->SweepSingleByObjectType(hit, GetActorLocation(), otherActor->GetActorLocation(), FQuat::Identity, ECC_Pawn, FCollisionShape::MakeSphere(20.f), params);

			if (didHit)
			{
				FVertPointDamageEvent PointDmg;
				PointDmg.DamageTypeClass = ImpactDamageType;
				PointDmg.HitInfo = hit;
				PointDmg.ShotDirection = ((-hit.ImpactNormal) + FVector::UpVector).GetSafeNormal();
				PointDmg.Damage = Damage;
				PointDmg.Knockback = Knockback;
				PointDmg.KnockbackScaling = KnockbackScaling;

				otherActor->TakeDamage(PointDmg.Damage, PointDmg, OwningController, this);
			}
		}
	}
}