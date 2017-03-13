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

	FORCEINLINE bool CanDash() { return !mIsGrappling; }
	FORCEINLINE void LoadGravityScale() { GravityScale = mSavedGravityScale; }
	FORCEINLINE void LoadGroundFriction() { GroundFriction = mSavedGroundFriction; }

	UFUNCTION(BlueprintNativeEvent, Category = "Grappling")
	void OnGrapplePull(const FVector& direction, const float force);

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

private:
	void SnapCharacterToHook(AGrappleHook* hook);
	void UnSnapCharacterFromHook(AGrappleHook* hook);

private:
	bool mIsGrappling = false;
	bool mIsGrappleLatched = false;
	float mSavedGravityScale = 1.f;
	float mSavedGroundFriction = 3.0f;
};
