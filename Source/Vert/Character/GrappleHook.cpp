// Fill out your copyright notice in the Description page of Project Settings.

#include "Vert.h"
#include "GrappleHook.h"

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

	Sheathe();

	if (AGrappleLauncher* launcher = GetOwnerAsGrappleLauncher())
	{
		if (UGrapplingComponent* ownerComponent = launcher->GetOwner() ? Cast<UGrapplingComponent>(launcher->GetOwner()->GetComponentByClass(UGrapplingComponent::StaticClass())) : nullptr)
		{
			ownerComponent->RegisterGrappleHookDelegates(this);
		}
		else
			UE_LOG(LogGrappleHook, Warning, TEXT("GrappleHook [%s] has no associated UGrapplingComponent, undesirable behaviour might occur."), *GetName());
	}
}

// Called every frame
void AGrappleHook::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	switch (mGrappleState)
	{
	case EGrappleState::Reeling:
		if (AGrappleLauncher* launcher = GetOwnerAsGrappleLauncher())
		{
			FVector diff = launcher->GetActorLocation() - GetActorLocation();
			FVector direction = diff.GetSafeNormal();
			float distance = DeltaTime * launcher->GrappleConfig.ReelSpeed;
			float sizeSquared = diff.SizeSquared();
			if (FMath::Square(distance) >= sizeSquared)
			{
				Sheathe();
			}
			else
			{
				ProjectileMovement->Velocity = direction * launcher->GrappleConfig.ReelSpeed;
			}
		}
		break;

	case EGrappleState::Launching:
		if (AGrappleLauncher* launcher = GetOwnerAsGrappleLauncher())
		{
			FVector diff = launcher->GetActorLocation() - GetActorLocation();
			if (diff.SizeSquared() >= FMath::Square(launcher->GrappleConfig.MaxLength))
			{
				Reel();
			}
		}
		break;

	case EGrappleState::Hooked:
		Pull();
		break;
	}
}

void AGrappleHook::ActivateHookCollision()
{
	SphereCollider->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
	SphereCollider->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
	SphereCollider->SetCollisionResponseToChannel(ECC_PhysicsBody, ECR_Block);
	SphereCollider->SetCollisionResponseToChannel(ECC_Vehicle, ECR_Block);
	SphereCollider->SetCollisionResponseToChannel(ECC_Destructible, ECR_Block);
	SphereCollider->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
}

void AGrappleHook::DeactivateHookCollision()
{
	SphereCollider->SetCollisionResponseToAllChannels(ECR_Ignore);
}

void AGrappleHook::Launch(const FVector& fireDirection)
{
	if (AGrappleLauncher* launcher = GetOwnerAsGrappleLauncher())
	{
		mGrappleState = EGrappleState::Launching;

		DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);

		ActivateHookCollision();
		ProjectileMovement->Velocity = fireDirection * GetOwnerAsGrappleLauncher()->GrappleConfig.LineSpeed;

		OnFired.Broadcast();
	}
}

void AGrappleHook::Reel()
{
	if (mGrappleState != EGrappleState::Sheathed)
	{
		mGrappleState = EGrappleState::Reeling;
		DeactivateHookCollision();
	}
}

void AGrappleHook::Sheathe()
{
	if (mGrappleState == EGrappleState::Latched)
	{
		OnUnLatched.Broadcast(this);

		if (!ProjectileMovement->UpdatedComponent)
			ProjectileMovement->SetUpdatedComponent(RootComponent);
	}

	mGrappleState = EGrappleState::Sheathed;

	if (AGrappleLauncher* launcher = GetOwnerAsGrappleLauncher())
	{
		ProjectileMovement->Velocity = FVector::ZeroVector;
		AttachToActor(launcher, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
		SetActorRelativeLocation(FVector::ZeroVector);
	}

	OnReturned.Broadcast();
}

void AGrappleHook::Hook(AActor* OtherActor, UPrimitiveComponent* OtherComp, const FHitResult& hit, bool attachToTarget /*= true*/)
{
	mGrappleState = EGrappleState::Hooked;

	ProjectileMovement->Velocity = FVector::ZeroVector;

	if(attachToTarget)
		AttachToActor(OtherActor, FAttachmentTransformRules::KeepWorldTransform);

	mHookAttachment.Actor = OtherActor;
	mHookAttachment.Component = OtherComp;

	// # TODO_MI: Check the type of object hooked (item, weapon, static, dynamic etc.)
	// OtherComp->GetCollisionProfileName()
}

void AGrappleHook::UnHook()
{
	if (mGrappleState == EGrappleState::Hooked)
	{
		if (mHookAttachment.Actor.IsValid())
		{
			DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
		}

		mHookAttachment.Actor = nullptr;
		mHookAttachment.Component = nullptr;

		SphereCollider->Activate();

		if(!ProjectileMovement->UpdatedComponent)
			ProjectileMovement->SetUpdatedComponent(RootComponent);

		Reel();
	}
}

void AGrappleHook::Pull()
{
	if (AGrappleLauncher* launcher = GetOwnerAsGrappleLauncher())
	{
		if (AVertCharacter* character = launcher->GetOwningCharacter())
		{
			FVector diff = GetActorLocation() - launcher->GetActorLocation();
			diff.Y = 0;
			FVector direction = (diff * 100).GetSafeNormal();

			OnPull.Broadcast(direction, launcher->GrappleConfig.ReelSpeed);
			character->LaunchCharacter(direction * (launcher->GrappleConfig.ReelSpeed/**GetWorld()->GetDeltaSeconds()*/), true, true);

			if (mHookAttachment.Actor.IsValid() && mHookAttachment.Component.IsValid())
			{
				if (mHookAttachment.Component->Mobility == EComponentMobility::Movable)
				{
					if (mHookAttachment.Component->IsSimulatingPhysics())
					{
						mHookAttachment.Component->AddForceAtLocation(launcher->GrappleConfig.ReelSpeed * -direction, GetActorLocation());
					}					
				}					
			}
		}
	}
}

AGrappleLauncher* AGrappleHook::GetOwnerAsGrappleLauncher() const
{
	if (AActor* owner = GetOwner())
	{
		if (AGrappleLauncher* launcher = Cast<AGrappleLauncher>(owner))
		{
			return launcher;
		}
	}

	return nullptr;
}

FVector AGrappleHook::GetHookVelocity() const
{
	return ProjectileMovement->Velocity;
}

bool AGrappleHook::GetProjectileMovementComponentIsActive() const
{
	bool inactive = ProjectileMovement->HasStoppedSimulation();
	return !inactive;
}

void AGrappleHook::OnHit(class UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (OtherActor && mGrappleState == EGrappleState::Launching)
	{
		if (AGrappleLauncher* launcher = GetOwnerAsGrappleLauncher())
		{
			FName collisionProfile = OtherComp->GetCollisionProfileName();
			UE_LOG(LogTemp, Warning, TEXT("Collision profile: %s"), *collisionProfile.ToString());

			if (OtherComp && launcher->GrappleConfig.GrapplePointCollisionProfileName == collisionProfile)
			{
				if (AGrapplePoint* point = Cast<AGrapplePoint>(OtherActor))
				{
					point->AttachGrappleHook(this);
					Hook(OtherActor, OtherComp, Hit, false);
					OnHooked.Broadcast();
				}
			}
			else
			{
				if (launcher->GrappleConfig.HookGrapplePointsOnly)
				{
					Reel();
				}
				else
				{
					Hook(OtherActor, OtherComp, Hit);
					OnHooked.Broadcast();
				}
			}
		}

		//Play VFX.
		if (ImpactParticleTemplate != NULL)
		{
			UGameplayStatics::SpawnEmitterAtLocation(this, ImpactParticleTemplate, Hit.Location, Hit.Normal.Rotation());
		}

		//Play SFX.
		if (ImpactSound != NULL)
		{
			UGameplayStatics::PlaySoundAtLocation(this, ImpactSound, Hit.Location);
		}

		SphereCollider->Deactivate();
	}
}

void AGrappleHook::OnBeginOverlap(UPrimitiveComponent* overlappedComp, AActor* otherActor, UPrimitiveComponent* otherComp, int32 otherBodyIndex, bool fromSweep, const FHitResult& sweepResult)
{
	if (mGrappleState == EGrappleState::Hooked)
	{
		if (AVertCharacter* character = Cast<AVertCharacter>(otherActor))
		{
			if (AGrappleLauncher* launcher = GetOwnerAsGrappleLauncher())
			{
				if (character == launcher->GetOwningCharacter())
				{
					mGrappleState = EGrappleState::Latched;
					OnLatched.Broadcast(this);
				}
			}
		}		
	}
}