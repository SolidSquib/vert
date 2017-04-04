// Copyright Inside Out Games Ltd. 2017

#pragma once

#include "Components/ActorComponent.h"
#include "BaseCharacterState.generated.h"

UENUM(BlueprintType, meta = (BitFlags))
enum class ECharacterStatePermissions : uint8
{
	CanJump,
	CanDash,
	CanMove,
	CanGrapple,
};
ENUM_CLASS_FLAGS(ECharacterStatePermissions)

UENUM(BlueprintType, meta = (BitFlags))
enum class ECharacterState : uint8
{
	Idle,
	Walk,
	Jump,
	Fall,
	GrappleShoot,
	Dash
};
ENUM_CLASS_FLAGS(ECharacterState)

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class VERT_API UBaseCharacterState : public UActorComponent
{
	GENERATED_BODY()

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "State|Animation")
	class UPaperFlipbook* StateAnimation;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "State")
	ECharacterState StateSlot;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "State|Permissions", meta = (Bitmask, BitmaskEnum = "ECharacterStatePermissions"))
	int32 Permissions;

public:	
	// Sets default values for this component's properties
	UBaseCharacterState();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	UFUNCTION(BlueprintImplementableEvent, Category = "CharacterStates|Events")
	void OnStateBegin();

	UFUNCTION(BlueprintImplementableEvent, Category = "CharacterStates|Events")
	void OnStateEnd();

	UFUNCTION(BlueprintImplementableEvent, Category = "CharacterStates|Events")
	void OnStateTick(float deltaTime);

public:	
	virtual void StateTick(float deltaTime);
	virtual bool HasPermission(int32 action);
	virtual ECharacterState GetCharacterState() const;

protected:
	TWeakObjectPtr<APawn> mPawnOwner = nullptr;
};
