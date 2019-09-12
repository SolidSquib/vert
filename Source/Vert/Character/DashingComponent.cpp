// Copyright Inside Out Games Ltd. 2017

#include "DashingComponent.h"
#include "VertCharacterMovementComponent.h"
#include "AkAudio/Classes/AkGameplayStatics.h"

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
}

// Called every frame
void UDashingComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	UpdateRechargeState();
	RechargeTimer.TickTimer(DeltaTime);

	if (mIsDashing && mCharacterOwner.IsValid())
	{
		UCharacterMovementComponent* movement = mCharacterOwner->GetCharacterMovement();
		if (movement->MovementMode == MOVE_Custom && movement->CustomMovementMode == static_cast<uint8>(ECustomMovementMode::MOVE_GrappleWalk))
		{
			if (mCharacterOwner->GetGrapplingComponent())
			{
				float cos = FVector::DotProduct(mCurrentDashDirection, mCharacterOwner->GetGrapplingComponent()->GetLineDirection());
				if (cos < 0)
				{
					movement->SetMovementMode(MOVE_Custom, static_cast<uint8>(ECustomMovementMode::MOVE_GrappleFall));
				}				
			}
		}
	}
}

//************************************
// Method:    OnLanded
// FullName:  UDashingComponent::OnLanded
// Access:    public 
// Returns:   void
// Qualifier:
//************************************
void UDashingComponent::OnLanded()
{
	if (RecieveChargeOnGroundOnly)
	{
		mRemainingDashes += RechargeTimer.PopAlarmBacklog();
	}
}

//************************************
// Method:    StopDashing
// FullName:  UDashingComponent::StopDashing
// Access:    public 
// Returns:   void
// Qualifier:
//************************************
void UDashingComponent::StopDashing()
{
	// Stop the dash early if it's in active state.
	if (GetWorld()->GetTimerManager().IsTimerActive(mTimerHandle_Dash))
	{
		GetWorld()->GetTimerManager().ClearTimer(mTimerHandle_Dash);
		DashTimerEnded();
	}
}


//************************************
// Method:    ExecuteGroundDash
// FullName:  UDashingComponent::ExecuteGroundDash
// Access:    public 
// Returns:   bool
// Qualifier:
// Parameter: FVector & outDirection
//************************************
bool UDashingComponent::ExecuteDash(FVector& outDirection)
{
	if (CanDash())
	{
		outDirection = FindLimitedLaunchDirection();
		float dot = FVector::DotProduct(outDirection, FVector::UpVector);

		if (dot < -0.5f && mCharacterOwner->IsGrounded())
			return false;

		mIsDashing = mIsGroundDash = true;
		mRemainingDashes = FMath::Max(0, mRemainingDashes - 1);

		mCharacterOwner->GetVertCharacterMovement()->SetDashFrictionForTime(DashTimer.EndTime);
		mCharacterOwner->GetCharacterMovement()->AddImpulse(LaunchForce*outDirection);

		mCurrentDashDirection = outDirection;

		GetWorld()->GetTimerManager().SetTimer(mTimerHandle_Dash, this, &UDashingComponent::DashTimerEnded, DashTimer.EndTime, false);

		if (UCharacterMovementComponent* movement = mCharacterOwner->GetCharacterMovement())
		{
			if (movement->MovementMode == MOVE_Custom && movement->CustomMovementMode == static_cast<uint8>(ECustomMovementMode::MOVE_GrappleWalk))
			{
				if (mCharacterOwner->GetGrapplingComponent())
				{
					float cos = FVector::DotProduct(outDirection, mCharacterOwner->GetGrapplingComponent()->GetLineDirection());
					if(cos < 0)
						movement->SetMovementMode(MOVE_Custom, static_cast<uint8>(ECustomMovementMode::MOVE_GrappleFall));
				}
				else
				{
					movement->SetMovementMode(MOVE_Custom, static_cast<uint8>(ECustomMovementMode::MOVE_GrappleFall));
				}
			}
		}

		if (DashSound)
		{
			UAkGameplayStatics::PostEvent(DashSound, GetOwner(), false);
		}

		UE_LOG(LogDashingComponent, Log, TEXT("Dash successfully started by actor [%s]"), *GetOwner()->GetName());
		return true;
	}

	return false;
}

//************************************
// Method:    UpdateRechargeState
// FullName:  UDashingComponent::UpdateRechargeState
// Access:    private 
// Returns:   void
// Qualifier:
//************************************
void UDashingComponent::UpdateRechargeState()
{
	if (mRemainingDashes < MaxDashes && RechargeTimer.GetAlarmBacklog() < (MaxDashes - mRemainingDashes))
	{
		(mCharacterOwner->CanComponentRecharge(RechargeMode))
			? RechargeTimer.Start()
			: RechargeTimer.Stop();
	}
}

//************************************
// Method:    RechargeTimerEnded
// FullName:  UDashingComponent::RechargeTimerEnded
// Access:    private 
// Returns:   void
// Qualifier:
//************************************
void UDashingComponent::RechargeTimerEnded()
{
	if (!RecieveChargeOnGroundOnly || mCharacterOwner->IsGrounded())
		mRemainingDashes += RechargeTimer.PopAlarmBacklog();
	RechargeTimer.Reset();
}

//************************************
// Method:    DashTimerEnded
// FullName:  UDashingComponent::DashTimerEnded
// Access:    private 
// Returns:   void
// Qualifier:
//************************************
void UDashingComponent::DashTimerEnded()
{
	mIsDashing = false;
	OnDashEnd.Broadcast();

	mOnCooldown = true;
	mCurrentDashDirection = FVector::ZeroVector;

	DashTimer.Reset();

	GetWorld()->GetTimerManager().SetTimer(mTimerHandle_Cooldown, this, &UDashingComponent::CooldownEnded, CooldownTimer.EndTime, false);

	UE_LOG(LogDashingComponent, Log, TEXT("Dash ended for actor [%s]"), *GetOwner()->GetName());
}

//************************************
// Method:    CooldownEnded
// FullName:  UDashingComponent::CooldownEnded
// Access:    private 
// Returns:   void
// Qualifier:
//************************************
void UDashingComponent::CooldownEnded()
{
	mOnCooldown = false;
}

//************************************
// Method:    FindLaunchDirection
// FullName:  UDashingComponent::FindLaunchDirection
// Access:    private 
// Returns:   FVector
// Qualifier:
//************************************
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

//************************************
// Method:    FindLimitedLaunchDirection
// FullName:  UDashingComponent::FindLimitedLaunchDirection
// Access:    private 
// Returns:   FVector
// Qualifier:
//************************************
FVector UDashingComponent::FindLimitedLaunchDirection()
{
	return UVertUtilities::LimitAimTrajectory(AimFreedom, FindLaunchDirection());
}