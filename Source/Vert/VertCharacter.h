// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "PaperCharacter.h"
#include "Engine/DebugGroup.h"
#include "Character/GrappleLauncher.h"
#include "VertCharacter.generated.h"

// This class is the default character for Vert, and it is responsible for all
// physical interaction between the player and the world.
//
//   The capsule component (inherited from ACharacter) handles collision with the world
//   The CharacterMovementComponent (inherited from ACharacter) handles movement of the collision capsule
//   The Sprite component (inherited from APaperCharacter) handles the visuals

UENUM()
enum class EDashAimMode : uint8
{
	PlayerDirection UMETA(DisplayName = "Player Direction (Left Stick)"),
	AimDirection UMETA(DisplayName = "Aim Direction (Right Stick)")
};

UENUM()
enum class ERechargeRule : uint8
{
	OnContactGround,
	OnRechargeTimer,
	OnContactGroundOrWall
};

USTRUCT()
struct FGrappleConfigRules
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, Category = "Recharge")
	ERechargeRule RechargeMode;

	UPROPERTY(EditDefaultsOnly, Category = "Aim")
	EAimFreedom AimFreedom;

	FGrappleConfigRules()
	{
		RechargeMode = ERechargeRule::OnContactGround;
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

	UPROPERTY(EditDefaultsOnly, Category = "Recharge", Meta = (EditCondition = "RechargeMode == ERechargeRule::OnRechargeTimer", DisplayName = "Time To Recharge (s)"))
	float TimeToRecharge;

	uint8 IsDashing : 1;
	float DistanceTravelled;
	FVector DirectionOfTravel;
	float Timer;
	float RechargeTimer;

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
		TimeToRecharge = 2.f;

		IsDashing = false;
		DistanceTravelled = 0.f;
		DirectionOfTravel = FVector::ZeroVector;
		Timer = 0.f;
		RechargeTimer = 0.f;
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
	struct ThumbstickPos
	{
		float RightX, RightY, LeftX, LeftY;

		ThumbstickPos()
		{
			RightX = 0;
			RightY = 0;
			LeftX = 0;
			LeftY = 0;
		}
	};

	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GrappleConfig")
	FGrappleConfigRules Grapple;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DashConfig")
	FDashConfigRules Dash;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Debug)
	FCharacterDebugSettings ShowDebug;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GrappleConfig", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<class AGrappleLauncher> GrappleClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GrappleConfig", meta = (AllowPrivateAccess = "true"))
	FName GrappleHandSocket;

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

	void RegisterGrappleHook(class AGrappleHook* hook);

	virtual void Tick(float DeltaSeconds) override;
	virtual void BeginPlay() override;
	virtual void PreInitializeComponents() override;

	FORCEINLINE class UCameraComponent* GetSideViewCameraComponent() const { return SideViewCameraComponent; }
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	FORCEINLINE int32 GetRemainingGrapples() const { return mRemainingGrapples; }
	FORCEINLINE int32 DecrementRemainingGrapples() { return --mRemainingGrapples; }
	FORCEINLINE void ResetRemainingGrapples() { mRemainingGrapples = MaxGrapples; }

	UFUNCTION(BlueprintCallable, Category = Thumbsticks)
	FORCEINLINE FVector GetPlayerLeftThumbstick() const { return FVector(mThumbstickPositions.LeftX, 0.f, mThumbstickPositions.LeftY); }

	UFUNCTION(BlueprintCallable, Category = Thumbsticks)
	FORCEINLINE FVector GetPlayerRightThumbstick() const { return FVector(mThumbstickPositions.RightX, 0.f, mThumbstickPositions.RightY); }

	UFUNCTION(BlueprintCallable, Category = Thumbsticks)
	FORCEINLINE FVector2D GetPlayerLeftThumbstick2D() const { return FVector2D(mThumbstickPositions.LeftX, mThumbstickPositions.LeftY); }

	UFUNCTION(BlueprintCallable, Category = Thumbsticks)
	FORCEINLINE FVector2D GetPlayerRightThumbstick2D() const { return FVector2D(mThumbstickPositions.RightX, mThumbstickPositions.RightY); }

	UFUNCTION(BlueprintCallable, Category = Thumbsticks)
	FORCEINLINE FVector GetPlayerLeftThumbstickDirection() const { return (FVector(mThumbstickPositions.LeftX, 0.f, mThumbstickPositions.LeftY) * 100).GetSafeNormal(); }

	UFUNCTION(BlueprintCallable, Category = Thumbsticks)
	FORCEINLINE FVector GetPlayerRightThumbstickDirection() const { return (FVector(mThumbstickPositions.RightX, 0.f, mThumbstickPositions.RightY) * 100).GetSafeNormal(); }

	UFUNCTION(BlueprintCallable, Category = Thumbsticks)
	FORCEINLINE FVector2D GetPlayerLeftThumbstickDirection2D() const { return (FVector2D(mThumbstickPositions.LeftX, mThumbstickPositions.LeftY) * 100).GetSafeNormal(); }

	UFUNCTION(BlueprintCallable, Category = Thumbsticks)
	FORCEINLINE FVector2D GetPlayerRightThumbstickDirection2D() const { return (FVector2D(mThumbstickPositions.RightX, mThumbstickPositions.RightY) * 100).GetSafeNormal(); }

	UFUNCTION(BlueprintCallable, Category = CharacterMovement)
	bool IsGrounded() const;

protected:
	/** Called to choose the correct animation to play based on the character's movement state */
	void UpdateAnimation();

	// Input Mappings
	void MoveRight(float Value);
	void GrappleShoot();
	void GrappleShootGamepad();
	void RightThumbstickMoveX(float value);
	void RightThumbstickMoveY(float value);
	void LeftThumbstickMoveX(float value);
	void LeftThumbstickMoveY(float value);
	void DoDash();
	// END Input Mappings

	void TickDash(float deltaSeconds);
	void UpdateCharacter();

	virtual void Jump() override;
	virtual void Landed(const FHitResult& Hit) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* InputComponent) override;

	UFUNCTION(BlueprintNativeEvent, Category = "Grappling")
	void OnHooked();

	UFUNCTION(BlueprintNativeEvent, Category = "Grappling")
	void OnFired();

	UFUNCTION(BlueprintNativeEvent, Category = "Grappling")
	void OnReturned();

private:
#if !UE_BUILD_SHIPPING
	void PrintDebugInfo();
#endif

protected:
	TWeakObjectPtr<AGrappleLauncher> mGrappleLauncher = nullptr;

	FVector mGrappleAimGamepad = FVector::ZeroVector;
	int32 mRemainingGrapples = 0;
	int32 mRemainingDashes = 0;
	ThumbstickPos mThumbstickPositions;
	float mGravityScale = 1.f;
	float mGroundFriction = 3.0f;

	bool mDisableInput = false;
};
