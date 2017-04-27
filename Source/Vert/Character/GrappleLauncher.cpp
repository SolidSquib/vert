// Fill out your copyright notice in the Description page of Project Settings.

#include "Vert.h"
#include "GrappleLauncher.h"

//************************************
// Class:    AGrappleLauncher
//************************************

DECLARE_LOG_CATEGORY_CLASS(LogGrappleLauncher, Log, All);

// Sets default values for this component's properties
AGrappleLauncher::AGrappleLauncher()
{
	PrimaryActorTick.bCanEverTick = false;
	PrimaryActorTick.bStartWithTickEnabled = false;

	Cable = CreateDefaultSubobject<UCableComponent>(TEXT("GrappleCable"));
	Cable->CableLength = 0.0;
	Cable->NumSegments = 5;
	Cable->SolverIterations = 2;

	SetRootComponent(Cable);

	HookClass = nullptr;
}

void AGrappleLauncher::BeginPlay()
{
	Super::BeginPlay();

	if (GetOwner())
	{
		if (AVertCharacter* character = Cast<AVertCharacter>(GetOwner()))
		{
			mCharacterOwner = character;

			if (mCharacterOwner->GetGrapplingComponent())
			{
				mGrapplingComponent = mCharacterOwner->GetGrapplingComponent();
			}
			else { UE_LOG(LogGrappleLauncher, Error, TEXT("Unable to find a UGrapplingComponent to associate to GrappleLauncher [%s]"), *GetName()); }
		}
		else { UE_LOG(LogGrappleLauncher, Error, TEXT("Unable to find AVertCharacter owner of GrappleLauncher [%s]"), *GetName()); }
	}

	// Attempt to create the hook
	if (HookClass != nullptr)
	{
		if (UWorld* world = GetWorld())
		{
			//Setup spawn parameters for the actor.
			FActorSpawnParameters spawnParameters;
			//spawnParameters.Name = TEXT("GrappleHook");
			spawnParameters.Owner = this;
			spawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

			//Spawn the actor.
			AGrappleHook* spawnedHook = world->SpawnActor<AGrappleHook>(HookClass, spawnParameters);

			//Assert that the actor exists.
			check(spawnedHook);

			if (spawnedHook)
			{
				mGrappleHook = spawnedHook;
				Cable->SetAttachEndTo(mGrappleHook.Get(), mGrappleHook->GetSprite()->GetFName());
			}
		}
	}
	else { UE_LOG(LogGrappleLauncher, Error, TEXT("Unable to spawn AGrappleHook to associate with [%s]."), *GetName()); }
}

//************************************
// Method:    FireGrapple
// FullName:  AGrappleLauncher::FireGrapple
// Access:    public 
// Returns:   bool
// Qualifier:
// Parameter: const FVector & fireDirection
// Parameter: bool wasGamepadTriggered
//************************************
bool AGrappleLauncher::FireGrapple(const FVector& fireDirection, bool wasGamepadTriggered /*= false*/)
{
	if (mGrappleHook.IsValid() && mGrappleHook->GetGrappleState() == EGrappleState::HookSheathed)
	{
		if (mGrappleHook.IsValid())
		{
			mGrappleHook->Launch(fireDirection);
		}

		Cable->SetVisibility(true);

		return true;
	}
	else if(!wasGamepadTriggered)
	{
		ResetGrapple();
	}

	return false;
}

//************************************
// Method:    FireGrapple
// FullName:  AGrappleLauncher::FireGrapple
// Access:    public 
// Returns:   bool
// Qualifier:
// Parameter: const FVector2D & shootDirection
// Parameter: bool wasGamepadTriggered
//************************************
bool AGrappleLauncher::FireGrapple(const FVector2D& shootDirection, bool wasGamepadTriggered /*= false*/)
{
	FVector direction(shootDirection.X, 0.f, shootDirection.Y);
	return FireGrapple(direction, wasGamepadTriggered);
}

//************************************
// Method:    StartPulling
// FullName:  AGrappleLauncher::StartPulling
// Access:    public 
// Returns:   bool
// Qualifier: const
//************************************
bool AGrappleLauncher::StartPulling() const
{
	return mGrappleHook.IsValid() ? mGrappleHook->StartPulling() : false;
}

//************************************
// Method:    ResetGrapple
// FullName:  AGrappleLauncher::ResetGrapple
// Access:    public 
// Returns:   void
// Qualifier:
//************************************
void AGrappleLauncher::ResetGrapple()
{
	if(mGrappleHook.IsValid())
		mGrappleHook->ResetHook();
}

AVertCharacter* AGrappleLauncher::GetCharacterOwner() const
{
	return mCharacterOwner.IsValid() ? mCharacterOwner.Get() : nullptr;
}

//************************************
// Class:    AGrappleHook
//************************************

DECLARE_LOG_CATEGORY_CLASS(LogGrappleHook, Log, All);

// Sets default values
AGrappleHook::AGrappleHook()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	// Use a sphere as a simple collision representation
	SphereCollider = CreateDefaultSubobject<USphereComponent>(TEXT("SphereComp"));
	SphereCollider->InitSphereRadius(5.0f);
	SphereCollider->BodyInstance.SetCollisionProfileName("GrappleHook");
	SphereCollider->OnComponentHit.AddDynamic(this, &AGrappleHook::OnHit);
	SphereCollider->OnComponentBeginOverlap.AddDynamic(this, &AGrappleHook::OnBeginOverlap);
	SphereCollider->SetCollisionObjectType(ECC_Grappler);
	SphereCollider->SetCollisionResponseToChannel(ECC_Grappler, ECR_Ignore);
	SphereCollider->SetWalkableSlopeOverride(FWalkableSlopeOverride(WalkableSlope_Unwalkable, 0.f));
	SphereCollider->CanCharacterStepUpOn = ECB_No;

	RootComponent = SphereCollider;

	Sprite = CreateDefaultSubobject<UPaperFlipbookComponent>(TEXT("Sprite"));
	Sprite->SetupAttachment(SphereCollider);

	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->UpdatedComponent = SphereCollider;
	ProjectileMovement->InitialSpeed = 0.f;
	ProjectileMovement->MaxSpeed = 3000.f;
	ProjectileMovement->bRotationFollowsVelocity = false;
	ProjectileMovement->bShouldBounce = true;
}

// Called when the game starts or when spawned
void AGrappleHook::BeginPlay()
{
	Super::BeginPlay();

	if (AGrappleLauncher* launcher = Cast<AGrappleLauncher>(GetOwner()))
	{
		mLauncherOwner = launcher;

		if (AVertCharacter* character = launcher->GetCharacterOwner())
		{
			mCharacterOwner = character;
		}
		else { UE_LOG(LogGrappleHook, Error, TEXT("GrappleHook [%s] has no associated AVertCharacter."), *GetName()); }

		if (UGrapplingComponent* ownerComponent = launcher->GetOwner() ? Cast<UGrapplingComponent>(launcher->GetOwner()->GetComponentByClass(UGrapplingComponent::StaticClass())) : nullptr)
		{
			mGrapplingComponent = ownerComponent;
			mGrapplingComponent->RegisterGrappleHookDelegates(this);
		}
		else { UE_LOG(LogGrappleHook, Error, TEXT("GrappleHook [%s] has no associated UGrapplingComponent."), *GetName()); }
	}
	else { UE_LOG(LogGrappleHook, Error, TEXT("GrappleHook [%s] has no associated AGrappleLauncher."), *GetName()); }

	SheatheHook();
}

// Called every frame
void AGrappleHook::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	switch (mGrappleState)
	{
	case EGrappleState::HookReturning:
		TickReel(DeltaTime);
		break;

	case EGrappleState::HookLaunching:
		if (IsLineDepleted(DeltaTime))
			StartReeling();
		break;

	case EGrappleState::HookDeployedAndReturning:
		TickHookDeployedAndReturning(DeltaTime);
		break;

	case EGrappleState::HookDeployed:
		TickHookDeployed(DeltaTime);
		break;
	}
}

//************************************
// Method:    ActivateHookCollision
// FullName:  AGrappleHook::ActivateHookCollision
// Access:    private 
// Returns:   void
// Qualifier:
//************************************
void AGrappleHook::ActivateHookCollision()
{
	SphereCollider->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
	SphereCollider->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
	SphereCollider->SetCollisionResponseToChannel(ECC_PhysicsBody, ECR_Block);
	SphereCollider->SetCollisionResponseToChannel(ECC_Vehicle, ECR_Block);
	SphereCollider->SetCollisionResponseToChannel(ECC_Destructible, ECR_Block);
	SphereCollider->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
}

//************************************
// Method:    DeactivateHookCollision
// FullName:  AGrappleHook::DeactivateHookCollision
// Access:    private 
// Returns:   void
// Qualifier:
//************************************
void AGrappleHook::DeactivateHookCollision()
{
	SphereCollider->SetCollisionResponseToAllChannels(ECR_Ignore);
}

//************************************
// Method:    Launch
// FullName:  AGrappleHook::Launch
// Access:    public 
// Returns:   void
// Qualifier:
// Parameter: const FVector & fireDirection
//************************************
void AGrappleHook::Launch(const FVector& fireDirection)
{
	if (mGrapplingComponent.IsValid())
	{
		mGrappleState = EGrappleState::HookLaunching;

		DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);

		ActivateHookCollision();
		ProjectileMovement->Velocity = fireDirection * mGrapplingComponent->LaunchSpeed;

		if(mLauncherOwner.IsValid())
			mLauncherOwner->Cable->SetVisibility(true);

		OnFired.Broadcast();
	}
	else { UE_LOG(LogGrappleHook, Error, TEXT("UGrapplingComponent associated with [%s] is invalid."), *GetName()); }
}

//************************************
// Method:    StartPulling
// FullName:  AGrappleHook::StartPulling
// Access:    public 
// Returns:   bool
// Qualifier:
//************************************
bool AGrappleHook::StartPulling()
{
	if (mGrappleState == EGrappleState::HookDeployed)
	{
		mGrappleState = EGrappleState::HookDeployedAndReturning;
		return true;
	} 
	UE_LOG(LogGrappleLauncher, Warning, TEXT("Could not start pulling, not currently hooked."));

	return false;
}

//************************************
// Method:    IsLineDepleted
// FullName:  AGrappleHook::IsLineDepleted
// Access:    private 
// Returns:   bool
// Qualifier:
// Parameter: float DeltaSeconds
//************************************
bool AGrappleHook::IsLineDepleted(float DeltaSeconds)
{
	if (mLauncherOwner.IsValid() && mGrapplingComponent.IsValid())
	{
		FVector diff = mLauncherOwner->GetActorLocation() - GetActorLocation();
		return diff.SizeSquared() >= FMath::Square(mGrapplingComponent->MaxLineLength);
	}
	UE_LOG(LogGrappleHook, Error, TEXT("Unable to determine size of line, returning default true."));

	return true;
}

//************************************
// Method:    ResetHook
// FullName:  AGrappleHook::ResetHook
// Access:    public 
// Returns:   void
// Qualifier:
//************************************
void AGrappleHook::ResetHook()
{
	if (mGrappleState == EGrappleState::HookDeployed || mGrappleState == EGrappleState::HookDeployedAndReturning)
		DetatchHook();
	else 
		SheatheHook();
}

//************************************
// Method:    StartReeling
// FullName:  AGrappleHook::StartReeling
// Access:    private 
// Returns:   void
// Qualifier:
//************************************
void AGrappleHook::StartReeling()
{
	if (mGrappleState != EGrappleState::HookReturning && mGrappleState != EGrappleState::HookSheathed)
	{
		mGrappleState = EGrappleState::HookReturning;
		DeactivateHookCollision();
	}
}

//************************************
// Method:    TickReel
// FullName:  AGrappleHook::TickReel
// Access:    private 
// Returns:   void
// Qualifier:
// Parameter: float DeltaSeconds
//************************************
void AGrappleHook::TickReel(float DeltaSeconds)
{	
	if (mGrapplingComponent.IsValid() && mLauncherOwner.IsValid())
	{
		FVector diff = mLauncherOwner->GetActorLocation() - GetActorLocation();
		FVector direction = diff.GetSafeNormal();
		float distance = DeltaSeconds * mGrapplingComponent->ReelSpeed;
		float sizeSquared = diff.SizeSquared();

		if (FMath::Square(distance) >= sizeSquared)
			SheatheHook();
		else
			ProjectileMovement->Velocity = direction * mGrapplingComponent->ReelSpeed;
	}
}

//************************************
// Method:    TickHookDeployed
// FullName:  AGrappleHook::TickHookDeployed
// Access:    private 
// Returns:   void
// Qualifier:
// Parameter: float DeltaSeconds
//************************************
void AGrappleHook::TickHookDeployed(float DeltaSeconds)
{
	if (mCharacterOwner.IsValid() && mGrapplingComponent.IsValid())
	{
		FVector difference = mCharacterOwner->GetActorLocation() - GetActorLocation();
		FVector direction = difference.GetSafeNormal();
		float actualLength = difference.Size();
		mLauncherOwner->Cable->CableLength = mDistanceFromLauncher;

		if ((mGrapplingComponent->StringContraint && actualLength > mDistanceFromLauncher) || !mGrapplingComponent->StringContraint)
		{
			FVector dampingVelocity = mGrapplingComponent->LineDampingCoefficient * mCharacterOwner->GetVelocity();
			FVector springVelocity = mGrapplingComponent->LineSpringCoefficient * (direction*(mDistanceFromLauncher - actualLength));
			FVector launchVelocity = springVelocity - dampingVelocity;

			mCharacterOwner->LaunchCharacter(launchVelocity, false, false);
		}
	} else { UE_LOG(LogGrappleHook, Warning, TEXT("Cannot pull character associated with [%s] because it is invalid."), *GetName()); }
}

//************************************
// Method:    TickHookDeployedAndReturning
// FullName:  AGrappleHook::TickHookDeployedAndReturning
// Access:    private 
// Returns:   void
// Qualifier:
// Parameter: float DeltaSeconds
//************************************
void AGrappleHook::TickHookDeployedAndReturning(float DeltaSeconds)
{
	if (mGrapplingComponent.IsValid() && mGrapplingComponent->UseSimpleGrapple)
	{
		FVector diff = GetActorLocation() - mLauncherOwner->GetActorLocation();
		FVector direction = diff.GetSafeNormal();

		float distance = DeltaSeconds * mGrapplingComponent->PullSpeed;
		FVector newLocation = diff - (distance * direction);

		if (newLocation.SizeSquared() <= FMath::Square(mGrapplingComponent->LineCutLength))
			ResetHook();
		else
			mCharacterOwner->LaunchCharacter(mGrapplingComponent->PullSpeed * direction, true, true);
	}
	else if (mGrapplingComponent.IsValid())
	{
		float distance = DeltaSeconds * mGrapplingComponent->PullSpeed;
		float newLength = mDistanceFromLauncher - distance;

		if (newLength <= mGrapplingComponent->LineCutLength)
			ResetHook();
		else
		{
			mDistanceFromLauncher -= (DeltaSeconds * mGrapplingComponent->PullSpeed);
			TickHookDeployed(DeltaSeconds);
		}			
	}
}

//************************************
// Method:    SheatheHook
// FullName:  AGrappleHook::SheatheHook
// Access:    private 
// Returns:   void
// Qualifier:
//************************************
void AGrappleHook::SheatheHook()
{
	mGrappleState = EGrappleState::HookSheathed;

	if (mLauncherOwner.IsValid())
	{
		ProjectileMovement->Velocity = FVector::ZeroVector;
		AttachToActor(mLauncherOwner.Get(), FAttachmentTransformRules::SnapToTargetNotIncludingScale);
		SetActorRelativeLocation(FVector::ZeroVector);
		
		mLauncherOwner->Cable->CableLength = 0.f;
		mLauncherOwner->Cable->SetVisibility(false);
	}

	mDistanceFromLauncher = 0.f;

	OnReturned.Broadcast();
}

//************************************
// Method:    DeployHook
// FullName:  AGrappleHook::DeployHook
// Access:    private 
// Returns:   void
// Qualifier:
// Parameter: AActor * OtherActor
// Parameter: UPrimitiveComponent * OtherComp
// Parameter: const FHitResult & hit
// Parameter: bool attachToTarget
//************************************
void AGrappleHook::DeployHook(AActor* OtherActor, UPrimitiveComponent* OtherComp, const FHitResult& hit, bool attachToTarget /*= true*/)
{
	mGrappleState = EGrappleState::HookDeployed;

	ProjectileMovement->Velocity = FVector::ZeroVector;

	if (attachToTarget)
	{
		AttachToActor(OtherActor, FAttachmentTransformRules::KeepWorldTransform);
		UE_LOG(LogGrappleHook, Log, TEXT("Hook [%s] attached to world object [%s]."), *GetName(), *OtherActor->GetName());
	}

	mHookAttachment.Actor = OtherActor;
	mHookAttachment.Component = OtherComp;

	if (mLauncherOwner.IsValid())
	{
		if (AVertCharacter* character = mLauncherOwner->GetCharacterOwner())
		{
			FVector diff = character->GetActorLocation() - GetActorLocation();
			mDistanceFromLauncher = diff.Size();
		}
	}

	OnHooked.Broadcast();
}

//************************************
// Method:    DetatchHook
// FullName:  AGrappleHook::DetatchHook
// Access:    private 
// Returns:   void
// Qualifier:
//************************************
void AGrappleHook::DetatchHook()
{
	if (mGrappleState == EGrappleState::HookDeployed || mGrappleState == EGrappleState::HookDeployedAndReturning)
	{
		if (mHookAttachment.Actor.IsValid())
		{
			DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
		}

		mHookAttachment.Actor = nullptr;
		mHookAttachment.Component = nullptr;

		SphereCollider->Activate();

		if (!ProjectileMovement->UpdatedComponent)
			ProjectileMovement->SetUpdatedComponent(RootComponent);

		OnHookReleased.Broadcast();
		StartReeling();
	}
}

void AGrappleHook::OnHit_Implementation(class UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (OtherActor && mGrappleState == EGrappleState::HookLaunching)
	{
		if (mGrapplingComponent.IsValid())
			DeployHook(OtherActor, OtherComp, Hit);

		//Play VFX.
		if (ImpactParticleTemplate != NULL)
			UGameplayStatics::SpawnEmitterAtLocation(this, ImpactParticleTemplate, Hit.Location, Hit.Normal.Rotation());

		//Play SFX.
		if (ImpactSound != NULL)
			UGameplayStatics::PlaySoundAtLocation(this, ImpactSound, Hit.Location);

		SphereCollider->Deactivate();
	}
}

void AGrappleHook::OnBeginOverlap_Implementation(UPrimitiveComponent* overlappedComp, AActor* otherActor, UPrimitiveComponent* otherComp, int32 otherBodyIndex, bool fromSweep, const FHitResult& sweepResult)
{
	// Possibly used for pulling players / weapons
}