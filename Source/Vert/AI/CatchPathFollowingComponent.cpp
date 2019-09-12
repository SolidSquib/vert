// Copyright Inside Out Games Ltd. 2017

#include "CatchPathFollowingComponent.h"
#include "NavAreaJump.h"
#include "GameFramework/Character.h"
#include "AIController.h"

//************************************
// Method:    SetMoveSegment
// FullName:  UCatchPathFollowingComponent::SetMoveSegment
// Access:    virtual public 
// Returns:   void
// Qualifier:
// Parameter: int32 SegmentStartIndex
//************************************
void UCatchPathFollowingComponent::SetMoveSegment(int32 SegmentStartIndex)
{
	FVector currentMoveInput = CurrentMoveInput;

	Super::SetMoveSegment(SegmentStartIndex);

#if 0
	if (CharacterMoveComp != NULL)
	{
		const FNavPathPoint& SegmentStart = Path->GetPathPoints()[MoveSegmentStartIndex];

		if (FNavAreaHelper::HasJumpFlag(SegmentStart))
		{
			CharacterMoveComp->SetMovementMode(MOVE_Flying);
			if (AAIController* controller = Cast<AAIController>(GetOwner()))
			{
				if (ACharacter* character = Cast<ACharacter>(controller->GetPawn()))
				{
					character->Jump();
				}
			}
			CharacterMoveComp->SetMovementMode(MOVE_Walking);
		}
		else
		{
			//regular movement
			CharacterMoveComp->SetMovementMode(MOVE_Walking);
			mExecutingJump = false;
		}
	}
#endif
}

//************************************
// Method:    SetMovementComponent
// FullName:  UCatchPathFollowingComponent::SetMovementComponent
// Access:    virtual public 
// Returns:   void
// Qualifier:
// Parameter: UNavMovementComponent * MoveComp
//************************************
void UCatchPathFollowingComponent::SetMovementComponent(UNavMovementComponent * MoveComp)
{
	Super::SetMovementComponent(MoveComp);
	CharacterMoveComp = Cast<UCharacterMovementComponent>(MovementComp);
}

//************************************
// Method:    StopJumping
// FullName:  UCatchPathFollowingComponent::StopJumping
// Access:    protected 
// Returns:   void
// Qualifier:
//************************************
void UCatchPathFollowingComponent::StopJumping()
{
	mExecutingJump = false;
}

//************************************
// Method:    TickComponent
// FullName:  UCatchPathFollowingComponent::TickComponent
// Access:    virtual public 
// Returns:   void
// Qualifier:
// Parameter: float DeltaTime
// Parameter: enum ELevelTick TickType
// Parameter: FActorComponentTickFunction * ThisTickFunction
//************************************
void UCatchPathFollowingComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (mExecutingJump)
	{
		if (AAIController* controller = Cast<AAIController>(GetOwner()))
		{
			if (ACharacter* character = Cast<ACharacter>(controller->GetPawn()))
			{
				character->LaunchCharacter(FVector::UpVector * CharacterMoveComp->JumpZVelocity, false, true);
			}
		}
	}
}