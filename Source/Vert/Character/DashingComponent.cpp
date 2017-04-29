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

		UE_LOG(LogTemp, Warning, TEXT("dot = %f"), dot);

		if (dot < -0.5f && mCharacterOwner->IsGrounded())
			return false;

		mIsDashing = mIsGroundDash = true;
		mRemainingDashes = FMath::Max(0, mRemainingDashes - 1);

		mStoredGroundFriction = mCharacterOwner->GetCharacterMovement()->GroundFriction;
		mCharacterOwner->GetCharacterMovement()->GroundFriction = 0;
		mCharacterOwner->GetCharacterMovement()->AddImpulse(LaunchForce*launchDirection);

		DashTimer.Start();
		OnDashStarted.Broadcast(launchDirection);

		UE_LOG(LogDashingComponent, Log, TEXT("Dash successfully started by actor [%s]"), *GetOwner()->GetName());
		return true;
	}

	return false;
}

bool UDashingComponent::ExecuteGrappleDash(const FVector& hookDirection)
{
	if (CanDash())
	{
		mIsDashing = true;
		mIsGroundDash = false;

		mRemainingDashes = FMath::Max(0, mRemainingDashes - 1);

		FVector launchDirection = FindLaunchDirection();
		float dot = FVector::DotProduct(launchDirection, hookDirection);

		if (dot < -SMALL_NUMBER)
		{
			UE_LOG(LogDashingComponent, Log, TEXT("Attempted dash towards hook."));
		}
		else if (dot > SMALL_NUMBER)
		{
			UE_LOG(LogDashingComponent, Log, TEXT("Attempted dash away from hook."));
		}

		OnDashStarted.Broadcast(launchDirection);
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

	UCharacterMovementComponent* movement = mCharacterOwner->GetCharacterMovement();
	mCharacterOwner->GetCharacterMovement()->GroundFriction = mStoredGroundFriction;

	mOnCooldown = true;

	DashTimer.Reset();

	if (mIsGroundDash && !mCharacterOwner->IsGrounded())
	{
		mStoredAirLateralFriction = mCharacterOwner->GetCharacterMovement()->FallingLateralFriction;
		mCharacterOwner->GetCharacterMovement()->FallingLateralFriction = AirSlowdownFriction;
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
	mCharacterOwner->GetCharacterMovement()->FallingLateralFriction = mStoredAirLateralFriction;
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