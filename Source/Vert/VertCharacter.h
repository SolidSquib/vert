// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Character/VertCharacterBase.h"
#include "Character/VertCharacterMovementComponent.h"
#include "Character/CharacterInteractionComponent.h"
#include "Character/HealthComponent.h"
#include "Character/GrapplingComponent.h"
#include "Character/DashingComponent.h"
#include "Character/LedgeGrabbingComponent.h"
#include "Engine/AxisPositions.h"
#include "VertCharacter.generated.h"

// This class is the default character for Vert, and it is responsible for all
// physical interaction between the player and the world.
//
//   The capsule component (inherited from ACharacter) handles collision with the world
//   The CharacterMovementComponent (inherited from ACharacter) handles movement of the collision capsule
//   The Sprite component (inherited from APaperCharacter) handles the visuals

DECLARE_LOG_CATEGORY_EXTERN(LogVertCharacter, Log, All);

UCLASS(config = Game, Abstract)
class AVertCharacter : public AVertCharacterBase
{
	GENERATED_BODY()

	friend class AVertPlayerController;

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Items")
	FName ItemHandSocket = "ItemSocket";

protected:
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadWrite, Category = "Items")
	UCharacterInteractionComponent* InteractionComponent;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Grappling")
	UGrapplingComponent* GrapplingComponent;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Dashing")
	UDashingComponent* DashingComponent;
	
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Climbing")
	ULedgeGrabbingComponent* ClimbingComponent;

	UPROPERTY(VisibleDefaultsOnly, Category = "UserInterface")
	class UAimArrowWidgetComponent* AimArrowComponent;

	UPROPERTY(VisibleDefaultsOnly, Category = "UserInterface")
	USceneComponent* AimArrowAnchor;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class UCameraComponent* SideViewCameraComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class USpringArmComponent* CameraBoom;

	/** material instances for setting team color in mesh */
	UPROPERTY(Transient)
	TArray<UMaterialInstanceDynamic*> MeshMIDs;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Controls)
	bool DisableInputWhenDashingOrGrappling;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Controls")
	int32 GamepadSwipeRequiredPasses = 3;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
	class UAkAudioEvent* JumpSound = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
	UAkAudioEvent* JumpLandSound = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
	UAkAudioEvent* DoubleJumpSound = nullptr;

	UPROPERTY()
	AVertPlayerController* VertController = nullptr;

public:
	AVertCharacter(const class FObjectInitializer& ObjectInitializer);

	void UpdateTeamColoursAllMIDs();
	const bool UsingGamepad() const;
	TArray<AController*> GetRecentHitters();

	virtual void BeginPlay() override;
	virtual void PostInitializeComponents() override;
	virtual void PossessedBy(AController* NewController) override;
	virtual void Landed(const FHitResult& Hit) override;
	virtual void OnRep_PlayerState() override; /** [client] perform PlayerState related setup */
	virtual bool Die() override;
	virtual float TakeDamage(float Damage, const FDamageEvent& DamageEvent, class AController* EventInstigator, class AActor* DamageCauser) override;

	FORCEINLINE class UCameraComponent* GetSideViewCameraComponent() const { return SideViewCameraComponent; }
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	FORCEINLINE UHealthComponent* GetHealthComponent() const { return HealthComponent; }
	FORCEINLINE UCharacterInteractionComponent* GetInteractionComponent() const { return InteractionComponent; }
	FORCEINLINE UGrapplingComponent* GetGrapplingComponent() const { return GrapplingComponent; }
	FORCEINLINE UDashingComponent* GetDashingComponent() const { return DashingComponent; }
	FORCEINLINE const FAxisPositions& GetAxisPostisions() const { return mAxisPositions; }

	UFUNCTION(BlueprintCallable, Category = "Combat")
	void Dislodge(bool dropItem = false);

	UFUNCTION(BlueprintCallable, Category = "Movement")
	float GetStartingMaxMoveSpeed() const { return mStartMovementSpeed; }

	UFUNCTION(BlueprintCallable, Category = "Combat")
	void StopAttacking();

	UFUNCTION(BlueprintCallable, Category = "Weapons")
	bool CanFire() const;

	UFUNCTION(BlueprintCallable, Category = "Weapons")
	bool CanReload() const;

	UFUNCTION(BlueprintCallable, Category = "Grappling")
	bool CanGrapple() const;

	UFUNCTION(BlueprintCallable, Category = "Dashing")
	bool CanDash() const;

	UFUNCTION(BlueprintCallable, Category = "Movement")
	bool CanMove() const;

	UFUNCTION(BlueprintCallable, Category = "Jump")
	bool CanWallJump() const;

	UFUNCTION(BlueprintCallable, Category = "Jump")
	bool CanAttack() const;

	UFUNCTION(BlueprintCallable, Category = "Jump")
	bool CanInteract() const;

	UFUNCTION(BlueprintCallable, Category = "Grappling")
	bool IsGrappling() const { return GrapplingComponent->GetGrappleState() == EGrappleState::HookDeployed || GrapplingComponent->GetGrappleState() == EGrappleState::HookDeployedAndReturning; }

	UFUNCTION(BlueprintCallable, Category = Climbing)
	bool IsClimbing() const { return ClimbingComponent->IsClimbingLedge(); }

	UFUNCTION(BlueprintCallable, Category = CharacterMovement)
	FORCEINLINE UVertCharacterMovementComponent* GetVertCharacterMovement() const { if (UVertCharacterMovementComponent* movement = Cast<UVertCharacterMovementComponent>(GetCharacterMovement())) { return movement; } return nullptr; }

	UFUNCTION(BlueprintCallable, Category = "Climbing")
	FORCEINLINE ULedgeGrabbingComponent* GetClimbingComponent() const { return ClimbingComponent; }

	UFUNCTION(BlueprintCallable, Category = "Ragdoll")
	FORCEINLINE bool IsRagdolling() const { return mIsRagdolling; }

	UFUNCTION(BlueprintCallable, Category = "Weapons")
	class ABaseWeapon* GetCurrentWeapon() const;

	UFUNCTION(BlueprintCallable, Category = "Weapons")
	ABaseWeapon* GetDefaultWeapon() const;

	UFUNCTION(BlueprintCallable, Category = "Weapons")
	ABaseWeapon* GetHeldWeapon() const;

protected:
	void UpdateCharacter();
	void ActionMoveRight(float Value);
	void ActionGrappleShoot();
	void ActionGrappleShootAltDown(); // Alternative function for gamepad users
	void ActionGrappleShootAltUp(); // Alternative function for gamepad users
	void ExecuteActionGrappleShoot();
	void ActionDash();
	void ActionInteract();
	void ActionAttack();
	void ActionJump();
	void ActionDropDown();
	void RightThumbstickMoveX(float value);
	void RightThumbstickMoveY(float value);
	void LeftThumbstickMoveY(float value);
	void MouseMove(float value);
	void UpdateTeamColours(UMaterialInstanceDynamic* useMIDs);

	virtual bool CanJumpInternal_Implementation() const override;
	virtual void OnJumped_Implementation() override;
	virtual void Jump() override;
	virtual void SetupPlayerInputComponent(class UInputComponent* InputComponent) override;
	virtual void ApplyDamageHitstun(float hitstunTime) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	
	UFUNCTION(BlueprintCallable, Category = "Player Controller")
	FORCEINLINE AVertPlayerController* GetPlayerController() const { if (AController* controller = GetController()) { if (AVertPlayerController* playerController = Cast<AVertPlayerController>(controller)) { return playerController; } } return nullptr; }

	UFUNCTION()
	void OnPickupInteractiveInternal(AInteractive* interactive, bool wasCaught);

	UFUNCTION()
	void OnDropInteractiveInternal(AInteractive* interactive, bool wasThrown);

	UFUNCTION()
	void AttemptToGrabGrappledLedge(const FHitResult& forwardHit, const FHitResult& downwardHit);

	UFUNCTION()
	void GrappleDetached();
	
	UFUNCTION()
	void GrappleHooked();

	UFUNCTION()
	void GrappleReturned();

	UFUNCTION()
	void PerformLedgeAttack();

	UFUNCTION()
	void OnCharacterHit(UPrimitiveComponent* hitComponent, AActor* otherActor, UPrimitiveComponent* otherComp, FVector normalImpulse, const FHitResult& hit);

	/// Blueprint Implementable functions //////////////////////////////////////////////////////////////////////////
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Actions")
	void Character_OnGrappleShootExecuted(const FVector& direction);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Actions")
	void Character_OnGrapplePullExecuted(const FVector& direction);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Actions")
	void Character_OnGrappleBeginReturn();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Actions")
	void Character_OnDashExecuted(const FVector& direction, bool isGrappled);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Actions")
	void Character_OnDashEnded();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Actions")
	void Character_OnInteractExecuted(AInteractive* interactive);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Actions")
	void Character_OnPickupNewInteractive(AInteractive* interactive, bool wasCaught);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Actions")
	void Character_OnDropCurrentInteractive(AInteractive* interactive, bool wasThrown);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Actions")
	void Character_OnStartAttackExecuted(ABaseWeapon* weapon);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Actions")
	void Character_OnStartDashAttackExecuted(ABaseWeapon* weapon);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Actions")
	void Character_OnStopAttackExecuted();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Actions")
	void Character_OnJumpExecuted();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Actions")
	void Character_OnAirJumpExecuted();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Actions")
	void Character_OnWeaponStateChangeExecuted(ABaseWeapon* weapon, EWeaponState newState, UAnimSequence* playerAnim);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Actions")
	void Character_OnAttackFired(const struct FWeaponAnim& playAnim, float recoilAmount);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Actions")
	void Character_OnLedgeTransition(ELedgeTransition transition);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Actions")
	void Character_OnLedgeGrabbed(const FHitResult& forwardHit, const FHitResult& downwardHit);

	UFUNCTION()
	void OnLedgeGrabbed(const FHitResult& forwardHit, const FHitResult& downwardHit);

	FORCEINLINE void EndGamepadStandby() { mGamepadOnStandby = false; }

	UFUNCTION()
	void RecentHitExpired(AController* attacker);

private:
	void RecordRecentHit(AController* attacker);

protected:
	TWeakObjectPtr<ABaseWeapon> mCurrentWeapon = nullptr;
	FAxisPositions mAxisPositions;
	FTimerHandle mTimerHandle_GrappleExecute;
	FTimerHandle mTimerHandle_GamepadGrappleDelay;
	FTimerHandle mTimerHandle_AutoAttack;
	bool mGamepadOnStandby = false;
	float mStartMovementSpeed = 0;

	FScriptDelegate mOnWeaponStateChangedDelegate;
	FScriptDelegate mOnWeaponFiredDelegate;

	/* Gamepad grapple bits and bobs */
	bool mGamepadWantsToGrapple = false;
	bool mJumpedFromGrapple = false;
	FVector mGamepadSwipeDirection = FVector::ZeroVector;
	int32 mGamepadSwipePassesDone = 0;
	TArray<FVector> mSwipePasses;

	TMap<AController*, FTimerHandle> mRecentHitters;
};
