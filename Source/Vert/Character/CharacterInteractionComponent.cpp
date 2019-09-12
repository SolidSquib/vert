// Copyright Inside Out Games Ltd. 2017

#include "CharacterInteractionComponent.h"
#include "VertCharacter.h"
#include "Components/CapsuleComponent.h"
#include "DrawDebugHelpers.h"
#include "AkGameplayStatics.h"

DECLARE_LOG_CATEGORY_CLASS(LogCharacterInteractionComponent, Log, All);

// Sets default values for this component's properties
UCharacterInteractionComponent::UCharacterInteractionComponent()
{
	if(ShowDebug)
		PrimaryComponentTick.bCanEverTick = true;
	else
		PrimaryComponentTick.bCanEverTick = false;
}


// Called when the game starts
//************************************
// Method:    BeginPlay
// FullName:  UCharacterInteractionComponent::BeginPlay
// Access:    virtual protected 
// Returns:   void
// Qualifier:
//************************************
void UCharacterInteractionComponent::BeginPlay()
{
	Super::BeginPlay();

	AActor* parent = GetOwner();
	if (AVertCharacter* character = Cast<AVertCharacter>(parent))
	{
		mCharacterOwner = character;
	}
	else
	{
		UE_LOG(LogCharacterInteractionComponent, Error, TEXT("Unable to find parent AVertCharacter"));
	}

	// Attempt to create the hook
	if (mCharacterOwner.IsValid() && DefaultWeaponClass != nullptr)
	{
		if (UWorld* world = GetWorld())
		{
			//Setup spawn parameters for the actor.
			FActorSpawnParameters spawnParameters;
			spawnParameters.Owner = GetOwner();
			spawnParameters.Instigator = mCharacterOwner.Get();
			spawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

			//Spawn the actor.
			ABaseWeapon* spawnedWeapon = world->SpawnActor<ABaseWeapon>(DefaultWeaponClass, spawnParameters);

			//Assert that the actor exists.
			check(spawnedWeapon);

			if (spawnedWeapon)
			{
				mDefaultWeapon = spawnedWeapon;
				mDefaultWeapon->Pickup(mCharacterOwner.Get());
				mDefaultWeapon->StartEquipping();
			}
		}
	}
	else { UE_LOG(LogCharacterInteractionComponent, Error, TEXT("[%s] was unable to spawn default weapon."), *GetName()); }
}

//************************************
// Method:    EndPlay
// FullName:  UCharacterInteractionComponent::EndPlay
// Access:    virtual protected 
// Returns:   void
// Qualifier:
// Parameter: const EEndPlayReason::Type endPlayReason
//************************************
void UCharacterInteractionComponent::EndPlay(const EEndPlayReason::Type endPlayReason)
{
	if (endPlayReason == EEndPlayReason::Destroyed && mDefaultWeapon.IsValid())
	{
		mDefaultWeapon->Destroy();
		mDefaultWeapon = nullptr;
	}
}

// Called every frame
//************************************
// Method:    TickComponent
// FullName:  UCharacterInteractionComponent::TickComponent
// Access:    virtual public 
// Returns:   void
// Qualifier:
// Parameter: float DeltaTime
// Parameter: ELevelTick TickType
// Parameter: FActorComponentTickFunction * ThisTickFunction
//************************************
void UCharacterInteractionComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (ShowDebug)
		DrawDebug();
}

//************************************
// Method:    AttemptInteract
// FullName:  UCharacterInteractionComponent::AttemptInteract
// Access:    public 
// Returns:   AInteractive*
// Qualifier:
//************************************
AInteractive* UCharacterInteractionComponent::AttemptInteract()
{
	switch (mInteractionState)
	{
	case EInteractionState::Free:
	{
		if (FindFirstInteractive)
		{
			AInteractive* interactive = TraceForSingleInteractive();
			if (interactive)
			{
				interactive->Interact(this);
			}
			return interactive;
		}
		else
		{
			TArray<AInteractive*> interactives = TraceForMultipleInteractives();
			// #MI_TODO: select the best one.

			if (interactives.Num() > 0)
			{
				return interactives[0];
			}
		}
	}
	break;

	case EInteractionState::HoldingItem:
	{
		AInteractive* interactive = mHeldInteractive;
		interactive->Interact(this);
		return interactive;
	}
	break;
	}

	return nullptr;
}

//************************************
// Method:    HoldInteractive
// FullName:  UCharacterInteractionComponent::HoldInteractive
// Access:    public 
// Returns:   bool
// Qualifier:
// Parameter: AInteractive * interactive
// Parameter: const FVector & localOffset
// Parameter: bool forceDrop
//************************************
bool UCharacterInteractionComponent::HoldInteractive(AInteractive* interactive, const FVector& localOffset /*=FVector::ZeroVector*/, bool forceDrop /*= false*/)
{
	if (forceDrop)
		DropInteractive();

	if (interactive && !mHeldInteractive && !mHeldWeapon)
	{
		StopAttacking();

		mHeldInteractive = interactive;
		if (ABaseWeapon* weapon = Cast<ABaseWeapon>(interactive))
		{
			mHeldWeapon = weapon;

			mHeldWeapon->Pickup(mCharacterOwner.Get()); // #NETWORK: could cause issues online...
			Delegate_OnPickupInteractive.Broadcast(mHeldInteractive, mHeldInteractive->Instigator != nullptr);

			mHeldWeapon->StartEquipping();
		}

		mInteractionState = EInteractionState::HoldingItem;

		return true;
	}

	return false;
}

//************************************
// Method:    DropInteractive
// FullName:  UCharacterInteractionComponent::DropInteractive
// Access:    public 
// Returns:   void
// Qualifier:
//************************************
void UCharacterInteractionComponent::DropInteractive()
{
	if (mHeldInteractive)
	{
		if (mHeldWeapon)
		{
			mHeldWeapon->OnDrop();
		}
	
		Delegate_OnDropInteractive.Broadcast(mHeldInteractive, false);

		mHeldInteractive = nullptr;
		mHeldWeapon = nullptr;
		mInteractionState = EInteractionState::Free;
	}
}

void UCharacterInteractionComponent::ThrowInteractive(UPrimitiveComponent* body, const FVector& impulse, const FVector& radialImpulse)
{
	if (mHeldInteractive)
	{
		if (mHeldWeapon)
		{
			mHeldWeapon->OnDrop();		
		}

		if (ThrowSound)
		{
			UAkGameplayStatics::PostEvent(ThrowSound, GetOwner(), false);
		}

		Delegate_OnDropInteractive.Broadcast(mHeldInteractive, true);

		mHeldInteractive = nullptr;
		mHeldWeapon = nullptr;
		mInteractionState = EInteractionState::Free;
	}
}

//************************************
// Method:    AttemptAttack
// FullName:  UCharacterInteractionComponent::AttemptAttack
// Access:    public 
// Returns:   bool
// Qualifier:
//************************************
bool UCharacterInteractionComponent::AttemptAttack()
{
	if (!mWantsToAttack)
	{
		mWantsToAttack = true;
		if (mHeldWeapon)
		{
			mHeldWeapon->StartAttacking();
			return true;
		}
		else if (mDefaultWeapon.IsValid())
		{
			mDefaultWeapon->StartAttacking();
			return true;
		}
	}	

	return false;
}

//************************************
// Method:    AttemptDashAttack
// FullName:  UCharacterMovementComponent::AttemptDashAttack
// Access:    public 
// Returns:   bool
// Qualifier:
//************************************
bool UCharacterInteractionComponent::AttemptDashAttack()
{
	if (!mWantsToAttack)
	{
		mWantsToAttack = true;
		if (mHeldWeapon)
		{
			return mHeldWeapon->DashAttack();
		}
		else if (mDefaultWeapon.IsValid())
		{
			return mDefaultWeapon->DashAttack();
		}
	}

	return false;
}

//************************************
// Method:    StopAttacking
// FullName:  UCharacterInteractionComponent::StopAttacking
// Access:    public 
// Returns:   void
// Qualifier:
//************************************
void UCharacterInteractionComponent::StopAttacking(bool forced /*= false*/)
{
	if (mWantsToAttack)
	{
		mWantsToAttack = false;
		if (mHeldWeapon && mHeldWeapon->GetWantsToAttack())
		{
			mHeldWeapon->StopAttacking(forced);
		}
		else if (mDefaultWeapon.IsValid() && mDefaultWeapon->GetWantsToAttack())
		{
			mDefaultWeapon->StopAttacking(forced);
		}
	}	
}

AInteractive* UCharacterInteractionComponent::TraceForSingleInteractive()
{
	UWorld* world = GetWorld();
	FHitResult hit;

	static const FName sInteractionTraceSingle(TEXT("InteractionTraceSingle"));

	FCollisionQueryParams params(sInteractionTraceSingle, false);
	params.bReturnPhysicalMaterial = false;
	params.bTraceAsyncScene = false;
	params.bFindInitialOverlaps = false;
	params.AddIgnoredActor(GetOwner());

	const FVector characterLocation = mCharacterOwner->GetActorLocation();
	const float capsuleHalfHeight = mCharacterOwner->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() - TraceRadius;

	const FVector start(characterLocation.X, characterLocation.Y, characterLocation.Z + capsuleHalfHeight);
	const FVector end(characterLocation.X, characterLocation.Y, characterLocation.Z - capsuleHalfHeight);	

	const bool foundHit = world->SweepSingleByChannel(hit, start, end, FQuat::Identity, ECC_InteractionTrace, FCollisionShape::MakeSphere(TraceRadius), params);
	
#if ENABLE_DRAW_DEBUG
	if (ShowDebug)
	{
		float lifetime = 5.f;

		if (foundHit && hit.bBlockingHit)
		{
			// Red up to the blocking hit, green thereafter
			UVertUtilities::DrawDebugSweptSphere(world, start, hit.Location, TraceRadius, FColor::Red, false, lifetime);
			UVertUtilities::DrawDebugSweptSphere(world, hit.Location, end, TraceRadius, FColor::Green, false, lifetime);
			DrawDebugPoint(world, hit.ImpactPoint, 16.f, FColor::Red, false, lifetime);
		}
		else
		{
			// no hit means all red
			UVertUtilities::DrawDebugSweptSphere(world, start, end, TraceRadius, FColor::Red, false, lifetime);
		}
	}
#endif

	if (foundHit && hit.Actor.IsValid())
	{
		UE_LOG(LogCharacterInteractionComponent, Log, TEXT("Sphere trace hit actor with name [%s]"), *hit.Actor->GetName());

		if (AInteractive* interactive = Cast<AInteractive>(hit.Actor.Get()))
		{
			return interactive;
		}
	}

	return nullptr;
}


TArray<AInteractive*> UCharacterInteractionComponent::TraceForMultipleInteractives()
{
	check(false); // out of date, see single interactive trace

	TArray<AInteractive*> interactives;

	if (UWorld* world = GetWorld())
	{
		FCollisionQueryParams params;
		params.AddIgnoredActor(GetOwner());
		params.bFindInitialOverlaps = false;
		
		FCollisionObjectQueryParams objectParams;
		objectParams.AddObjectTypesToQuery(ECC_Interactive);

		TArray<FHitResult> hits;
		AController* controller = mCharacterOwner->GetController();
		if (mCharacterOwner.IsValid() && controller)
		{
			FVector start = mCharacterOwner->GetActorLocation() + controller->GetControlRotation().RotateVector(LocalSphereTraceOffset);

			bool hasHit = UVertUtilities::SphereTraceMultiByObjectTypes(start, start, TraceRadius, hits, params, objectParams);

			for (int32 i = 0; i < hits.Num(); ++i)
			{
				if (hits[i].Actor.IsValid())
				{
					if (AInteractive* interactive = Cast<AInteractive>(hits[i].Actor.Get()))
					{
						interactives.Add(interactive);
					}
				}
			}
		}
	}

	return interactives;
}

void UCharacterInteractionComponent::DrawDebug()
{
	if (mCharacterOwner.IsValid())
	{
		if (AController* controller = mCharacterOwner->GetController())
		{
			DrawDebugSphere(GetWorld(), mCharacterOwner->GetActorLocation() + controller->GetControlRotation().RotateVector(LocalSphereTraceOffset), TraceRadius, 16, FColor::Yellow);
		}
	}
}