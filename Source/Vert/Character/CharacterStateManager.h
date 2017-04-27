// Copyright Inside Out Games Ltd. 2017

#pragma once

#include "Components/ActorComponent.h"
#include "Character/BaseCharacterState.h"
#include "CharacterStateManager.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnStateChangedDelegate, ECharacterState, exitingState, ECharacterState, enteringState);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class VERT_API UCharacterStateManager : public UActorComponent
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable, Category = "CharacterStates|Events")
	FOnStateChangedDelegate OnStateChanged;

public:	
	// Sets default values for this component's properties
	UCharacterStateManager();

	bool HasValidCharacterState(ECharacterState state);

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintCallable, Category = "CharacterStates")
	void ForceStateChange(ECharacterState newState);

	UFUNCTION(BlueprintCallable, Category = "CharacterStates|Action")
	bool NotifyActionTaken(ECharacterActions action);

	UFUNCTION(BlueprintNativeEvent, Category = "CharacterStates")
	void OnCurrentStateEnded(class UBaseCharacterState* exitingState, ECharacterState enteringState);

	UFUNCTION(BlueprintCallable, Category = "CharacterStates")
	FORCEINLINE ECharacterState GetCurrentState() const { return mActiveState; }

protected:
	// Called when the game starts
	virtual void BeginPlay() override;
	
private:
	TMap<ECharacterState, UBaseCharacterState*> mCharacterStates;
	ECharacterState mActiveState = ECharacterState::Idle;
};
