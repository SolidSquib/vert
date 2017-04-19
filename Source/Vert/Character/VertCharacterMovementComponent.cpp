// Fill out your copyright notice in the Description page of Project Settings.

#include "Vert.h"
#include "VertCharacterMovementComponent.h"

void UVertCharacterMovementComponent::RegisterHookDelegates(AGrappleHook* hook)
{
	FScriptDelegate onHookedDelegate;
	onHookedDelegate.BindUFunction(this, TEXT("OnHooked"));
	hook->OnHooked.Add(onHookedDelegate);

	FScriptDelegate onFiredDelegate;
	onFiredDelegate.BindUFunction(this, TEXT("OnFired"));
	hook->OnFired.Add(onFiredDelegate);

	FScriptDelegate onReturnedDelegate;
	onReturnedDelegate.BindUFunction(this, TEXT("OnReturned"));
	hook->OnReturned.Add(onReturnedDelegate);

	FScriptDelegate onPulledDelegate;
	onPulledDelegate.BindUFunction(this, TEXT("OnGrapplePull"));
	hook->OnPull.Add(onPulledDelegate);

	FScriptDelegate onLatchedDelegate;
	onLatchedDelegate.BindUFunction(this, TEXT("OnLatched"));
	hook->OnLatched.Add(onLatchedDelegate);

	FScriptDelegate onUnLatchedDelegate;
	onUnLatchedDelegate.BindUFunction(this, TEXT("OnUnLatched"));
	hook->OnUnLatched.Add(onUnLatchedDelegate);
}

void UVertCharacterMovementComponent::OnGrapplePull_Implementation(const FVector& direction, const float force)
{
	//Launch(direction*force);
}

void UVertCharacterMovementComponent::OnHooked_Implementation()
{
	mIsGrappling = true;
}

void UVertCharacterMovementComponent::OnFired_Implementation()
{
}

void UVertCharacterMovementComponent::OnReturned_Implementation()
{
	mIsGrappling = false;
}

void UVertCharacterMovementComponent::OnLatched_Implementation(AGrappleHook* hook)
{
	if (!mIsGrappleLatched)
	{
		mIsGrappleLatched = true;
		GravityScale = 0.f;
		Velocity = FVector::ZeroVector;

		SnapCharacterToHook(hook);
	}	
}

void UVertCharacterMovementComponent::OnUnLatched_Implementation(AGrappleHook* hook)
{
	if (mIsGrappleLatched)
	{
		mIsGrappleLatched = false;
		EnableGravity();

		UnSnapCharacterFromHook(hook);
	}	
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
	mSavedGravityScale = GravityScale;
	mSavedGroundFriction = GroundFriction;
}

void UVertCharacterMovementComponent::SnapCharacterToHook(AGrappleHook* hook)
{
	if (AVertCharacter* character = Cast<AVertCharacter>(CharacterOwner))
	{
		//FTransform socketTrans = character->GetSprite()->GetSocketTransform(character->GrappleHandSocket) * character->GetTransform().Inverse();
		character->AttachToActor(hook, FAttachmentTransformRules::KeepWorldTransform);
		//character->SetActorRelativeLocation(socketTrans.GetLocation());
	}	
}

void UVertCharacterMovementComponent::UnSnapCharacterFromHook(AGrappleHook* hook)
{
	if (CharacterOwner)
	{
		CharacterOwner->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	}
}