// Copyright Inside Out Games Ltd. 2017

#include "Vert.h"
#include "BaseCharacterState.h"

DECLARE_LOG_CATEGORY_CLASS(LogCharacterState, Log, All);

// Sets default values for this component's properties
UBaseCharacterState::UBaseCharacterState()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UBaseCharacterState::BeginPlay()
{
	Super::BeginPlay();

	if (AVertCharacter* character = Cast<AVertCharacter>(GetOwner()))
	{
		mCharacterOwner = character;
	}
	else
		UE_LOG(LogCharacterState, Error, TEXT("Character state [%s] not attached to a valid character!"), *GetName());

	if (UCharacterStateManager* stateMan = Cast<UCharacterStateManager>(GetOwner()->GetComponentByClass(UCharacterStateManager::StaticClass())))
	{
		mStateMan = stateMan;
	}
	else
		UE_LOG(LogCharacterState, Error, TEXT("Actor [%s] contains state [%s] but no state manager."), *GetOwner()->GetName(), *GetName());

	

	UE_LOG(LogCharacterState, Log, TEXT("State [%s->%s] initialised."), *GetOwner()->GetName(), *GetName());
}

void UBaseCharacterState::StateBegin()
{
	UE_LOG(LogCharacterState, Log, TEXT("State begin for state %s"), *GetName());

	mCharacterOwner->GetSprite()->SetFlipbook(StateAnimation);
	mStateChangeQueued = false;
	OnStateBegin();
}

// Not called if overridden in blueprint.
bool UBaseCharacterState::OnNotifyActionTaken(ECharacterActions action)
{	
	if (HasPermission(action))
	{
		switch (action)
		{
		case ECharacterActions::Jump:
			return TakeActionJump();
		case ECharacterActions::Dash:
			return TakeActionDash();
		case ECharacterActions::Move:
			return TakeActionMove();
		case ECharacterActions::Grapple:
			return TakeActionGrapple();
		case ECharacterActions::Interact:
			return TakeActionInteract();
		case ECharacterActions::Attack:
			return TakeActionAttack();
		case ECharacterActions::Block:
			return TakeActionBlock();
		default:
			UE_LOG(LogCharacterState, Error, TEXT("Action [%s] unaccounted for."), *UVertUtilities::GetEnumValueToString<ECharacterActions>(TEXT("ECharacterActions"), action));
			break;
		}
	}

	return false;
}

bool UBaseCharacterState::TakeActionJump_Implementation()
{
	if (mCharacterOwner.IsValid())
	{
		ChangeState(ECharacterState::Jump);
		return true;
	}

	return false;
}

bool UBaseCharacterState::TakeActionDash_Implementation()
{
	if (mCharacterOwner.IsValid() && mCharacterOwner->GetDashingComponent())
	{
		if (mCharacterOwner->GetDashingComponent()->ExecuteGroundDash())
		{
			ChangeState(ECharacterState::Dash);
			return true;
		}
	}
	
	return false;
}

bool UBaseCharacterState::TakeActionMove_Implementation()
{
	if (mCharacterOwner.IsValid())
	{
		float value = mCharacterOwner->GetAxisPostisions().GetPlayerLeftThumbstick2D().X;

		if (value > 0)
			value = 1.f;
		else if (value < 0)
			value = -1.f;

		// Apply the input to the character motion
		mCharacterOwner->AddMovementInput(FVector(1.0f, 0.0f, 0.0f), value);
		//ChangeState(ECharacterState::Walk);
		return true;
	}

	return false;
}

bool UBaseCharacterState::TakeActionGrapple_Implementation()
{
	if (mCharacterOwner.IsValid() && mCharacterOwner->GetGrapplingComponent())
	{
		if (mCharacterOwner->GetGrapplingComponent()->ExecuteGrapple(mCharacterOwner->UsingGamepad() ? mCharacterOwner->GetAxisPostisions().GetPlayerRightThumbstickDirection() : mCharacterOwner->GetAxisPostisions().GetPlayerMouseDirection()))
		{
			ChangeState(ECharacterState::GrappleShoot);
			return true;
		}
	}

	return false;
}

bool UBaseCharacterState::TakeActionInteract_Implementation()
{
	if (mCharacterOwner.IsValid() && mCharacterOwner->GetInteractionComponent())
	{
		if (mCharacterOwner->GetInteractionComponent()->AttemptInteract())
			return true;
	}

	return false;
}

bool UBaseCharacterState::TakeActionAttack_Implementation()
{
	if (mCharacterOwner.IsValid() && mCharacterOwner->GetInteractionComponent())
	{
		return mCharacterOwner->GetInteractionComponent()->AttemptAttack();
	}

	return false;
}

bool UBaseCharacterState::TakeActionBlock_Implementation()
{
	return false;
}

bool UBaseCharacterState::ChangeState(ECharacterState newState)
{
	if (newState != StateSlot && !mStateChangeQueued && mStateMan->HasValidCharacterState(newState))
	{
		OnStateEnd();
		OnStateExit.Broadcast(this, newState);
		mStateChangeQueued = true;

		return true;
	}
	else
	{
		// UE_LOG(LogCharacterState, Warning, TEXT("State [%s] attempted to change to state [%s] but was unable; check your logic."), *GetName(), *UVertUtilities::GetEnumValueToString(TEXT("ECharacterState"), newState));

		if (!mStateMan->HasValidCharacterState(newState))
		{
			UE_LOG(LogCharacterState, Warning, TEXT("State [%s]->[%s] attempted to change to state [%s] but no state of that type found on owning actor."), *GetOwner()->GetName(), *GetName(), *UVertUtilities::GetEnumValueToString<ECharacterState>(TEXT("ECharacterState"), newState));
		}
	}

	return false;
}

bool UBaseCharacterState::HasPermission(ECharacterActions action) const
{
	int32 bitFlag = static_cast<int32>(1 << (int32)action);
	return (Permissions & bitFlag) == bitFlag;
}