// Copyright Inside Out Games Ltd. 2017

#pragma once

#include "GameFramework/PlayerController.h"
#include "Engine/VertGlobals.h"
#include "VertPlayerController.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogVertPlayerController, Log, All);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPossessedDelegate, APawn*, pawn);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnUnPossessedDelegate, APawn*, pawn);

UENUM(BlueprintType)
enum class EControllerType : uint8
{
	Keyboard_Mouse,
	Gamepad_Xbox,
	Gamepad_PS4,
	Gamepad_Steam,
	Gamepad_Switch
};

/**
 * 
 */
UCLASS()
class VERT_API AVertPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable, Category = "PawnPossesion")
	FOnPossessedDelegate OnPossessed;

	UPROPERTY(BlueprintAssignable, Category = "PawnPossesion")
	FOnUnPossessedDelegate OnUnPossessed;

public:
	AVertPlayerController();

	void OnKill();

	virtual void DropIn();
	virtual void HandleReturnToMainMenu();
	virtual void Possess(APawn* aPawn) override;
	virtual void UnPossess() override;
	virtual void UnFreeze() override;
#if PLATFORM_WINDOWS
	virtual bool InputKey(FKey Key, EInputEvent EventType, float AmountDepressed, bool bGamepad) override;
#endif

	class UVertLocalPlayer* GetVertLocalPlayer();
	class AVertPlayerState* GetVertPlayerState();

	FORCEINLINE void ToggleFOV() { UE_LOG(LogVertPlayerController, Warning, TEXT("Toggling FOV for test")); mTestFOV = !mTestFOV; }
	FORCEINLINE bool IsTestingFOV() const { return mTestFOV; }
	
	UFUNCTION(BlueprintCallable, Category = "PlayerManagement")
	virtual void DropOut();

	UFUNCTION(exec)
	void SetGodMode(bool enable);

	UFUNCTION(exec)
	void SetInfiniteWeaponUsage(bool enable);

	UFUNCTION(exec)
	void SetInfiniteClip(bool enable);

	UFUNCTION(exec)
	void OverridePawnFollow(int32 pawnIndex);

	UFUNCTION(exec)
	void OverrideCameraZoom(int32 cameraZoomAmount);

	UFUNCTION(exec)
	void EnableDebugInfo(bool enable);

	UFUNCTION(BlueprintCallable, Category = "InputMethod")
	bool UsingGamepad() const;

	UFUNCTION(BlueprintCallable)
	bool HasInfiniteWeaponUsage() const;
	
	UFUNCTION(BlueprintCallable)
	bool HasInfiniteClip() const;

	UFUNCTION(BlueprintCallable)
	bool HasGodMode() const;

protected:
	virtual void SetupInputComponent() override;
	virtual ASpectatorPawn* SpawnSpectatorPawn() override;

	UFUNCTION(BlueprintNativeEvent, Category = "PlayerPawn")
	void OnPawnDeath(const struct FTakeHitInfo& lastHit);

	UFUNCTION(BlueprintNativeEvent, Category = "PlayerPawn")
	void RespawnDeadPawn();

private:
	bool mTestFOV = false;
	bool mGodMode = false;
	bool mInfiniteClip = false;
	bool mInfiniteWeaponUsage = false;
#if PLATFORM_WINDOWS || PLATFORM_XBOXONE
	EControllerType mControllerType = EControllerType::Gamepad_Xbox;
#elif PLATFORM_PS4
	EControllerType mControllerType = EControllerType::Gamepad_PS4;
#elif PLATFORM_SWITCH
	EControllerType mControllerType = EControllerType::Gamepad_Switch;
#endif
	FTimerHandle mPawnDeathRespawnTimer;
};
