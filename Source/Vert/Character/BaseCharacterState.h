// Copyright Inside Out Games Ltd. 2017

#pragma once

#include "Components/ActorComponent.h"
#include "BaseCharacterState.generated.h"

UENUM(BlueprintType, meta = (BitFlags))
enum class ECharacterStatePermissions : uint8
{
	NONE,
	CanJump,
	CanDash,
	CanMove,
	CanGrapple,
	CanInteract,
	CanTurn
};
ENUM_CLASS_FLAGS(ECharacterStatePermissions)

UENUM(BlueprintType, meta = (BitFlags))
enum class ECharacterState : uint8
{
	NONE,
	Idle,
	Walk,
	Jump,
	Fall,
	GrappleShoot,
	Dash
};
ENUM_CLASS_FLAGS(ECharacterState)

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnStateEndDelegate, UBaseCharacterState*, exitingState, ECharacterState, enteringState);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class VERT_API UBaseCharacterState : public UActorComponent
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable, Category = "State|Transitions")
	FOnStateEndDelegate OnStateExit;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "State|Animation")
	class UPaperFlipbook* StateAnimation;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "State")
	ECharacterState StateSlot;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "State|Permissions", meta = (Bitmask, BitmaskEnum = "ECharacterStatePermissions"))
	int32 Permissions;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "State|Transitions", meta = (Bitmask, BitmaskEnum = "ECharacterState"))
	int32 Transitions;

public:	
	// Sets default values for this component's properties
	UBaseCharacterState();

	bool CanChangeState(ECharacterState newState);
	void StateBegin();

	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction) override;

	bool HasPermission(ECharacterStatePermissions action) const;
	FORCEINLINE ECharacterState GetCharacterState() const { return StateSlot; }

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	UFUNCTION(BlueprintImplementableEvent, Category = "CharacterStates|Events")
	void OnStateBegin();

	UFUNCTION(BlueprintImplementableEvent, Category = "CharacterStates|Events")
	void OnStateEnd();

	UFUNCTION(BlueprintCallable, Category = "CharacterStates")
	void ChangeState(ECharacterState newState);

protected:
	TWeakObjectPtr<AVertCharacter> mCharacterOwner = nullptr;
};
