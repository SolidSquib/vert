// Fill out your copyright notice in the Description page of Project Settings.

#include "Vert.h"
#include "VertCharacterMovementComponent.h"

DECLARE_LOG_CATEGORY_CLASS(LogVertCharacterMovement, Log, All);

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

void UVertCharacterMovementComponent::OnGrapplePull_Implementation(AGrappleHook* hook, const FVector& direction, const float lineLength)
{
	const float k = 10000.f;
	const float b = 1000.f;

	// calculate centripetal force: Fc = M * V^2 / d
	//float magnitudeC = Mass * Velocity.SizeSquared() / lineLength;
	//FVector forceC = magnitudeC * direction;

	// calculate hookes law: F = -kx -bv
	FVector diff = GetOwner()->GetActorLocation() - hook->GetActorLocation();
	float actualLength = diff.Size();
	float magnitudeH = -k * (actualLength-lineLength) - b * Velocity.Size();
	FVector forceH = magnitudeH * -direction;

	DrawDebugLine(GetWorld(), CharacterOwner->GetActorLocation(), CharacterOwner->GetActorLocation() + Velocity, FColor::Red, false, -1.f, 0, 5.f);
	//DrawDebugLine(GetWorld(), CharacterOwner->GetActorLocation(), CharacterOwner->GetActorLocation() + forceC, FColor::Green, false, -1.f, 1, 5.f);
	DrawDebugLine(GetWorld(), CharacterOwner->GetActorLocation(), CharacterOwner->GetActorLocation() + forceH, FColor::Blue, false, -1.f, 2, 5.f);

	//AddForce(forceC);
	AddForce(forceH);
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
	Super::BeginPlay();

	mSavedGravityScale = GravityScale;
	mSavedGroundFriction = GroundFriction;

	if (AVertCharacter* character = Cast<AVertCharacter>(CharacterOwner))
	{
		mCharacterOwner = character;
		mDashingComponent = character->GetDashingComponent();
		mGrapplingComponent = character->GetGrapplingComponent();

		check(mGrapplingComponent.IsValid() && mDashingComponent.IsValid());
	} else { UE_LOG(LogVertCharacterMovement, Error, TEXT("[%s] unable to find AVertCharacter owner.")); }
}

void UVertCharacterMovementComponent::SnapCharacterToHook(AGrappleHook* hook)
{
	if (AVertCharacter* character = Cast<AVertCharacter>(CharacterOwner))
	{
		character->AttachToActor(hook, FAttachmentTransformRules::KeepWorldTransform);
	}	
}

void UVertCharacterMovementComponent::UnSnapCharacterFromHook(AGrappleHook* hook)
{
	if (CharacterOwner)
	{
		CharacterOwner->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	}
}