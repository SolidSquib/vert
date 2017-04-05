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
}

bool UBaseCharacterState::CanChangeState(ECharacterState newState)
{
	int32 flag = static_cast<int32>(newState);
	return (Transitions & flag) == flag;
}

void UBaseCharacterState::StateBegin()
{
	mCharacterOwner->GetSprite()->SetFlipbook(StateAnimation);
	OnStateBegin();
}

void UBaseCharacterState::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

bool UBaseCharacterState::HasPermission(ECharacterStatePermissions action) const
{
	UE_LOG(LogCharacterState, Warning, TEXT("State [%s] %i"), *UVertUtilities::GetEnumValueToString<ECharacterStatePermissions>(TEXT("ECharacterStatePermissions"), ECharacterStatePermissions::CanJump), (int32)ECharacterStatePermissions::CanJump);
	UE_LOG(LogCharacterState, Warning, TEXT("State [%s] %i"), *UVertUtilities::GetEnumValueToString<ECharacterStatePermissions>(TEXT("ECharacterStatePermissions"), ECharacterStatePermissions::CanDash), (int32)ECharacterStatePermissions::CanDash);
	UE_LOG(LogCharacterState, Warning, TEXT("State [%s] %i"), *UVertUtilities::GetEnumValueToString<ECharacterStatePermissions>(TEXT("ECharacterStatePermissions"), ECharacterStatePermissions::CanMove), (int32)ECharacterStatePermissions::CanMove);
	UE_LOG(LogCharacterState, Warning, TEXT("State [%s] %i"), *UVertUtilities::GetEnumValueToString<ECharacterStatePermissions>(TEXT("ECharacterStatePermissions"), ECharacterStatePermissions::CanGrapple), (int32)ECharacterStatePermissions::CanGrapple);
	UE_LOG(LogCharacterState, Warning, TEXT("State [%s] %i"), *UVertUtilities::GetEnumValueToString<ECharacterStatePermissions>(TEXT("ECharacterStatePermissions"), ECharacterStatePermissions::CanInteract), (int32)ECharacterStatePermissions::CanInteract);
	UE_LOG(LogCharacterState, Warning, TEXT("State [%s] %i"), *UVertUtilities::GetEnumValueToString<ECharacterStatePermissions>(TEXT("ECharacterStatePermissions"), ECharacterStatePermissions::CanTurn), (int32)ECharacterStatePermissions::CanTurn);

	int32 bitFlag = static_cast<int32>(1 << (int32)action);
	return (Permissions & bitFlag) == bitFlag;
}

void UBaseCharacterState::ChangeState(ECharacterState newState)
{
	OnStateExit.Broadcast(this, newState);
	OnStateEnd();
}