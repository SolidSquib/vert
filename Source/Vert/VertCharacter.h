// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "PaperCharacter.h"
#include "Engine/VertTimer.h"
#include "Engine/DebugGroup.h"
#include "Character/GrappleLauncher.h"
#include "Character/VertCharacterMovementComponent.h"
#include "Character/HealthComponent.h"
#include "Engine/AxisPositions.h"
#include "VertCharacter.generated.h"

// This class is the default character for Vert, and it is responsible for all
// physical interaction between the player and the world.
//
//   The capsule component (inherited from ACharacter) handles collision with the world
//   The CharacterMovementComponent (inherited from ACharacter) handles movement of the collision capsule
//   The Sprite component (inherited from APaperCharacter) handles the visuals

DECLARE_LOG_CATEGORY_EXTERN(LogVertCharacter, Log, All);

UENUM()
enum class EDashAimMode : uint8
{
	PlayerDirection UMETA(DisplayName = "Player Direction (Left Stick)"),
	AimDirection UMETA(DisplayName = "Aim Direction (Right Stick)")
};

UENUM()
enum class ERechargeRule : uint8
{
	OnRechargeTimer UMETA(DisplayName="Always recharge"),
	OnContactGround UMETA(DisplayName = "Recharge on ground"),
	OnContactGroundOrLatchedToHook UMETA(DisplayName = "Recharge on ground / Latched to climbing hooks"),
	OnContactGroundOrLatchedAnywhere UMETA(DisplayName = "Recharge on ground / Latched to any surface")
};

USTRUCT()
struct FGrappleConfigRules
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, Category = "Recharge")
	ERechargeRule RechargeMode;

	UPROPERTY(EditDefaultsOnly, Category = "Recharge")
	bool RecieveChargeOnGroundOnly;

	UPROPERTY(EditDefaultsOnly, Category = "Aim")
	EAimFreedom AimFreedom;
	
	UPROPERTY(EditDefaultsOnly, Category = "Recharge")
	FVertTimer RechargeTimer;

	FGrappleConfigRules()
	{
		RechargeMode = ERechargeRule::OnContactGround;
		RecieveChargeOnGroundOnly = false;
		AimFreedom = EAimFreedom::Free;
	}
};

USTRUCT()
struct FDashConfigRules
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditDefaultsOnly, Category = "LaunchOptions")
	uint8 UseMomentum : 1;

	UPROPERTY(EditDefaultsOnly, Category = "Speed", Meta = (EditCondition = "UseMomentum"))
	float LaunchForce;

	UPROPERTY(EditDefaultsOnly, Category = "LaunchOptions", Meta = (EditCondition = "UseMomentum", DisplayName = "Override Horizontal Velocity"))
	uint8 OverrideXY : 1;

	UPROPERTY(EditDefaultsOnly, Category = "LaunchOptions", Meta = (EditCondition = "UseMomentum", DisplayName = "Override Vertical Velocity"))
	uint8 OverrideZ : 1;

	UPROPERTY(EditDefaultsOnly, Category = "LaunchOptions", Meta = (EditCondition = "UseMomentum", DisplayName = "Last For (s)"))
	float TimeToDash;

	UPROPERTY(EditDefaultsOnly, Category = "Speed", Meta = (EditCondition = "!UseMomentum"))
	float DashLength;

	UPROPERTY(EditDefaultsOnly, Category = "Speed", Meta = (EditCondition = "!UseMomentum"))
	float LinearSpeed;

	UPROPERTY(EditDefaultsOnly, Category = "LaunchOptions")
	uint8 DisableGravityWhenDashing : 1;

	UPROPERTY(EditDefaultsOnly, Category = "Aim")
	EDashAimMode AimMode;

	UPROPERTY(EditDefaultsOnly, Category = "Aim")
	EAimFreedom AimFreedom;

	UPROPERTY(EditDefaultsOnly, Category = "Recharge")
	ERechargeRule RechargeMode;

	UPROPERTY(EditDefaultsOnly, Category = "Recharge")
	bool RecieveChargeOnGroundOnly;

	UPROPERTY(EditDefaultsOnly, Category = "Recharge")
	FVertTimer RechargeTimer;

	uint8 IsDashing : 1;
	float DistanceTravelled;
	FVector DirectionOfTravel;
	float Timer;
	FDashConfigRules()
	{
		UseMomentum = true;
		LaunchForce = 2000.f;
		OverrideXY = true;
		OverrideZ = true;
		TimeToDash = 0.5;
		DashLength = 20.f;
		LinearSpeed = 20.f;
		DisableGravityWhenDashing = false;
		AimMode = EDashAimMode::PlayerDirection;
		AimFreedom = EAimFreedom::Free;
		RechargeMode = ERechargeRule::OnContactGround;
		RecieveChargeOnGroundOnly = false;

		IsDashing = false;
		DistanceTravelled = 0.f;
		DirectionOfTravel = FVector::ZeroVector;
		Timer = 0.f;
	}
};

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
	bool InfiniteDashGrapple;

	FCharacterDebugSettings()
	{
		Grapple.MessageColour = FColor::Emerald;
		Dash.MessageColour = FColor::Blue;
		CharacterMovement.MessageColour = FColor::Red;
		InfiniteDashGrapple = false;
	}
};

UCLASS(config = Game)
class AVertCharacter : public APaperCharacter
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Controls|GrappleConfig")
	FGrappleConfigRules Grapple;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Controls|DashConfig")
	FDashConfigRules Dash;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Debug)
	FCharacterDebugSettings ShowDebug;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Controls|GrappleConfig", meta = (AllowPrivateAccess = "true"))
	FName GrappleHandSocket;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Controls|GrappleConfig", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<class AGrappleLauncher> GrappleClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CharacterHealth")
	UHealthComponent* HealthComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class UCameraComponent* SideViewCameraComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class USpringArmComponent* CameraBoom;

	// The animation to play while running around
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Animations)
	class UPaperFlipbook* RunningAnimation;

	// The animation to play while idle (standing still)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Animations)
	class UPaperFlipbook* IdleAnimation;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Controls)
	int32 MaxGrapples;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Controls)
	int32 MaxDashes;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Controls)
	bool DisableInputWhenDashingOrGrappling;

public:
	AVertCharacter(const class FObjectInitializer& ObjectInitializer);

	void RegisterGrappleHookDelegates(class AGrappleHook* hook);

	virtual void Tick(float DeltaSeconds) override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type endPlayReason) override;
	virtual void PreInitializeComponents() override;
	virtual void Landed(const FHitResult& Hit) override;

	FORCEINLINE class UCameraComponent* GetSideViewCameraComponent() const { return SideViewCameraComponent; }
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	FORCEINLINE int32 GetRemainingGrapples() const { return mRemainingGrapples; }
	FORCEINLINE int32 DecrementRemainingGrapples() { return --mRemainingGrapples; }

	UFUNCTION(BlueprintCallable, Category = CharacterMovement)
	FORCEINLINE UVertCharacterMovementComponent* GetVertCharacterMovement() const { if (UVertCharacterMovementComponent* movement = Cast<UVertCharacterMovementComponent>(GetCharacterMovement())) { return movement; } return nullptr; }

	UFUNCTION(BlueprintCallable, Category = "CharacterMovement")
	FORCEINLINE bool IsGrounded() const { return !GetCharacterMovement()->IsFlying() && !GetCharacterMovement()->IsFalling(); }

protected:
	/** Called to choose the correct animation to play based on the character's movement state */
	void UpdateAnimation();

	void MoveRight(float Value);
	void GrappleShootMK();
	void GrappleShootGamepad(const FVector2D& axis);
	void DashMK();
	void DashGamepad();
	void RightThumbstickMoveX(float value);
	void RightThumbstickMoveY(float value);
	void LeftThumbstickMoveX(float value);
	void LeftThumbstickMoveY(float value);
	void MouseMove(float value);
	
	void TickDash(float deltaSeconds);
	void UpdateCharacter();
	void SortAbilityRechargeState();

	virtual void Jump() override;
	virtual void SetupPlayerInputComponent(class UInputComponent* InputComponent) override;

	UFUNCTION(BlueprintNativeEvent, Category = "Grappling")
	void OnHooked();

	UFUNCTION(BlueprintNativeEvent, Category = "Grappling")
	void OnFired();

	UFUNCTION(BlueprintNativeEvent, Category = "Grappling")
	void OnReturned();

	UFUNCTION(BlueprintNativeEvent, Category = "Grappling")
	void OnLatched(class AGrappleHook* hook);

	UFUNCTION(BlueprintNativeEvent, Category = "Grappling")
	void OnUnLatched(class AGrappleHook* hook);

	UFUNCTION(BlueprintNativeEvent, Category = "Recharging")
	void OnDashRechargeTimerFinished();

	UFUNCTION(BlueprintNativeEvent, Category = "Recharging")
	void OnGrappleRechargeTimerFinished();

	UFUNCTION(BlueprintCallable, Category = "Grappling")
	FORCEINLINE AGrappleHook* GetGrappleHook() const
	{
		if (mGrappleLauncher.IsValid())
		{
			return mGrappleLauncher->GetGrappleHook();
		}

		return nullptr;
	}

	UFUNCTION(BlueprintCallable, Category = "Player Controller")
	FORCEINLINE AVertPlayerController* GetPlayerController() const
	{
		if (AController* controller = GetController())
		{
			if (AVertPlayerController* playerController = Cast<AVertPlayerController>(controller))
			{
				return playerController;
			}
		}

		return nullptr;
	}

private:
#if !UE_BUILD_SHIPPING
	void PrintDebugInfo();
#endif
	bool CanRecharge(ERechargeRule rule);
	bool CheckShootGrappleGamepad();

protected:
	TWeakObjectPtr<AGrappleLauncher> mGrappleLauncher = nullptr;

	FAxisPositions mAxisPositions;
	int32 mRemainingGrapples = 0;
	int32 mRemainingDashes = 0;
	bool mDisableDash = false;
	bool mDisableGrapple = false;
	bool mDisableMovement = false;
	bool mDisableJump = false;
	bool mUsingMouse = false;
};
