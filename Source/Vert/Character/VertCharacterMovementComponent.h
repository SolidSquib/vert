// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameFramework/CharacterMovementComponent.h"
#include "VertCharacterMovementComponent.generated.h"

/**
 * 
 */
UCLASS()
class VERT_API UVertCharacterMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Debug)
	bool ShowDebug = false;

public:
	FVector DetermineVelocityAfterGravityForGrapple(const FVector& initialVelocity, const FVector& gravity, float deltaTime) const;

	virtual bool DoJump(bool replayingMoves) override;
	virtual void BeginPlay() override;
	virtual bool IsFalling() const override;
	virtual bool IsMovingOnGround() const override;
	virtual FVector NewFallVelocity(const FVector& InitialVelocity, const FVector& Gravity, float DeltaTime) const override;
	virtual void SetPostLandedPhysics(const FHitResult& Hit) override;
	virtual void StartFalling(int32 Iterations, float remainingTime, float timeTick, const FVector& Delta, const FVector& subLoc) override;

	UFUNCTION(BlueprintCallable, Category = "Pawn|Components|CharacterMovement")
	virtual void SetMovementMode(EMovementMode NewMovementMode, uint8 NewCustomMode = 0) override;

	UFUNCTION(BlueprintCallable, Category = "Friction")
	virtual void SetDashFrictionForTime(float time, bool disableGravity = false);

protected:
	virtual void PhysClimbing(float deltaTime, int32 iterations);
	virtual void PhysGrappling(float deltaTime, int32 iterations);
	virtual void PhysGrappleFalling(float deltaTime, int32 iterations);
	virtual void CorrectGrapplePosition();
	virtual void DashFrictionEnded();
	virtual void DashFrictionCorrectionEnded();
	virtual void PhysCustom(float deltaTime, int32 Iterations) override;

private:
	TWeakObjectPtr<class UGrapplingComponent> mGrapplingComponent = nullptr;
	TWeakObjectPtr<class UDashingComponent> mDashingComponent = nullptr;
	TWeakObjectPtr<class AVertCharacter> mCharacterOwner = nullptr;

	float mCurrentGrappleLengthSqr = 0.f;
	bool mIsGrappling = false;
	bool mIsGrappleLatched = false;
	float mStartingGravityScale = 1.f;
	float mStartingGroundFriction = 3.0f;
	float mStartingAirLateralFriction = 0.f;

	FVector storedSwingDirection = FVector::ZeroVector;
	float storedMagnitude = 0;
	float lerpSpeed = 100.f;

	FTimerHandle mTimerHandle_Friction;
	FTimerHandle mTimerHandle_PostDash;
};
