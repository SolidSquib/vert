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
	void AddGrappleLineForce(const float desiredLineLength, const float actualLineLength, const FVector& direction, const float k, const float b);
	void AddGrappleLineForce(const FVector& desiredLineLength, const FVector& actualLineLength, const float k, const float b);

	virtual bool DoJump(bool replayingMoves) override;
	virtual void BeginPlay() override;
	
	UFUNCTION(BlueprintCallable)
	void AlterAirLateralFriction(float newFriction);

	FORCEINLINE UFUNCTION(BlueprintCallable)
	void DisableGravity() { mSavedGravityScale = GravityScale; GravityScale = 0.0f; }

	FORCEINLINE UFUNCTION(BlueprintCallable)
	void DisableGroundFriction() { mSavedGroundFriction = GroundFriction; GroundFriction = 0.0f; }

	FORCEINLINE UFUNCTION(BlueprintCallable)
	void ResetAirLateralFriction() { FallingLateralFriction = mSavedAirLateralFriction; }

	FORCEINLINE UFUNCTION(BlueprintCallable)
	void EnableGravity() { GravityScale = mSavedGravityScale; }

	FORCEINLINE UFUNCTION(BlueprintCallable)
	void EnableGroundFriction() { GroundFriction = mSavedGroundFriction; }

private:
	TWeakObjectPtr<class UGrapplingComponent> mGrapplingComponent = nullptr;
	TWeakObjectPtr<class UDashingComponent> mDashingComponent = nullptr;
	TWeakObjectPtr<class AVertCharacter> mCharacterOwner = nullptr;

	float mCurrentGrappleLengthSqr = 0.f;
	bool mIsGrappling = false;
	bool mIsGrappleLatched = false;
	float mSavedGravityScale = 1.f;
	float mSavedGroundFriction = 3.0f;
	float mSavedAirLateralFriction = 0.f;
	float mStartingGravityScale = 1.f;
	float mStartingGroundFriction = 3.0f;
	float mStartingAirLateralFriction = 0.f;
};
