// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "GameFramework/Character.h"
#include "Engine/DebugGroup.h"
#include "Character/VertCharacterMovementComponent.h"
#include "Character/CharacterInteractionComponent.h"
#include "Character/HealthComponent.h"
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
class AVertCharacter : public ACharacter
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
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class UCameraComponent* SideViewCameraComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class USpringArmComponent* CameraBoom;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Controls)
	bool DisableInputWhenDashingOrGrappling;

public:
	AVertCharacter(const class FObjectInitializer& ObjectInitializer);

	bool CanComponentRecharge(ERechargeRule rule);
	bool IsMoving();
	const bool UsingGamepad() const;

	virtual void Tick(float DeltaSeconds) override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type endPlayReason) override;
	virtual void Landed(const FHitResult& Hit) override;
	virtual void ApplyDamageMomentum(float DamageTaken, FDamageEvent const& DamageEvent, APawn* PawnInstigator, AActor* DamageCauser) override;

	FORCEINLINE class UCameraComponent* GetSideViewCameraComponent() const { return SideViewCameraComponent; }
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	FORCEINLINE UHealthComponent* GetHealthComponent() const { return HealthComponent; }
	FORCEINLINE UCharacterInteractionComponent* GetInteractionComponent() const { return InteractionComponent; }
	FORCEINLINE UGrapplingComponent* GetGrapplingComponent() const { return GrapplingComponent; }
	FORCEINLINE UDashingComponent* GetDashingComponent() const { return DashingComponent; }
	FORCEINLINE const FAxisPositions& GetAxisPostisions() const { return mAxisPositions; }
	
	UFUNCTION(BlueprintCallable, Category = "Health")
	virtual float TakeDamage(float Damage, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, class AActor* DamageCauser) override;

	UFUNCTION(BlueprintCallable, Category = "Combat")
	void StopAttacking();

	UFUNCTION(BlueprintCallable, Category = CharacterMovement)
	FORCEINLINE UVertCharacterMovementComponent* GetVertCharacterMovement() const { if (UVertCharacterMovementComponent* movement = Cast<UVertCharacterMovementComponent>(GetCharacterMovement())) { return movement; } return nullptr; }

	UFUNCTION(BlueprintCallable, Category = "CharacterMovement")
	FORCEINLINE bool IsGrounded() const { return !GetCharacterMovement()->IsFlying() && !GetCharacterMovement()->IsFalling(); }

	UFUNCTION(BlueprintCallable, Category = "Weapons")
	FORCEINLINE class ABaseWeapon* GetWeapon() const { return InteractionComponent->GetHeldWeapon(); }

	UFUNCTION(BlueprintCallable, Category = "Weapons")
	bool CanFire() const;

	UFUNCTION(BlueprintCallable, Category = "Weapons")
	bool CanReload() const;

	UFUNCTION(BlueprintImplementableEvent, Category = "Events")
	void NotifyJumped();

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

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Character|Health")
	void OnNotifyDeath(const FTakeHitInfo& lastHit);

	UFUNCTION(BlueprintCallable, Category = "Player Controller")
	FORCEINLINE AVertPlayerController* GetPlayerController() const { if (AController* controller = GetController()) { if (AVertPlayerController* playerController = Cast<AVertPlayerController>(controller)) { return playerController; } } return nullptr; }

private:
	void SetRagdollPhysics();

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
