// Copyright Inside Out Games Ltd. 2017

#include "GrapplingComponent.h"

DECLARE_LOG_CATEGORY_CLASS(LogGrapplingComponent, Log, All);

UGrapplingComponent::UGrapplingComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
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
				spawnedGrapple->AttachToComponent(mCharacterOwner->GetMesh(), FAttachmentTransformRules(EAttachmentRule::SnapToTarget, EAttachmentRule::KeepWorld, EAttachmentRule::SnapToTarget, false), GrappleHandSocket);
				mGrappleLauncher = spawnedGrapple;
			}
		}
	}

	mRemainingGrapples = MaxGrapples;
	mRechargeTimer.BindAlarm(this, TEXT("OnGrappleRechargeTimerFinished"));

	if (mGrappleHook.IsValid() && mCharacterOwner.IsValid())
	{
		FScriptDelegate onBeginReturn;
		onBeginReturn.BindUFunction(mCharacterOwner.Get(), TEXT("Character_OnGrappleBeginReturn"));
		mGrappleHook->OnHookBeginReturn.Add(onBeginReturn);
	}
}


// Called every frame
void UGrapplingComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	UpdateRechargeSate();
	mRechargeTimer.TickTimer(DeltaTime);
}

bool UGrapplingComponent::ExecuteGrapple(FVector& aimDirection)
{
	if (mGrappleLauncher.IsValid() && (mRemainingGrapples > 0 || MaxGrapples == -1))
	{
		aimDirection = UVertUtilities::LimitAimTrajectory(AimFreedom, aimDirection);
		if (mGrappleLauncher->FireGrapple(aimDirection))
		{
			mRemainingGrapples = FMath::Max(0, mRemainingGrapples - 1);
			return true;
		}
	}

	return false;
}

void UGrapplingComponent::OnLanded()
{
	if (RecieveChargeOnGroundOnly)
	{
		mRemainingGrapples += mRechargeTimer.PopAlarmBacklog();
	}
}

void UGrapplingComponent::UpdateRechargeSate()
{
	if (mRemainingGrapples < MaxGrapples && mRechargeTimer.GetAlarmBacklog() < (MaxGrapples - mRemainingGrapples))
	{
		(mCharacterOwner->CanComponentRecharge(RechargeMode))
			? mRechargeTimer.Start()
			: mRechargeTimer.Stop();
	}
}

void UGrapplingComponent::RegisterGrappleHookDelegates(AGrappleHook* hook)
{
	if (hook)
	{
		mGrappleHook = hook;
	}
}

bool UGrapplingComponent::Reset()
{
	if (mGrappleLauncher.IsValid() && mGrappleHook.IsValid())
	{
		if (mGrappleHook->GetGrappleState() == EGrappleState::HookDeployed || mGrappleHook->GetGrappleState() == EGrappleState::HookDeployedAndReturning)
		{
			mGrappleLauncher->ResetGrapple();
			return true;
		}		
	}
	
	return false;
}

bool UGrapplingComponent::StartPulling()
{
	if (mGrappleLauncher.IsValid())
	{
		return mGrappleLauncher->StartPulling();
	}

	return false;
}

AGrappleHook* UGrapplingComponent::GetGrappleHookBP() const 
{
	return mGrappleHook.Get();
}

void UGrapplingComponent::OnGrappleRechargeTimerFinished_Implementation()
{
	if (!RecieveChargeOnGroundOnly || (!mCharacterOwner.IsValid() || mCharacterOwner->IsGrounded()))
	{
		mRemainingGrapples += mRechargeTimer.PopAlarmBacklog();
		if (!mCharacterOwner.IsValid())
			UE_LOG(LogGrapplingComponent, Warning, TEXT("Component has not AVertCharacter parent, grapple recharge may be innaccurate."));
	}
	mRechargeTimer.Reset();
}