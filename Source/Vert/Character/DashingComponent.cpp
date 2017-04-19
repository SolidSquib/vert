// Copyright Inside Out Games Ltd. 2017

#include "Vert.h"
#include "DashingComponent.h"

DECLARE_LOG_CATEGORY_CLASS(LogDashingComponent, Log, All);

// Sets default values for this component's properties
UDashingComponent::UDashingComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
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
	Dash.RechargeTimer.BindAlarm(this, TEXT("OnDashRechargeTimerFinished"));
}

// Called every frame
void UDashingComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	if (Dash.IsDashing)
	{
		if (Dash.UseMomentum) // Add a one time impulse in the desired direction
		{
			if (Dash.Timer <= 0.f)
				mCharacterOwner->LaunchCharacter(Dash.DirectionOfTravel*Dash.LaunchForce, Dash.OverrideXY, Dash.OverrideZ);

			Dash.Timer += DeltaTime;

			if (Dash.Timer >= Dash.TimeToDash)
			{
				EndDash();
				Dash.Timer = 0.f;
			}
		}
		else if (mCharacterOwner.IsValid())
		{
			float distanceToTravel = Dash.LinearSpeed*DeltaTime;
			float remainingDistance = Dash.DashLength - Dash.DistanceTravelled;
			mCharacterOwner->SetActorLocation(mCharacterOwner->GetActorLocation() + (Dash.DirectionOfTravel * FMath::Min(distanceToTravel, remainingDistance)));

			Dash.DistanceTravelled += distanceToTravel;

			if (remainingDistance <= distanceToTravel)
			{
				EndDash();
				Dash.DistanceTravelled = 0.f;
			}
		}
	}

	UpdateRechargeSate();
	Dash.RechargeTimer.TickTimer(DeltaTime);
}

void UDashingComponent::OnLanded()
{
	if (Dash.RecieveChargeOnGroundOnly)
	{
		mRemainingDashes += Dash.RechargeTimer.PopAlarmBacklog();
	}
}

bool UDashingComponent::ExecuteDash()
{
	if (!Dash.IsDashing && (mRemainingDashes > 0 || MaxDashes == -1))
	{
		if (Dash.AimMode == EDashAimMode::AimDirection)
			Dash.DirectionOfTravel = mCharacterOwner->GetAxisPostisions().GetPlayerRightThumbstickDirection();
		else if (Dash.AimMode == EDashAimMode::PlayerDirection)
			Dash.DirectionOfTravel = mCharacterOwner->GetAxisPostisions().GetPlayerLeftThumbstickDirection();

		if (Dash.DirectionOfTravel.X == 0 && Dash.DirectionOfTravel.Y == 0)
		{
			Dash.DirectionOfTravel = (mCharacterOwner->GetController()) ? mCharacterOwner->GetController()->GetControlRotation().RotateVector(FVector(1.f, 0.f, 0.f)) : FVector(1.f, 0.f, 0.f);
		}

		Dash.DirectionOfTravel = UVertUtilities::LimitAimTrajectory(Dash.AimFreedom, Dash.DirectionOfTravel);

		Dash.IsDashing = true;
		mRemainingDashes = FMath::Max(0, mRemainingDashes-1);

		UE_LOG(LogDashingComponent, Log, TEXT("Dash successfully started by actor [%s]"), *GetOwner()->GetName());

		return true;
	}

	return false;
}

void UDashingComponent::UpdateRechargeSate()
{
	if (mRemainingDashes < MaxDashes && Dash.RechargeTimer.GetAlarmBacklog() < (MaxDashes - mRemainingDashes))
	{
		(mCharacterOwner->CanComponentRecharge(Dash.RechargeMode))
			? Dash.RechargeTimer.Start()
			: Dash.RechargeTimer.Stop();
	}
}

void UDashingComponent::EndDash()
{
	Dash.IsDashing = false;
	OnDashEnd.Broadcast();

	UE_LOG(LogDashingComponent, Log, TEXT("Dash ended for actor [%s]"), *GetOwner()->GetName());
}

void UDashingComponent::OnDashRechargeTimerFinished_Implementation()
{
	if (!Dash.RecieveChargeOnGroundOnly || mCharacterOwner->IsGrounded())
		mRemainingDashes += Dash.RechargeTimer.PopAlarmBacklog();
	Dash.RechargeTimer.Reset();
}