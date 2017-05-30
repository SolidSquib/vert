// Fill out your copyright notice in the Description page of Project Settings.

#include "Vert.h"
#include "VertCharacterMovementComponent.h"

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
	bool result =  Super::DoJump(replayingMoves);
	
	// if false because of charactermovement limitations, allow it when we are latched (wall jump)
	if (!result && mIsGrappleLatched && CharacterOwner && !CharacterOwner->CanJump())
		result = true;

	return result;
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