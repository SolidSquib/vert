// Copyright Inside Out Games Ltd. 2017

#pragma once

#include "GameFramework/PlayerController.h"
#include "Engine/VertGlobals.h"
#include "VertPlayerController.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogVertPlayerController, Log, All);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnPossessedDelegate, AVertPlayerController*, possessor, APawn*, pawn);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnUnPossessedDelegate, AVertPlayerController*, possessor, APawn*, pawn);

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

	friend class AVertGameMode;

public:
	UPROPERTY(BlueprintAssignable, Category = "PawnPossesion")
	FOnPossessedDelegate OnPossessed;

	UPROPERTY(BlueprintAssignable, Category = "PawnPossesion")
	FOnUnPossessedDelegate OnUnPossessed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Widgets")
	TSubclassOf<class UUserWidget> wTimer;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Widgets")
	TSubclassOf<class UUserWidget> wScoreboard;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Respawn")
	FVector DesiredSpawnLocation = FVector::ZeroVector;

	UPROPERTY(EditDefaultsOnly, Category = "Death")
	float LifeSaverTime = 3.f;

protected:
	UPROPERTY()
	UUserWidget* TimerWidget = nullptr;

	UPROPERTY()
	UUserWidget* ScoreboardWidget = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Respawn")
	float TargetterSpeed = 50.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Respawn")
	float TargetterInterpSpeed = 10.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Respawn")
	TSubclassOf<class APlayerDroppod> PlayerDroppodClass = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Respawn")
	float RespawnTimer = 5.f;

public:
	AVertPlayerController();

	void OnKill();
	void DisableMovementInput();
	void EnableMovementInput();
	bool IsMovementInputEnabled() const;
	FVector GetPodSpawnLocation();
	float GetPodTargetHeight();
	void GetPodLeftAndRightBounds(float& Left, float& Right);
	
	virtual void GameHasEnded(class AActor* EndGameFocus = NULL, bool bIsWinner = false) override;
	virtual void HandleReturnToMainMenu();
	virtual void FailedToSpawnPawn() override;
	virtual void Possess(APawn* aPawn) override;
	virtual void UnPossess() override;
	virtual void UnFreeze() override;
#if PLATFORM_WINDOWS || PLATFORM_MAC
	virtual bool InputKey(FKey Key, EInputEvent EventType, float AmountDepressed, bool bGamepad) override;
#endif

	class UVertLocalPlayer* GetVertLocalPlayer();
	class AVertPlayerState* GetVertPlayerState();
	UClass* GetDropPodClass() const { return PlayerDroppodClass; }
	
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

	UFUNCTION(exec)
	void Suicide();

	UFUNCTION(exec)
	void ShowCameraDebug(bool showDebug);

	UFUNCTION(BlueprintCallable, Category = "InputMethod")
	bool UsingGamepad() const;

	UFUNCTION(BlueprintCallable)
	bool HasInfiniteWeaponUsage() const;
	
	UFUNCTION(BlueprintCallable)
	bool HasInfiniteClip() const;

	UFUNCTION(BlueprintCallable)
	bool HasGodMode() const;

	UFUNCTION(BlueprintCallable, Category = UserInterface)
	void ShowTimer();

	UFUNCTION(BlueprintCallable, Category = UserInterface)
	void HideTimer();

	/** Starts the online game using the session name in the PlayerState */
	UFUNCTION(reliable, client)
	void ClientStartOnlineGame();

	/** Ends the online game using the session name in the PlayerState */
	UFUNCTION(reliable, client)
	void ClientEndOnlineGame();

	UFUNCTION(BlueprintCallable)
	bool GetPlayerWantsToLeave() const { return mReadyToLeaveGame; }

protected:
	void LifeSaverTimeExpired();
	void ShowScoreboard();
	void RestartGame();
	void StartPressed();
	void StartReleased();
	void BackPressed();
	void BackReleased();
	
	virtual void SetupInputComponent() override;
	virtual ASpectatorPawn* SpawnSpectatorPawn() override;

	UFUNCTION(BlueprintNativeEvent, Category = "PlayerPawn")
	void OnPawnDeath(const struct FTakeHitInfo& lastHit);

private:
	bool mTestFOV = false;
	bool mGodMode = false;
	bool mInfiniteClip = false;
	bool mInfiniteWeaponUsage = false;
	bool mEnableMovement = true;
	bool mReadyToLeaveGame = false;
	bool mGameFinished = false;
	bool mStartPressed = false;
	bool mBackPressed = false;

#if PLATFORM_WINDOWS || PLATFORM_XBOXONE || PLATFORM_MAC
	EControllerType mControllerType = EControllerType::Gamepad_Xbox;
#elif PLATFORM_PS4
	EControllerType mControllerType = EControllerType::Gamepad_PS4;
#elif PLATFORM_SWITCH
	EControllerType mControllerType = EControllerType::Gamepad_Switch;
#endif
	FTimerHandle mPawnDeathRespawnTimer;
	TWeakObjectPtr<APlayerDroppod> mPlayerDroppod = nullptr;
	
	FTimerHandle mTimerHandle_RespawnTimer;
	FTimerHandle mTimerHandle_ClientStartOnlineGame;
};
