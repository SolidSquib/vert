// Copyright Inside Out Games Ltd. 2017

#include "Vert.h"
#include "DashingComponent.h"

DECLARE_LOG_CATEGORY_CLASS(LogDashingComponent, Log, All);

// Sets default values for this component's properties
UDashingComponent::UDashingComponent()
	: UseMomentum(true),
	OverrideXY(true),
	OverrideZ(true)
{
	PrimaryComponentTick.bCanEverTick = true;
}

// Called when the game starts
void UDashingComponent::BeginPlay()
{
	Super::BeginPlay();

	mCharacterOwner = Cast<AVertCharacter>(GetOwner());
	if (!mCharacterOwner.IsValid())
		UE_LOG(LogDashingComponent, Warning, TEXT("Owner [%s] of DashingComponent is not an AVertCharacter, component functionality will be limited."), *GetOwner()->GetName());

	mRemainingDashes = MaxDashes;
	RechargeTimer.BindAlarm(this, TEXT("OnDashRechargeTimerFinished"));
}

// Called every frame
void UDashingComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	if (mIsDashing)
	{
		if (UseMomentum) // Add a one time impulse in the desired direction
		{
			if (DashTimer.GetProgressRatio() < 1.f)
				mCharacterOwner->LaunchCharacter(mDirectionOfTravel*LaunchForce, OverrideXY, OverrideZ);

			DashTimer.TickTimer(DeltaTime);

			if (DashTimer.IsFinished())
			{
				EndDash();
				DashTimer.Reset();
			}
		}
		else if (mCharacterOwner.IsValid())
		{
			float distanceToTravel = LinearSpeed*DeltaTime;
			float remainingDistance = DashLength - mDistanceTravelled;
			mCharacterOwner->SetActorLocation(mCharacterOwner->GetActorLocation() + (mDirectionOfTravel * FMath::Min(distanceToTravel, remainingDistance)));

			mDistanceTravelled += distanceToTravel;

			if (remainingDistance <= distanceToTravel)
			{
				EndDash();
				mDistanceTravelled = 0.f;
			}
		}
	}

	UpdateRechargeSate();
	RechargeTimer.TickTimer(DeltaTime);
}

void UDashingComponent::OnLanded()
{
	if (RecieveChargeOnGroundOnly)
	{
		mRemainingDashes += RechargeTimer.PopAlarmBacklog();
	}
}

bool UDashingComponent::ExecuteDash()
{
	if (!mIsDashing && (mRemainingDashes > 0 || MaxDashes == -1))
	{
		if (AimMode == EDashAimMode::AimDirection)
			mDirectionOfTravel = mCharacterOwner->GetAxisPostisions().GetPlayerRightThumbstickDirection();
		else if (AimMode == EDashAimMode::PlayerDirection)
			mDirectionOfTravel = mCharacterOwner->GetAxisPostisions().GetPlayerLeftThumbstickDirection();

		if (mDirectionOfTravel.X == 0 && mDirectionOfTravel.Y == 0)
		{
			mDirectionOfTravel = (mCharacterOwner->GetController()) ? mCharacterOwner->GetController()->GetControlRotation().RotateVector(FVector(1.f, 0.f, 0.f)) : FVector(1.f, 0.f, 0.f);
		}

		mDirectionOfTravel = UVertUtilities::LimitAimTrajectory(AimFreedom, mDirectionOfTravel);

		mIsDashing = true;
		mRemainingDashes = FMath::Max(0, mRemainingDashes-1);

		UE_LOG(LogDashingComponent, Log, TEXT("Dash successfully started by actor [%s]"), *GetOwner()->GetName());

		return true;
	}

	return false;
}

void UDashingComponent::UpdateRechargeSate()
{
	if (mRemainingDashes < MaxDashes && RechargeTimer.GetAlarmBacklog() < (MaxDashes - mRemainingDashes))
	{
		(mCharacterOwner->CanComponentRecharge(RechargeMode))
			? RechargeTimer.Start()
			: RechargeTimer.Stop();
	}
}

void UDashingComponent::EndDash()
{
	mIsDashing = false;
	OnDashEnd.Broadcast();

	UE_LOG(LogDashingComponent, Log, TEXT("Dash ended for actor [%s]"), *GetOwner()->GetName());
}

void UDashingComponent::OnDashRechargeTimerFinished_Implementation()
{
	if (!RecieveChargeOnGroundOnly || mCharacterOwner->IsGrounded())
		mRemainingDashes += RechargeTimer.PopAlarmBacklog();
	RechargeTimer.Reset();
}