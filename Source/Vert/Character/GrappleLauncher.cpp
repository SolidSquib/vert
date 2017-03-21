// Fill out your copyright notice in the Description page of Project Settings.

#include "Vert.h"
#include "GrappleLauncher.h"

// Sets default values for this component's properties
AGrappleLauncher::AGrappleLauncher()
	: ShowDebug(true)
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	Cable = CreateDefaultSubobject<UCableComponent>(TEXT("GrappleCable"));
	Cable->CableLength = 0.0;
	Cable->NumSegments = 5;
	Cable->SolverIterations = 2;

	SetRootComponent(Cable);

	HookClass = nullptr;
}

// Called when the game starts
void AGrappleLauncher::BeginPlay()
{
	Super::BeginPlay();
	SetActorTickEnabled(true);

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

	if (mGrappleHook.IsValid())
	{
		FScriptDelegate onHookedDelegate;
		onHookedDelegate.BindUFunction(this, TEXT("OnHooked"));
		mGrappleHook->OnHooked.Add(onHookedDelegate);
	}
}


// Called every frame
void AGrappleLauncher::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
}


void AGrappleLauncher::FireGrapple(const FVector& fireDirection, bool wasGamepadTriggered /*= false*/)
{
	if (mGrappleHook.IsValid() && mGrappleHook->GetGrappleState() == EGrappleState::Sheathed)
	{
		if (GetOwningCharacter()->GetRemainingGrapples() <= 0 && !GetOwningCharacter()->ShowDebug.InfiniteDashGrapple)
			return;

		if (mGrappleHook.IsValid())
		{
			mGrappleHook->Launch(fireDirection);
		}

		mGrappleHook->Activate();
		Cable->SetVisibility(true);

		GetOwningCharacter()->DecrementRemainingGrapples();
	}
	else if(!wasGamepadTriggered)
	{
		ResetGrapple();
	}
}

void AGrappleLauncher::FireGrapple(const FVector2D& shootDirection, bool wasGamepadTriggered /*= false*/)
{
	FVector direction(shootDirection.X, 0.f, shootDirection.Y);
	FireGrapple(direction, wasGamepadTriggered);
}

void AGrappleLauncher::ResetGrapple()
{
	if (mGrappleHook.IsValid() && mGrappleHook->GetGrappleState() == EGrappleState::Hooked)
	{
		mGrappleHook->UnHook();
	}
	else if (mGrappleHook.IsValid() && mGrappleHook->GetGrappleState() == EGrappleState::Latched)
	{
		mGrappleHook->Sheathe();
	}
}

AVertCharacter* AGrappleLauncher::GetOwningCharacter() const
{
	if (GetOwner())
	{
		if (AVertCharacter* character = Cast<AVertCharacter>(GetOwner()))
		{
			return character;
		}
	}

	return nullptr;
}

void AGrappleLauncher::OnHooked_Implementation()
{
}

void AGrappleLauncher::ReelIn()
{
	AVertCharacter* character = GetOwningCharacter();
	FVector direction = (mGrappleHook->GetActorLocation() - GetActorLocation()).GetSafeNormal();

	FVector launchVelocity = direction * (GrappleConfig.ReelSpeed);
	FVector launchVelocity2D = FVector(launchVelocity.X, 0.0f, launchVelocity.Z);

	character->LaunchCharacter(launchVelocity2D, true, true);
}

bool AGrappleLauncher::MoveGrappleLine()
{
	// 	if (mGrappleHook.IsValid())
	// 	{
	// 		if (IsGrappleLineOutOfRope())
	// 		{
	// 			GrappleConfig.HasTarget = false;
	// 		}
	// 
	// 		mGrappleHook->SetActorLocation(FMath::VInterpTo(mGrappleHook->GetActorLocation(), GrappleConfig.TargetLocation, GetWorld()->GetDeltaSeconds(), GrappleConfig.LineSpeed*0.5));
	// 	}
	// 	
	return false;
}

void AGrappleLauncher::ReturnGrappleToLauncher()
{
	mGrappleHook->SetActorLocation(FMath::VInterpTo(mGrappleHook->GetActorLocation(), GetActorLocation(), GetWorld()->GetDeltaSeconds(), GrappleConfig.LineSpeed));
	if ((mGrappleHook->GetActorLocation() - GetActorLocation()).SizeSquared() <= 2)
	{
		mGrappleHook->Deactivate();
		Cable->SetVisibility(false);
	}
}

bool AGrappleLauncher::IsGrappleLineOutOfRope()
{
	if ((GetActorLocation() - mGrappleHook->GetActorLocation()).SizeSquared() >= (GrappleConfig.MaxLength * GrappleConfig.MaxLength))
	{
		return true;
	}
	return false;
}