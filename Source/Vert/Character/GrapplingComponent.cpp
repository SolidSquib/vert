// Copyright Inside Out Games Ltd. 2017

#include "Vert.h"
#include "GrapplingComponent.h"

DECLARE_LOG_CATEGORY_CLASS(LogGrapplingComponent, Log, All);

// Sets default values for this component's properties
UGrapplingComponent::UGrapplingComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


// Called when the game starts
void UGrapplingComponent::BeginPlay()
{
	Super::BeginPlay();

	mCharacterOwner = Cast<AVertCharacter>(GetOwner());
	if (!mCharacterOwner.IsValid())
		UE_LOG(LogGrapplingComponent, Warning, TEXT("Owner [%s] of GrapplingComponent is not an AVertCharacter, component functionality will be limited."), *GetOwner()->GetName());

	if (mCharacterOwner.IsValid() && GrappleClass != nullptr && GrappleHandSocket != NAME_None)
	{
		if (UWorld* world = GetWorld())
		{
			//Setup spawn parameters for the actor.
			FActorSpawnParameters spawnParameters;
			spawnParameters.Owner = GetOwner();
			spawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

			//Spawn the actor.
			AGrappleLauncher* spawnedGrapple = world->SpawnActor<AGrappleLauncher>(GrappleClass, spawnParameters);

			//Assert that the actor exists.
			check(spawnedGrapple);

			if (spawnedGrapple)
			{
				//Attach it to the player's hand.
				spawnedGrapple->AttachToComponent(mCharacterOwner->GetSprite(), FAttachmentTransformRules(EAttachmentRule::SnapToTarget, EAttachmentRule::KeepWorld, EAttachmentRule::SnapToTarget, false), GrappleHandSocket);
				mGrappleLauncher = spawnedGrapple;
			}
		}
	}

	mRemainingGrapples = MaxGrapples;
	Grapple.RechargeTimer.BindAlarm(this, TEXT("OnGrappleRechargeTimerFinished"));
}


// Called every frame
void UGrapplingComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	UpdateRechargeSate();

	Grapple.RechargeTimer.TickTimer(DeltaTime);
}

bool UGrapplingComponent::ExecuteGrapple(const FVector& aimDirection)
{
	if (mGrappleLauncher.IsValid() && (mRemainingGrapples > 0 || mRemainingGrapples == -1))
	{
		if (mGrappleLauncher->FireGrapple(UVertUtilities::LimitAimTrajectory(Grapple.AimFreedom, aimDirection)))
		{
			mRemainingGrapples = FMath::Max(0, mRemainingGrapples - 1);
			return true;
		}
	}

	return false;
}

void UGrapplingComponent::OnLanded()
{
	if (Grapple.RecieveChargeOnGroundOnly)
	{
		mRemainingGrapples += Grapple.RechargeTimer.PopAlarmBacklog();
	}
}

void UGrapplingComponent::UpdateRechargeSate()
{
	if (mRemainingGrapples < MaxGrapples && Grapple.RechargeTimer.GetAlarmBacklog() < (MaxGrapples - mRemainingGrapples))
	{
		(mCharacterOwner->CanComponentRecharge(Grapple.RechargeMode))
			? Grapple.RechargeTimer.Start()
			: Grapple.RechargeTimer.Stop();
	}
}

void UGrapplingComponent::RegisterGrappleHookDelegates(AGrappleHook* hook)
{
	if (hook)
	{
		FScriptDelegate onHookedDelegate;
		onHookedDelegate.BindUFunction(this, TEXT("OnHooked"));
		hook->OnHooked.Add(onHookedDelegate);

		FScriptDelegate onFiredDelegate;
		onFiredDelegate.BindUFunction(this, TEXT("OnFired"));
		hook->OnFired.Add(onFiredDelegate);

		FScriptDelegate onReturnedDelegate;
		onReturnedDelegate.BindUFunction(this, TEXT("OnReturned"));
		hook->OnReturned.Add(onReturnedDelegate);

		FScriptDelegate onLatchedDelegate;
		onLatchedDelegate.BindUFunction(this, TEXT("OnLatched"));
		hook->OnLatched.Add(onLatchedDelegate);

		FScriptDelegate onUnLatchedDelegate;
		onUnLatchedDelegate.BindUFunction(this, TEXT("OnUnLatched"));
		hook->OnUnLatched.Add(onUnLatchedDelegate);

		if (mCharacterOwner.IsValid())
		{
			if (UVertCharacterMovementComponent* movement = mCharacterOwner->GetVertCharacterMovement())
			{
				movement->RegisterHookDelegates(hook);
			}
		}
		else
			UE_LOG(LogGrapplingComponent, Warning, TEXT("Component has no AVertCharacter parent, cannot register AGrappleHook delegates."));

		mGrappleHook = hook;
	}
}

bool UGrapplingComponent::Reset()
{
	if (mGrappleLauncher.IsValid() && mGrappleHook.IsValid())
	{
		if (mGrappleHook->GetGrappleState() == EGrappleState::Hooked || mGrappleHook->GetGrappleState() == EGrappleState::Latched)
		{
			mGrappleLauncher->ResetGrapple();
			return true;
		}		
	}
	
	return false;
}

void UGrapplingComponent::OnGrappleRechargeTimerFinished_Implementation()
{
	if (!Grapple.RecieveChargeOnGroundOnly || (!mCharacterOwner.IsValid() || mCharacterOwner->IsGrounded()))
	{
		mRemainingGrapples += Grapple.RechargeTimer.PopAlarmBacklog();
		if (!mCharacterOwner.IsValid())
			UE_LOG(LogGrapplingComponent, Warning, TEXT("Component has not AVertCharacter parent, grapple recharge may be innaccurate."));
	}
	Grapple.RechargeTimer.Reset();
}

void UGrapplingComponent::OnHooked_Implementation()
{
//	if (DisableInputWhenDashingOrGrappling)
//	{
//		mDisableDash = mDisableGrapple = mDisableMovement = mDisableJump = false;
//	}
}

void UGrapplingComponent::OnFired_Implementation()
{
//	if (DisableInputWhenDashingOrGrappling)
//	{
//		mDisableDash = mDisableGrapple = mDisableMovement = mDisableJump = true;
//	}
}

void UGrapplingComponent::OnReturned_Implementation()
{
	//if (DisableInputWhenDashingOrGrappling)
	//{
	//	mDisableDash = mDisableGrapple = mDisableMovement = mDisableJump = false;
	//}
}

void UGrapplingComponent::OnLatched_Implementation(AGrappleHook* hook)
{
	//mDisableMovement = mDisableDash = true;
}

void UGrapplingComponent::OnUnLatched_Implementation(AGrappleHook* hook)
{
	//mDisableMovement = mDisableDash = false;
}