// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "GameFramework/Character.h"
#include "Engine/DebugGroup.h"
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
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Character|Health")
	UHealthComponent* HealthComponent;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadWrite, Category = "Character|Interact")
	UCharacterInteractionComponent* InteractionComponent;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Character|Grappling")
	UGrapplingComponent* GrapplingComponent;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Character|Dashing")
	UDashingComponent* DashingComponent;
	
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Character|Climbing")
	ULedgeGrabbingComponent* ClimbingComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class UCameraComponent* SideViewCameraComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class USpringArmComponent* CameraBoom;

	/** material instances for setting team color in mesh (3rd person view) */
	UPROPERTY(Transient)
	TArray<UMaterialInstanceDynamic*> MeshMIDs;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Controls)
	bool DisableInputWhenDashingOrGrappling;
	
public:
	AVertCharacter(const class FObjectInitializer& ObjectInitializer);

	void UpdateTeamColoursAllMIDs();
	bool CanComponentRecharge(ERechargeRule rule);
	bool IsMoving();
	const bool UsingGamepad() const;

	virtual void Tick(float DeltaSeconds) override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type endPlayReason) override;
	virtual void Landed(const FHitResult& Hit) override;
	virtual void ApplyDamageMomentum(float DamageTaken, const FDamageEvent& DamageEvent, APawn* PawnInstigator, AActor* DamageCauser) override;
	virtual bool CanDie(float KillingDamage, const FDamageEvent& DamageEvent, AController* Killer, AActor* DamageCauser) const;
	virtual void OnDeath(float killingDamage, const FDamageEvent& damageEvent, APawn* pawnInstigator, AActor* damageCauser);
	virtual void FellOutOfWorld(const class UDamageType& dmgType) override;
	virtual void KilledBy(class APawn* EventInstigator);

	FORCEINLINE class UCameraComponent* GetSideViewCameraComponent() const { return SideViewCameraComponent; }
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	FORCEINLINE UHealthComponent* GetHealthComponent() const { return HealthComponent; }
	FORCEINLINE UCharacterInteractionComponent* GetInteractionComponent() const { return InteractionComponent; }
	FORCEINLINE UGrapplingComponent* GetGrapplingComponent() const { return GrapplingComponent; }
	FORCEINLINE UDashingComponent* GetDashingComponent() const { return DashingComponent; }
	FORCEINLINE const FAxisPositions& GetAxisPostisions() const { return mAxisPositions; }
	
	UFUNCTION(BlueprintCallable, Category = "Health")
	virtual float TakeDamage(float Damage, const FDamageEvent& DamageEvent, class AController* EventInstigator, class AActor* DamageCauser) override;

	UFUNCTION(BlueprintCallable, Category = "Health")
	virtual void Suicide();

	UFUNCTION(BlueprintCallable, Category = "Health")
	virtual bool Die(float KillingDamage, const FDamageEvent& DamageEvent, class AController* Killer, class AActor* DamageCauser);

	UFUNCTION(BlueprintCallable, Category = "Combat")
	void StopAttacking();

	UFUNCTION(BlueprintCallable, Category = CharacterMovement)
	FORCEINLINE UVertCharacterMovementComponent* GetVertCharacterMovement() const { if (UVertCharacterMovementComponent* movement = Cast<UVertCharacterMovementComponent>(GetCharacterMovement())) { return movement; } return nullptr; }

	UFUNCTION(BlueprintCallable, Category = "Climbing")
	FORCEINLINE ULedgeGrabbingComponent* GetClimbingComponent() const { return ClimbingComponent; }

	UFUNCTION(BlueprintCallable, Category = "Ragdoll")
	FORCEINLINE bool IsRagdolling() const { return mIsRagdolling; }

	UFUNCTION(BlueprintCallable, Category = "CharacterMovement")
	FORCEINLINE bool IsGrounded() const { return !IsJumpProvidingForce() && !GetCharacterMovement()->IsFalling(); }

	UFUNCTION(BlueprintCallable, Category = "Weapons")
	FORCEINLINE class ABaseWeapon* GetWeapon() const { return InteractionComponent->GetHeldWeapon(); }

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

	UFUNCTION(BlueprintCallable, Category = "Hitstun")
	bool IsInHitstun() const;

protected:
	void ActionMoveRight(float Value);
	void ActionGrappleShoot();
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
	void UpdateCharacter();
	void UpdateTeamColours(UMaterialInstanceDynamic* useMIDs);

	virtual void ApplyDamageHitstun(int32 hitstunFrames);
	virtual bool CanJumpInternal_Implementation() const override;
	virtual void SetupPlayerInputComponent(class UInputComponent* InputComponent) override;
	
	UFUNCTION(BlueprintCallable, Category = "Player Controller")
	FORCEINLINE AVertPlayerController* GetPlayerController() const { if (AController* controller = GetController()) { if (AVertPlayerController* playerController = Cast<AVertPlayerController>(controller)) { return playerController; } } return nullptr; }

	UFUNCTION()
	void OnPickupInteractive(AInteractive* interactive, bool wasCaught);

	UFUNCTION()
	void OnDropInteractive(AInteractive* interactive, bool wasThrown);

	/// Blueprint Implementable functions //////////////////////////////////////////////////////////////////////////
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Actions")
	void Character_OnGrappleShootExecuted(const FVector& direction);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Actions")
	void Character_OnGrapplePullExecuted(const FVector& direction);

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
	void Character_OnStopAttackExecuted();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Actions")
	void Character_OnJumpExecuted();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Actions")
	void Character_OnWeaponStateChangeExecuted(ABaseWeapon* weapon, EWeaponState newState, UAnimSequence* playerAnim);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Actions")
	void Character_OnWeaponFiredWithRecoilExecuted(float recoilAmount);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Actions")
	void Character_OnLedgeTransition(ELedgeTransition transition);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Actions")
	void Character_OnLedgeGrabbed(const FHitResult& forwardHit, const FHitResult& downwardHit);

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
	FTimerHandle mHitStunTimer;
	bool mGamepadOnStandby = false;
	bool mIsDying = false;
	bool mIsRagdolling = false;

	FScriptDelegate mOnWeaponStateChangedDelegate;
	FScriptDelegate mOnWeaponFiredWithRecoilDelegate;
};
