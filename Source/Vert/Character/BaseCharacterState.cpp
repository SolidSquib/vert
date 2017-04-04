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

	// ...
}


// Called when the game starts
void UBaseCharacterState::BeginPlay()
{
	Super::BeginPlay();

	if (APawn* character = Cast<APawn>(GetOwner()))
	{
		mPawnOwner = character;
	}
	else
		UE_LOG(LogCharacterState, Error, TEXT("Character state [%s] not attached to a valid character!"), *GetName());
}


// Called every frame
void UBaseCharacterState::StateTick(float DeltaTime)
{
	OnStateTick(DeltaTime); // Call OnStateTick blueprint event
}

bool UBaseCharacterState::HasPermission(int32 flags)
{
	return (Permissions & flags) == flags;
}

ECharacterState UBaseCharacterState::GetCharacterState() const
{
	return StateSlot;
}