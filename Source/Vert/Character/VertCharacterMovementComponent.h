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
	void RegisterHookDelegates(class AGrappleHook* hook);

	virtual bool DoJump(bool replayingMoves) override;
	virtual void BeginPlay() override;

	UFUNCTION(BlueprintNativeEvent, Category = "Grappling")
	void OnGrapplePull(AGrappleHook* hook, const FVector& direction, const float force);

	UFUNCTION(BlueprintNativeEvent, Category = "Grappling")
	void OnHooked();

	UFUNCTION(BlueprintNativeEvent, Category = "Grappling")
	void OnFired();

	UFUNCTION(BlueprintNativeEvent, Category = "Grappling")
	void OnReturned();

	UFUNCTION(BlueprintNativeEvent, Category = "Grappling")
	void OnLatched(AGrappleHook* hook);
	
	UFUNCTION(BlueprintNativeEvent, Category = "Grappling")
	void OnUnLatched(AGrappleHook* hook);

	FORCEINLINE UFUNCTION(BlueprintCallable)
	void DisableGravity() { mSavedGravityScale = GravityScale; GravityScale = 0.0f; }

	FORCEINLINE UFUNCTION(BlueprintCallable)
	void DisableGroundFriction() { mSavedGroundFriction = GroundFriction; GroundFriction = 0.0f; }

	FORCEINLINE UFUNCTION(BlueprintCallable)
	void EnableGravity() { GravityScale = mSavedGravityScale; }

	FORCEINLINE UFUNCTION(BlueprintCallable)
	void EnableGroundFriction() { GroundFriction = mSavedGroundFriction; }

private:
	void SnapCharacterToHook(AGrappleHook* hook);
	void UnSnapCharacterFromHook(AGrappleHook* hook);

private:
	TWeakObjectPtr<class UGrapplingComponent> mGrapplingComponent = nullptr;
	TWeakObjectPtr<class UDashingComponent> mDashingComponent = nullptr;
	TWeakObjectPtr<class AVertCharacter> mCharacterOwner = nullptr;

	float mCurrentGrappleLengthSqr = 0.f;
	bool mIsGrappling = false;
	bool mIsGrappleLatched = false;
	float mSavedGravityScale = 1.f;
	float mSavedGroundFriction = 3.0f;
};
