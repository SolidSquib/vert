// Copyright Inside Out Games Ltd. 2017

#include "Vert.h"
#include "CharacterInteractionComponent.h"

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
}

// Called every frame
void UCharacterInteractionComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (ShowDebug)
		DrawDebug();
}

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

bool UCharacterInteractionComponent::HoldInteractive(AInteractive* interactive, const FVector& localOffset /*=FVector::ZeroVector*/, bool forceDrop /*= false*/)
{
	if (forceDrop)
		DropInteractive();

	if (interactive && !mHeldInteractive && !mHeldWeapon)
	{
		mHeldInteractive = interactive;
		if (ABaseWeapon* weapon = Cast<ABaseWeapon>(interactive))
		{
			mHeldWeapon = weapon;
			mHeldWeapon->OnPickup(mCharacterOwner.Get());
		}

// 		if (AActor* actor = Cast<AActor>(interactive))
// 		{
// 			actor->DisableComponentsSimulatePhysics();
// 			actor->AttachToComponent(mCharacterOwner->GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, mCharacterOwner->ItemHandSocket);
// 			actor->SetActorRelativeLocation(localOffset);
// 		}
		mInteractionState = EInteractionState::HoldingItem;

		return true;
	}

	return false;
}

void UCharacterInteractionComponent::DropInteractive()
{
	if (mHeldInteractive)
	{
		if (mHeldWeapon)
		{
			mHeldWeapon->OnDrop();
		}
		//mHeldInteractive->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	
		mHeldInteractive = nullptr;
		mHeldWeapon = nullptr;
		mInteractionState = EInteractionState::Free;
	}
}

void UCharacterInteractionComponent::ForceDropInteractive(FVector force, float radialForce)
{

}

bool UCharacterInteractionComponent::AttemptAttack()
{
	if (!mWantsToAttack)
	{
		mWantsToAttack = true;
		if (mHeldWeapon)
		{
			mHeldWeapon->StartFire();

			return true;
		}
	}	

	return false;
}

void UCharacterInteractionComponent::StopAttacking()
{
	if (mWantsToAttack)
	{
		mWantsToAttack = false;
		if (mHeldWeapon)
		{
			mHeldWeapon->StopFire();
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