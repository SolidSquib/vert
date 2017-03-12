// Fill out your copyright notice in the Description page of Project Settings.

#include "Vert.h"
#include "GrappleHook.h"

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
		if (AVertCharacter* character = launcher->GetOwningCharacter())
		{
			character->RegisterGrappleHook(this);
		}
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

void AGrappleHook::Activate()
{
	Sprite->SetVisibility(true);
	SphereCollider->Activate();
}

void AGrappleHook::Deactivate()
{
	Sprite->SetVisibility(false);
	SphereCollider->Deactivate();
}

void AGrappleHook::Launch(const FVector& fireDirection)
{
	if (AGrappleLauncher* launcher = GetOwnerAsGrappleLauncher())
	{
		mGrappleState = EGrappleState::Launching;

		DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);

		Activate();
		ProjectileMovement->Velocity = fireDirection * GetOwnerAsGrappleLauncher()->GrappleConfig.LineSpeed;

		OnFired.Broadcast();
	}
}

void AGrappleHook::Reel()
{
	if (mGrappleState != EGrappleState::Sheathed)
	{
		mGrappleState = EGrappleState::Reeling;
	}
}

void AGrappleHook::Sheathe()
{
	mGrappleState = EGrappleState::Sheathed;

	if (AGrappleLauncher* launcher = GetOwnerAsGrappleLauncher())
	{
		ProjectileMovement->Velocity = FVector::ZeroVector;
		AttachToActor(launcher, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
		SetActorRelativeLocation(FVector::ZeroVector);
	}

	OnReturned.Broadcast();
	Deactivate();
}

void AGrappleHook::Hook(AActor* OtherActor, UPrimitiveComponent* OtherComp, const FHitResult& hit)
{
	mGrappleState = EGrappleState::Hooked;

	ProjectileMovement->Velocity = FVector::ZeroVector;

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

			character->LaunchCharacter(direction * (launcher->GrappleConfig.ReelSpeed/**GetWorld()->GetDeltaSeconds()*/), true, true);

			if (mHookAttachment.Actor.IsValid() && mHookAttachment.Component.IsValid())
			{
				mHookAttachment.Component->AddForceAtLocation(launcher->GrappleConfig.ReelSpeed * -direction, GetActorLocation());
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
		// #TODO_MI: check the impact surface to see if we should hook
		Hook(OtherActor, OtherComp, Hit);

		OnHooked.Broadcast();

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
