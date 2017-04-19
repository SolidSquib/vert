// Copyright Inside Out Games Ltd. 2017

#pragma once

#include "Components/ActorComponent.h"
#include "BaseCharacterState.generated.h"

UENUM(BlueprintType, meta = (BitFlags))
enum class ECharacterActions : uint8
{
	NONE,
	Jump,
	Dash,
	Move,
	Grapple,
	Interact,
	Block
};
ENUM_CLASS_FLAGS(ECharacterActions)

UENUM(BlueprintType, meta = (BitFlags))
enum class ECharacterState : uint8
{
	NONE,
	Idle,
	Walk,
	Jump,
	Fall,
	GrappleShoot,
	GrapplePull,
	GrappleHang,
	Dash,
	BlockingGround,
	BlockingAir
};
ENUM_CLASS_FLAGS(ECharacterState)

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnStateEndDelegate, UBaseCharacterState*, exitingState, ECharacterState, enteringState);

UCLASS(Blueprintable, ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class VERT_API UBaseCharacterState : public UActorComponent
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable, Category = "State|Transitions")
	FOnStateEndDelegate OnStateExit;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CharacterState|Animation")
	class UPaperFlipbook* StateAnimation;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CharacterState")
	ECharacterState StateSlot;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CharacterState|Permissions", meta = (Bitmask, BitmaskEnum = "ECharacterActions"))
	int32 Permissions;

	// All of the valid state transitions from this state.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CharacterState|Transitions", meta = (Bitmask, BitmaskEnum = "ECharacterState"))
	int32 Transitions;

public:	
	// Sets default values for this component's properties
	UBaseCharacterState();

	void StateBegin();
	
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction) override;

	FORCEINLINE ECharacterState GetCharacterState() const { return StateSlot; }

	UFUNCTION(BlueprintCallable, Category = "CharacterState|Action")
	bool OnNotifyActionTaken(ECharacterActions action);

	UFUNCTION(BlueprintCallable, Category = "CharacterState|Permissions")
	bool CanChangeState(ECharacterState newState);

protected:
	virtual void BeginPlay() override;

	UFUNCTION(BlueprintNativeEvent, Category = "CharacterState|Tick")
	void OnStateTick(float DeltaTime);

	UFUNCTION(BlueprintNativeEvent, Category = "CharacterState|Action")
	bool TakeActionJump();

	UFUNCTION(BlueprintNativeEvent, Category = "CharacterState|Action")
	bool TakeActionDash();

	UFUNCTION(BlueprintNativeEvent, Category = "CharacterState|Action")
	bool TakeActionMove();

	UFUNCTION(BlueprintNativeEvent, Category = "CharacterState|Action")
	bool TakeActionGrapple();

	UFUNCTION(BlueprintNativeEvent, Category = "CharacterState|Action")
	bool TakeActionInteract();

	UFUNCTION(BlueprintNativeEvent, Category = "CharacterState|Action")
	bool TakeActionBlock();

	UFUNCTION(BlueprintImplementableEvent, Category = "CharacterState|Events")
	void OnStateBegin();

	UFUNCTION(BlueprintImplementableEvent, Category = "CharacterState|Events")
	void OnStateEnd();

	UFUNCTION(BlueprintCallable, Category = "CharacterState")
	void ChangeState(ECharacterState newState);

	UFUNCTION(BlueprintCallable, Category = "CharacterState|Owner")
	AVertCharacter* GetCharacterOwner() const { return mCharacterOwner.IsValid() ? mCharacterOwner.Get() : nullptr; }

private:
	bool HasPermission(ECharacterActions action) const;

protected:
	TWeakObjectPtr<AVertCharacter> mCharacterOwner = nullptr;
	TWeakObjectPtr<class UCharacterStateManager> mStateMan = nullptr;
	bool mStateChangeQueued = false;
};
