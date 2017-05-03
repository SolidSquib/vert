// Copyright Inside Out Games Ltd. 2017

#include "Vert.h"
#include "DashingComponent.h"

DECLARE_LOG_CATEGORY_CLASS(LogDashingComponent, Log, All);

// Sets default values for this component's properties
UDashingComponent::UDashingComponent()
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
	RechargeTimer.BindAlarm(this, TEXT("RechargeTimerEnded"));
	DashTimer.BindAlarm(this, TEXT("DashTimerEnded"));
	CooldownTimer.BindAlarm(this, TEXT("CooldownEnded"));
	AirSlowdownTimer.BindAlarm(this, TEXT("AirSlowdownTimerEnded"));
}

// Called every frame
void UDashingComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	UpdateRechargeState();
	DashTimer.TickTimer(DeltaTime);
	RechargeTimer.TickTimer(DeltaTime);
	CooldownTimer.TickTimer(DeltaTime);
	AirSlowdownTimer.TickTimer(DeltaTime);
}

void UDashingComponent::OnLanded()
{
	if (RecieveChargeOnGroundOnly)
	{
		mRemainingDashes += RechargeTimer.PopAlarmBacklog();
	}
}

bool UDashingComponent::ExecuteGroundDash()
{
	if (CanDash())
	{
		FVector launchDirection = FindLimitedLaunchDirection();
		float dot = FVector::DotProduct(launchDirection, FVector::UpVector);

		if (dot < -0.5f && mCharacterOwner->IsGrounded())
			return false;

		mIsDashing = mIsGroundDash = true;
		mRemainingDashes = FMath::Max(0, mRemainingDashes - 1);

		mCharacterOwner->GetVertCharacterMovement()->DisableGroundFriction();
		mCharacterOwner->GetCharacterMovement()->AddImpulse(LaunchForce*launchDirection);

		OnDashStarted.Broadcast(launchDirection);
		DashTimer.Start();

		UE_LOG(LogDashingComponent, Log, TEXT("Dash successfully started by actor [%s]"), *GetOwner()->GetName());
		return true;
	}

	return false;
}

bool UDashingComponent::ExecuteGrappleDash(const FVector& hookDirection, FGrappleDashResponse& response)
{
	constexpr float direction_threshold = 0.7f;

	if (CanDash())
	{
		FVector launchDirection = FindLaunchDirection();
		float dot = FVector::DotProduct(launchDirection, hookDirection);
		FVector rightVector = FVector::CrossProduct(hookDirection, FVector::RightVector);
		float dotRight = FVector::DotProduct(launchDirection, rightVector);

		if (dot > direction_threshold)
		{
			response.Action = EGrappleDashBehaviour::Break;
			response.Direction = hookDirection;
			mIsGroundDash = true;

			UE_LOG(LogDashingComponent, Log, TEXT("[%s] attempted dash towards grapple hook."), *GetName());
		}
		else// if (FMath::Abs(dotRight) > KINDA_SMALL_NUMBER)
		{			
			response.Action = EGrappleDashBehaviour::Impulse;
			response.Direction = (dotRight > 0) ? rightVector : -rightVector;
			mIsGroundDash = false;

			UE_LOG(LogDashingComponent, Log, TEXT("[%s] attempted lateral dash while grappled."), *GetName());
		}

		mIsDashing = true;
		mRemainingDashes = FMath::Max(0, mRemainingDashes - 1);

		mCharacterOwner->GetCharacterMovement()->AddImpulse(LaunchForce*response.Direction);

		OnDashStarted.Broadcast(response.Direction);
		DashTimer.Start();

		UE_LOG(LogDashingComponent, Log, TEXT("Dash successfully started by actor [%s]"), *GetOwner()->GetName());
		return true;
	}

	return false;
}

void UDashingComponent::UpdateRechargeState()
{
	if (mRemainingDashes < MaxDashes && RechargeTimer.GetAlarmBacklog() < (MaxDashes - mRemainingDashes))
	{
		(mCharacterOwner->CanComponentRecharge(RechargeMode))
			? RechargeTimer.Start()
			: RechargeTimer.Stop();
	}
}

void UDashingComponent::RechargeTimerEnded()
{
	if (!RecieveChargeOnGroundOnly || mCharacterOwner->IsGrounded())
		mRemainingDashes += RechargeTimer.PopAlarmBacklog();
	RechargeTimer.Reset();
}

void UDashingComponent::DashTimerEnded()
{
	mIsDashing = false;
	OnDashEnd.Broadcast();

	mCharacterOwner->GetVertCharacterMovement()->EnableGroundFriction();

	mOnCooldown = true;

	DashTimer.Reset();

	if (mIsGroundDash && !mCharacterOwner->IsGrounded())
	{
		mCharacterOwner->GetVertCharacterMovement()->AlterAirLateralFriction(AirSlowdownFriction);
		AirSlowdownTimer.Start();
	}

	CooldownTimer.Start();

	UE_LOG(LogDashingComponent, Log, TEXT("Dash ended for actor [%s]"), *GetOwner()->GetName());
}

void UDashingComponent::CooldownEnded()
{
	mOnCooldown = false;
	CooldownTimer.Reset();
}

void UDashingComponent::AirSlowdownTimerEnded()
{
	mCharacterOwner->GetVertCharacterMovement()->ResetAirLateralFriction();
	AirSlowdownTimer.Reset();
}

FVector UDashingComponent::FindLaunchDirection()
{
	FVector launchDirection = FVector::ZeroVector;
	launchDirection = mCharacterOwner->GetAxisPostisions().GetPlayerLeftThumbstickDirection();

	if (launchDirection.SizeSquared() <= SMALL_NUMBER)
	{
		launchDirection = (mCharacterOwner->GetController()) ? mCharacterOwner->GetController()->GetControlRotation().RotateVector(FVector(1.f, 0.f, 0.f)) : FVector(1.f, 0.f, 0.f);
	}

	return launchDirection;
}

FVector UDashingComponent::FindLimitedLaunchDirection()
{
	return UVertUtilities::LimitAimTrajectory(AimFreedom, FindLaunchDirection());
}