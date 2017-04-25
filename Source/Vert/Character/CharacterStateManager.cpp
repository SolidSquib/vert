// Copyright Inside Out Games Ltd. 2017

#include "Vert.h"
#include "CharacterStateManager.h"

DECLARE_LOG_CATEGORY_CLASS(LogCharacterStateManager, Log, All);

// Sets default values for this component's properties
UCharacterStateManager::UCharacterStateManager()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
}

// Called when the game starts
void UCharacterStateManager::BeginPlay()
{
	Super::BeginPlay();

	TInlineComponentArray<UBaseCharacterState*> components;
	GetOwner()->GetComponents<UBaseCharacterState>(components);
	for (auto* component : components)
	{
		FScriptDelegate onStateEndDelegates;
		onStateEndDelegates.BindUFunction(this, TEXT("OnCurrentStateEnded"));
		component->OnStateExit.Add(onStateEndDelegates);

		component->GetCharacterState() == ECharacterState::Idle ? component->SetComponentTickEnabled(true) : component->SetComponentTickEnabled(false);
		component->GetCharacterState() == ECharacterState::Idle ? component->Activate() : component->Deactivate();
		mCharacterStates.Add(component->GetCharacterState(), component);
		UE_LOG(LogCharacterStateManager, Log, TEXT("State [%s] added to character [%s]"), *component->GetName(), *GetOwner()->GetName());
	}
	if (!mCharacterStates.Find(ECharacterState::Idle))
	{
		UE_LOG(LogCharacterStateManager, Fatal, TEXT("No idle animation state found for characer [%s]"), *GetOwner()->GetName());
	}
}

// Called every frame
void UCharacterStateManager::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

void UCharacterStateManager::ForceStateChange(ECharacterState newState)
{
	if (mCharacterStates.Find(newState))
	{
		mCharacterStates[mActiveState]->SetComponentTickEnabled(false);
		mCharacterStates[newState]->SetComponentTickEnabled(true);
		OnStateChanged.Broadcast(mActiveState, newState);
		mActiveState = newState;

		mCharacterStates[mActiveState]->StateBegin();
	}
}

bool UCharacterStateManager::HasValidCharacterState(ECharacterState state)
{
	return mCharacterStates.Find(state) != nullptr;
}

bool UCharacterStateManager::NotifyActionTaken(ECharacterActions action)
{
	return mCharacterStates[mActiveState]->OnNotifyActionTaken(action);
}

void UCharacterStateManager::OnCurrentStateEnded_Implementation(UBaseCharacterState* exitingState, ECharacterState enteringState)
{
	if (exitingState != mCharacterStates[mActiveState])
	{
		UE_LOG(LogCharacterStateManager, Error, TEXT("Character [%s] can't change from inactive state [%s], actual active state is [%s]."), *GetOwner()->GetName(), *UVertUtilities::GetEnumValueToString<ECharacterState>(TEXT("ECharacterState"), exitingState->GetCharacterState()), *UVertUtilities::GetEnumValueToString<ECharacterState>(TEXT("ECharacterState"), mActiveState));
	}

	if (mCharacterStates.Find(enteringState))
	{
		exitingState->SetComponentTickEnabled(false);
		exitingState->Deactivate();
		mCharacterStates[enteringState]->Activate();
		mCharacterStates[enteringState]->SetComponentTickEnabled(true);
		OnStateChanged.Broadcast(mActiveState, enteringState);
		mActiveState = enteringState;

		mCharacterStates[mActiveState]->StateBegin();
	}
	else
	{
		UE_LOG(LogCharacterStateManager, Error, TEXT("Character [%s] attemted to change to state [%s] when it doesn't exist"), *GetOwner()->GetName(), *UVertUtilities::GetEnumValueToString<ECharacterState>(TEXT("ECharacterState"), enteringState));
	}
}