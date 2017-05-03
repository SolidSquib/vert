// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "PaperCharacter.h"
#include "Engine/DebugGroup.h"
#include "Character/VertCharacterMovementComponent.h"
#include "Character/CharacterInteractionComponent.h"
#include "Character/HealthComponent.h"
#include "Character/CharacterStateManager.h"
#include "Character/GrapplingComponent.h"
#include "Character/DashingComponent.h"
#include "Engine/AxisPositions.h"
#include "VertCharacter.generated.h"

// This class is the default character for Vert, and it is responsible for all
// physical interaction between the player and the world.
//
//   The capsule component (inherited from ACharacter) handles collision with the world
//   The CharacterMovementComponent (inherited from ACharacter) handles movement of the collision capsule
//   The Sprite component (inherited from APaperCharacter) handles the visuals

DECLARE_LOG_CATEGORY_EXTERN(LogVertCharacter, Log, All);

USTRUCT()
struct FCharacterDebugSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = Debug)
	FDebugGroup Grapple;

	UPROPERTY(EditAnywhere, Category = Debug)
	FDebugGroup Dash;

	UPROPERTY(EditAnywhere, Category = Debug)
	FDebugGroup CharacterMovement;

	UPROPERTY(EditAnywhere, Category = Debug)
	FDebugGroup States;

	UPROPERTY(EditAnywhere, Category = Debug)
	bool InfiniteDashGrapple;

	FCharacterDebugSettings()
	{
		Grapple.MessageColour = FColor::Emerald;
		Dash.MessageColour = FColor::Blue;
		CharacterMovement.MessageColour = FColor::Red;
		InfiniteDashGrapple = false;
	}
};

UCLASS(config = Game, Abstract)
class AVertCharacter : public APaperCharacter
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Debug)
	FCharacterDebugSettings ShowDebug;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character|Interact")
	FName ItemHandSocket = "ItemSocket";

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character|Health")
	UHealthComponent* HealthComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Character|Interact")
	UCharacterInteractionComponent* InteractionComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character|Grappling")
	UGrapplingComponent* GrapplingComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character|Dashing")
	UDashingComponent* DashingComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character|States")
	UCharacterStateManager* StateManager;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class UCameraComponent* SideViewCameraComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class USpringArmComponent* CameraBoom;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Controls)
	bool DisableInputWhenDashingOrGrappling;

public:
	AVertCharacter(const class FObjectInitializer& ObjectInitializer);

	bool CanComponentRecharge(ERechargeRule rule);
	const bool UsingGamepad() const;

	virtual void Tick(float DeltaSeconds) override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type endPlayReason) override;
	virtual void Landed(const FHitResult& Hit) override;

	FORCEINLINE class UCameraComponent* GetSideViewCameraComponent() const { return SideViewCameraComponent; }
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	FORCEINLINE UHealthComponent* GetHealthComponent() const { return HealthComponent; }
	FORCEINLINE UCharacterInteractionComponent* GetInteractionComponent() const { return InteractionComponent; }
	FORCEINLINE UGrapplingComponent* GetGrapplingComponent() const { return GrapplingComponent; }
	FORCEINLINE UDashingComponent* GetDashingComponent() const { return DashingComponent; }
	FORCEINLINE UCharacterStateManager* GetStateManager() const { return StateManager; }
	FORCEINLINE const FAxisPositions& GetAxisPostisions() const { return mAxisPositions; }

	UFUNCTION(BlueprintCallable, Category = "Combat")
	void StopAttacking();

	UFUNCTION(BlueprintCallable, Category = CharacterMovement)
	FORCEINLINE UVertCharacterMovementComponent* GetVertCharacterMovement() const { if (UVertCharacterMovementComponent* movement = Cast<UVertCharacterMovementComponent>(GetCharacterMovement())) { return movement; } return nullptr; }

	UFUNCTION(BlueprintCallable, Category = "CharacterMovement")
	FORCEINLINE bool IsGrounded() const { return !GetCharacterMovement()->IsFlying() && !GetCharacterMovement()->IsFalling(); }

protected:
	void ActionMoveRight(float Value);
	void ActionGrappleShoot();
	void ExecuteActionGrappleShoot();
	void ActionDash();
	void ActionInteract();
	void ActionAttack();
	void ActionJump();
	void RightThumbstickMoveX(float value);
	void RightThumbstickMoveY(float value);
	void LeftThumbstickMoveY(float value);
	void MouseMove(float value);	
	void UpdateCharacter();

	virtual void SetupPlayerInputComponent(class UInputComponent* InputComponent) override;

	UFUNCTION(BlueprintCallable, Category = "Player Controller")
	FORCEINLINE AVertPlayerController* GetPlayerController() const { if (AController* controller = GetController()) { if (AVertPlayerController* playerController = Cast<AVertPlayerController>(controller)) { return playerController; } } return nullptr; }

private:
#if !UE_BUILD_SHIPPING
	void PrintDebugInfo();
#endif
	FORCEINLINE void EndGamepadStandby() { mGamepadOnStandby = false; }

protected:
	FAxisPositions mAxisPositions;
	FTimerHandle mTimerHandle;
	FTimerHandle mGamepadGrappleDelay;
	bool mGamepadOnStandby = false;
};
