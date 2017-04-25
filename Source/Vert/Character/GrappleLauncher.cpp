// Fill out your copyright notice in the Description page of Project Settings.

#include "Vert.h"
#include "GrappleLauncher.h"

DECLARE_LOG_CATEGORY_CLASS(LogGrappleLauncher, Log, All);

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

	// Bind to the hook's events if it was successfully created
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


bool AGrappleLauncher::FireGrapple(const FVector& fireDirection, bool wasGamepadTriggered /*= false*/)
{
	if (mGrappleHook.IsValid() && mGrappleHook->GetGrappleState() == EGrappleState::Sheathed)
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

bool AGrappleLauncher::FireGrapple(const FVector2D& shootDirection, bool wasGamepadTriggered /*= false*/)
{
	FVector direction(shootDirection.X, 0.f, shootDirection.Y);
	return FireGrapple(direction, wasGamepadTriggered);
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

AVertCharacter* AGrappleLauncher::GetCharacterOwner() const
{
	return mCharacterOwner.IsValid() ? mCharacterOwner.Get() : nullptr;
}

void AGrappleLauncher::OnHooked_Implementation()
{
}