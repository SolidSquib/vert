// Copyright Inside Out Games Ltd. 2017

#pragma once

#include "CoreMinimal.h"
#include "Navigation/PathFollowingComponent.h"
#include "CatchPathFollowingComponent.generated.h"

class UNavMovementComponent;
class UCharacterMovementComponent;

/**
 * 
 */
UCLASS()
class VERT_API UCatchPathFollowingComponent : public UPathFollowingComponent
{
	GENERATED_BODY()
	
protected:
	UPROPERTY(transient)
	UCharacterMovementComponent* CharacterMoveComp;

public:
	virtual void SetMoveSegment(int32 SegmentStartIndex) override;
	virtual void SetMovementComponent(UNavMovementComponent* MoveComp) override;
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction) override;

protected:
	void StopJumping();

private:
	FTimerHandle mTimerHandle_JumpHold;
	bool mExecutingJump = false;
	FVector mJumpDirection = FVector::ZeroVector;
};
