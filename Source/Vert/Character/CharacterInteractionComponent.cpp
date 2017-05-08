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

IInteractive* UCharacterInteractionComponent::AttemptInteract()
{
	switch (mInteractionState)
	{
	case EInteractionState::Free:
	{
		if (FindFirstInteractive)
		{
			IInteractive* interactive = TraceForSingleInteractive();
			if (interactive)
			{
				interactive->Interact(this);
			}
			return interactive;
		}
		else
		{
			TArray<IInteractive*> interactives = TraceForMultipleInteractives();
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
		IInteractive* interactive = mHeldInteractive;
		interactive->Interact(this);
		return interactive;
	}
	break;
	}

	return nullptr;
}

bool UCharacterInteractionComponent::HoldInteractive(IInteractive* interactive, const FVector& localOffset /*=FVector::ZeroVector*/, bool forceDrop /*= false*/)
{
	if (forceDrop)
		DropInteractive();

	if (interactive && !mHeldInteractive && !mHeldWeapon)
	{
		mHeldInteractive = interactive;
		if (ABaseWeapon* weapon = Cast<ABaseWeapon>(interactive))
		{
			mHeldWeapon = weapon;
		}

		if (AActor* actor = Cast<AActor>(interactive))
		{
			actor->DisableComponentsSimulatePhysics();
			actor->AttachToComponent(mCharacterOwner->GetSprite(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, mCharacterOwner->ItemHandSocket);
			actor->SetActorRelativeLocation(localOffset);
		}
		mInteractionState = EInteractionState::HoldingItem;

		return true;
	}

	return false;
}

void UCharacterInteractionComponent::DropInteractive()
{
	if (mHeldInteractive)
	{
		if (AActor* actor = Cast<AActor>(mHeldInteractive))
		{
			actor->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
		}

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
	if (mHeldWeapon)
	{
		mHeldWeapon->NotifyAttackCommand();

		return true;
	}

	return false;
}

void UCharacterInteractionComponent::StopAttacking()
{
	if (mHeldWeapon)
	{
		mHeldWeapon->NotifyStopAttacking();
	}
}

IInteractive* UCharacterInteractionComponent::TraceForSingleInteractive()
{
	if (UWorld* world = GetWorld())
	{
		FCollisionQueryParams params;
		params.AddIgnoredActor(GetOwner());
		params.bFindInitialOverlaps = false;

		FCollisionObjectQueryParams objectParams;
		objectParams.AddObjectTypesToQuery(ECC_Interactive);

		FHitResult hit;
		AController* controller = mCharacterOwner.IsValid() ? mCharacterOwner->GetController() : nullptr;
		if (mCharacterOwner.IsValid() && controller)
		{
			FVector start = mCharacterOwner->GetActorLocation() + controller->GetControlRotation().RotateVector(LocalSphereTraceOffset);

			bool hasHit = UVertUtilities::SphereTraceSingleByObjectTypes(start, start, TraceRange, hit, params, objectParams);

			if (hit.Actor.IsValid())
			{
				UE_LOG(LogCharacterInteractionComponent, Log, TEXT("Sphere trace hit actor with name [%s]"), *hit.Actor->GetName());

				if (IInteractive* interactive = Cast<IInteractive>(hit.Actor.Get()))
				{
					return interactive;
				}
			}
		}		
	}

	return nullptr;
}


TArray<IInteractive*> UCharacterInteractionComponent::TraceForMultipleInteractives()
{
	TArray<IInteractive*> interactives;

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

			bool hasHit = UVertUtilities::SphereTraceMultiByObjectTypes(start, start, TraceRange, hits, params, objectParams);

			for (int32 i = 0; i < hits.Num(); ++i)
			{
				if (hits[i].Actor.IsValid())
				{
					if (IInteractive* interactive = Cast<IInteractive>(hits[i].Actor.Get()))
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
			DrawDebugSphere(GetWorld(), mCharacterOwner->GetActorLocation() + controller->GetControlRotation().RotateVector(LocalSphereTraceOffset), TraceRange, 16, FColor::Yellow);
		}
	}
}