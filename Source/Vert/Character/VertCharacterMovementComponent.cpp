// Fill out your copyright notice in the Description page of Project Settings.

#include "VertCharacterMovementComponent.h"
#include "Vert.h"

DECLARE_LOG_CATEGORY_CLASS(LogVertCharacterMovement, Log, All);

void UVertCharacterMovementComponent::AddGrappleLineForce(const float desiredLineLength, const float actualLineLength, const FVector& direction, const float k, const float b)
{
	float magnitude = -k * (actualLineLength - desiredLineLength) - b * Velocity.Size();
	//AddForce(magnitude*direction.GetSafeNormal());
	CharacterOwner->LaunchCharacter(magnitude*direction.GetSafeNormal(), false, true);
}

void UVertCharacterMovementComponent::AddGrappleLineForce(const FVector& desiredLineLength, const FVector& actualLineLength, const float k, const float b)
{
	FVector force = -k * (actualLineLength - desiredLineLength) - b * Velocity;
	AddForce(force);
}

bool UVertCharacterMovementComponent::DoJump(bool replayingMoves)
{
	if (mCharacterOwner.IsValid() && mCharacterOwner->CanJump())
	{
		// Don't jump if we can't move up/down.
		if (!bConstrainToPlane || FMath::Abs(PlaneConstraintNormal.Z) != 1.f)
		{
			float zVelocity = JumpZVelocity;

			if (mCharacterOwner->CanWallJump())
			{

			}
			else if (mCharacterOwner->GetClimbingComponent()->IsClimbingLedge())
			{

			}

			Velocity.Z = zVelocity;
			SetMovementMode(MOVE_Falling);
			return true;
		}
	}

	return false;
}

void UVertCharacterMovementComponent::BeginPlay()
{
	Super::BeginPlay();

	mStartingGravityScale = mSavedGravityScale = GravityScale;
	mStartingGroundFriction = mSavedGroundFriction = GroundFriction;
	mStartingAirLateralFriction = mSavedAirLateralFriction = FallingLateralFriction;

	if (AVertCharacter* character = Cast<AVertCharacter>(CharacterOwner))
	{
		mCharacterOwner = character;
		mDashingComponent = character->GetDashingComponent();
		mGrapplingComponent = character->GetGrapplingComponent();

		check(mGrapplingComponent.IsValid() && mDashingComponent.IsValid());
	} else { UE_LOG(LogVertCharacterMovement, Error, TEXT("[%s] unable to find AVertCharacter owner.")); }
}

void UVertCharacterMovementComponent::AlterAirLateralFriction(float newFriction)
{
	if (mSavedAirLateralFriction != mStartingAirLateralFriction)
	{
		UE_LOG(LogVertCharacterMovement, Warning, TEXT("Might be altering an altered value for lateral friction!"));
	}
	
	mSavedAirLateralFriction = FallingLateralFriction;
	FallingLateralFriction = newFriction;
}