// Copyright Inside Out Games Ltd. 2017

#include "VertPlayerController.h"
#include "Blueprint/UserWidget.h"
#include "Actors/SpawnTargetter.h"
#include "Online.h"

DEFINE_LOG_CATEGORY(LogVertPlayerController);

AVertPlayerController::AVertPlayerController()
{
	PlayerCameraManagerClass = AVertCameraManager::StaticClass();
	bAutoManageActiveCameraTarget = false;
}

//************************************
// Method:    OnKill
// FullName:  AVertPlayerController::OnKill
// Access:    public 
// Returns:   void
// Qualifier:
//************************************
void AVertPlayerController::OnKill()
{
	// Do stuff
}

//************************************
// Method:    Suicide
// FullName:  AVertPlayerController::Suicide
// Access:    public 
// Returns:   void
// Qualifier:
//************************************
void AVertPlayerController::Suicide()
{
	AVertCharacterBase* character = Cast<AVertCharacterBase>(GetPawn());
	if (character != nullptr && character->IsValidLowLevel())
	{
		character->Die();
	}
}

//************************************
// Method:    ShowCameraDebug
// FullName:  AVertPlayerController::ShowCameraDebug
// Access:    public 
// Returns:   void
// Qualifier:
// Parameter: bool showDebug
//************************************
void AVertPlayerController::ShowCameraDebug(bool showDebug)
{
	if (AVertCameraManager* vertCameraMan = Cast<AVertCameraManager>(PlayerCameraManager))
	{
		vertCameraMan->SetShowCameraDebug(showDebug);
	}
}

//************************************
// Method:    DropIn
// FullName:  AVertPlayerController::DropIn
// Access:    virtual public 
// Returns:   void
// Qualifier:
//************************************
void AVertPlayerController::StartPressed()
{
	if (mGameFinished)
	{
		mReadyToLeaveGame = !mReadyToLeaveGame;
	}
	else
	{
// 		mStartPressed = true;
// 		if (mStartPressed && mBackPressed)
// 		{
// 			RestartGame();
// 		}

		if (GetInputKeyTimeDown(EKeys::Gamepad_LeftTrigger) > 0 && GetInputKeyTimeDown(EKeys::Gamepad_RightTrigger) > 0 && GetInputKeyTimeDown(EKeys::Gamepad_Special_Left) > 0)
		{
			RestartGame();
		}
	}
}

void AVertPlayerController::StartReleased()
{
	mStartPressed = false;
}

void AVertPlayerController::BackPressed()
{
	if (!mGameFinished)
	{
		mBackPressed = true;
		if (mStartPressed && mBackPressed)
		{
			RestartGame();
		}
	}
}

void AVertPlayerController::BackReleased()
{
	mBackPressed = false;
}

//************************************
// Method:    DropOut
// FullName:  AVertPlayerController::DropOut
// Access:    virtual public 
// Returns:   void
// Qualifier:
//************************************
void AVertPlayerController::DropOut()
{
	UnPossess();		
	// #MI_TODO: possess by AIController or remove character...
}

//************************************
// Method:    UsingGamepad
// FullName:  AVertPlayerController::UsingGamepad
// Access:    public 
// Returns:   bool
// Qualifier: const
//************************************
bool AVertPlayerController::UsingGamepad() const
{
	return mControllerType != EControllerType::Keyboard_Mouse;
}

#if PLATFORM_WINDOWS || PLATFORM_MAC
//************************************
// Method:    InputKey
// FullName:  AVertPlayerController::InputKey
// Access:    virtual public 
// Returns:   bool
// Qualifier:
// Parameter: FKey Key
// Parameter: EInputEvent EventType
// Parameter: float AmountDepressed
// Parameter: bool bGamepad
//************************************
bool AVertPlayerController::InputKey(FKey Key, EInputEvent EventType, float AmountDepressed, bool bGamepad)
{
	// #MI_TODO: input device type for UI
	if (bGamepad)
	{
		bShowMouseCursor = false;
		bEnableClickEvents = false;
		bEnableMouseOverEvents = false;
		mControllerType = EControllerType::Gamepad_Xbox;
	}
	else
	{
		bShowMouseCursor = true;
		bEnableClickEvents = true;
		bEnableMouseOverEvents = true;
		mControllerType = EControllerType::Keyboard_Mouse;
	}

	return Super::InputKey(Key, EventType, AmountDepressed, bGamepad);
}
#endif

//************************************
// Method:    GetVertLocalPlayer
// FullName:  AVertPlayerController::GetVertLocalPlayer
// Access:    public 
// Returns:   UVertLocalPlayer*
// Qualifier:
//************************************
UVertLocalPlayer* AVertPlayerController::GetVertLocalPlayer()
{
	if (ULocalPlayer* localPlayer = GetLocalPlayer())
	{
		if (UVertLocalPlayer* vertLocalPlayer = Cast<UVertLocalPlayer>(localPlayer))
		{
			return vertLocalPlayer;
		}
	}

	return nullptr;
}

//************************************
// Method:    GetVertPlayerState
// FullName:  AVertPlayerController::GetVertPlayerState
// Access:    public 
// Returns:   AVertPlayerState*
// Qualifier:
//************************************
AVertPlayerState* AVertPlayerController::GetVertPlayerState()
{
	if (AVertPlayerState* state = Cast<AVertPlayerState>(PlayerState))
	{
		return state;
	}

	return nullptr;
}

//************************************
// Method:    Possess
// FullName:  AVertPlayerController::Possess
// Access:    virtual public 
// Returns:   void
// Qualifier:
// Parameter: APawn * aPawn
//************************************
void AVertPlayerController::Possess(APawn* aPawn)
{
	Super::Possess(aPawn);

	if (aPawn)
	{
		UE_LOG(LogVertPlayerController, Log, TEXT("Broadcasting controller possessed"));
		OnPossessed.Broadcast(this, aPawn);

		if (wTimer && !TimerWidget)
		{
			TimerWidget = CreateWidget<UUserWidget>(this, wTimer);
			if (TimerWidget)
			{
				TimerWidget->AddToViewport();
				TimerWidget->SetVisibility(ESlateVisibility::Hidden);
			}
		}
	}
}

//************************************
// Method:    UnPossess
// FullName:  AVertPlayerController::UnPossess
// Access:    virtual public 
// Returns:   void
// Qualifier:
//************************************
void AVertPlayerController::UnPossess()
{
	if (GetPawn())
	{
		UE_LOG(LogVertPlayerController, Log, TEXT("Broadcasting controller unpossessed"));
		OnUnPossessed.Broadcast(this, GetPawn());
	}

	Super::UnPossess();
}


//************************************
// Method:    UnFreeze
// FullName:  AVertPlayerController::UnFreeze
// Access:    virtual public 
// Returns:   void
// Qualifier:
//
// Respawn after dying.
//************************************
void AVertPlayerController::UnFreeze()
{
	UE_LOG(LogVertPlayerController, Log, TEXT("Restarting %s"), *GetName());

	ServerRestartPlayer();
}

//************************************
// Method:    FailedToSpawnPawn
// FullName:  AVertPlayerController::FailedToSpawnPawn
// Access:    virtual public 
// Returns:   void
// Qualifier:
//************************************
void AVertPlayerController::FailedToSpawnPawn()
{
	if (StateName == NAME_Inactive)
	{
		BeginInactiveState();
	}
	Super::FailedToSpawnPawn();
}

//************************************
// Method:    HandleReturnToMainMenu
// FullName:  AVertPlayerController::HandleReturnToMainMenu
// Access:    virtual public 
// Returns:   void
// Qualifier:
//************************************
void AVertPlayerController::HandleReturnToMainMenu()
{
	// Do cleanup
}

//************************************
// Method:    SetupInputComponent
// FullName:  AVertPlayerController::SetupInputComponent
// Access:    virtual protected 
// Returns:   void
// Qualifier:
//************************************
void AVertPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (InputComponent)
	{
		InputComponent->BindAction("Start", IE_Pressed, this, &AVertPlayerController::StartPressed);
		InputComponent->BindAction("Start", IE_Released, this, &AVertPlayerController::StartReleased);
		InputComponent->BindAction("Back", IE_Pressed, this, &AVertPlayerController::BackPressed);
		InputComponent->BindAction("Back", IE_Released, this, &AVertPlayerController::BackReleased);
	}	
}

//************************************
// Method:    GetPodSpawnLocation
// FullName:  AVertPlayerController::GetPodSpawnLocation
// Access:    public 
// Returns:   void
// Qualifier:
//************************************
FVector AVertPlayerController::GetPodSpawnLocation()
{
	AVertCameraManager* VertCameraMan = Cast<AVertCameraManager>(PlayerCameraManager);
	if (VertCameraMan)
	{
		FVector TopLeft, BottomRight, BottomLeft, TopRight;
		VertCameraMan->GetCurrentPlayerBounds(TopLeft, BottomRight, BottomLeft, TopRight);
		
		FVector MiddleTop = (TopRight + TopLeft) / 2;
		float MiddleX = MiddleTop.X;
		float MiddleZ = ((TopLeft + BottomLeft) / 2).Z;

		FCollisionQueryParams Params;
		FHitResult Hit(ForceInit);
		const bool DidHit = GetWorld()->LineTraceSingleByChannel(Hit, FVector(MiddleX, MiddleTop.Y, MiddleZ), MiddleTop, ECC_Visibility, Params);
		if (DidHit)
		{
			return Hit.ImpactPoint;
		}

		return FVector(MiddleTop.X, TopLeft.Y, TopLeft.Z - 100.f);
	}

	UE_LOG(LogVertPlayerController, Warning, TEXT("Unable to get PlayerCameraManager"));
	return FVector::ZeroVector;
}

float AVertPlayerController::GetPodTargetHeight()
{
	AVertCameraManager* VertCameraMan = Cast<AVertCameraManager>(PlayerCameraManager);
	if (VertCameraMan)
	{
		FVector TopLeft, BottomRight, BottomLeft, TopRight;
		VertCameraMan->GetCurrentPlayerBounds(TopLeft, BottomRight, BottomLeft, TopRight);

		return TopLeft.Z - 100.f;
	}

	UE_LOG(LogVertPlayerController, Warning, TEXT("Unable to get PlayerCameraManager"));
	return 0;
}

void AVertPlayerController::GetPodLeftAndRightBounds(float& Left, float& Right)
{
	AVertCameraManager* VertCameraMan = Cast<AVertCameraManager>(PlayerCameraManager);
	if (VertCameraMan)
	{
		return VertCameraMan->GetBoundsXMinMax(Left, Right);
	}

	UE_LOG(LogVertPlayerController, Warning, TEXT("Unable to get PlayerCameraManager"));
	Left = Right = 0;
}

//************************************
// Method:    SpawnSpectatorPawn
// FullName:  AVertPlayerController::SpawnSpectatorPawn
// Access:    virtual protected 
// Returns:   ASpectatorPawn*
// Qualifier:
//************************************
ASpectatorPawn* AVertPlayerController::SpawnSpectatorPawn()
{
	ASpectatorPawn* pawn = nullptr;
	return Super::SpawnSpectatorPawn();

	UE_LOG(LogVertPlayerController, Log, TEXT("Player inactive, no spectator spawned"));

	return pawn;
}

//************************************
// Method:    OnPawnDeath_Implementation
// FullName:  AVertPlayerController::OnPawnDeath_Implementation
// Access:    public 
// Returns:   void
// Qualifier:
// Parameter: const FTakeHitInfo & lastHit
//************************************
void AVertPlayerController::OnPawnDeath_Implementation(const FTakeHitInfo& lastHit)
{
	if (--GetVertPlayerState()->Lives > 0)
	{
		const FTimerManager& timerMan = GetWorld()->GetTimerManager();

	}
}

//************************************
// Method:    SetGodMode
// FullName:  AVertPlayerController::SetGodMode
// Access:    public 
// Returns:   void
// Qualifier:
// Parameter: bool enable
//************************************
void AVertPlayerController::SetGodMode(bool enable)
{
	mGodMode = enable;
}

//************************************
// Method:    SetInfiniteWeaponUsage
// FullName:  AVertPlayerController::SetInfiniteWeaponUsage
// Access:    public 
// Returns:   void
// Qualifier:
// Parameter: bool enable
//************************************
void AVertPlayerController::SetInfiniteWeaponUsage(bool enable)
{
	mInfiniteWeaponUsage = enable;
}

//************************************
// Method:    SetInfiniteClip
// FullName:  AVertPlayerController::SetInfiniteClip
// Access:    public 
// Returns:   void
// Qualifier:
// Parameter: bool enable
//************************************
void AVertPlayerController::SetInfiniteClip(bool enable)
{
	mInfiniteClip = enable;
}

//************************************
// Method:    HasInfiniteWeaponUsage
// FullName:  AVertPlayerController::HasInfiniteWeaponUsage
// Access:    public 
// Returns:   bool
// Qualifier: const
//************************************
bool AVertPlayerController::HasInfiniteWeaponUsage() const
{
	return mInfiniteWeaponUsage;
}

//************************************
// Method:    HasInfiniteClip
// FullName:  AVertPlayerController::HasInfiniteClip
// Access:    public 
// Returns:   bool
// Qualifier: const
//************************************
bool AVertPlayerController::HasInfiniteClip() const
{
	return mInfiniteClip;
}

//************************************
// Method:    HasGodMode
// FullName:  AVertPlayerController::HasGodMode
// Access:    public 
// Returns:   bool
// Qualifier: const
//************************************
bool AVertPlayerController::HasGodMode() const 
{
	return mGodMode;
}

//************************************
// Method:    OverridePawnFollow
// FullName:  AVertPlayerController::OverridePawnFollow
// Access:    public 
// Returns:   void
// Qualifier:
// Parameter: int32 pawnIndex
//************************************
void AVertPlayerController::OverridePawnFollow(int32 pawnIndex)
{
	if (AVertGameMode* mode = Cast<AVertGameMode>(GetWorld()->GetAuthGameMode()))
	{
		AVertPlayerCameraActor* cam = mode->GetActivePlayerCamera();
		if (cam)
		{
			cam->OverridePawnFollow(pawnIndex);
		}
	}
}

//************************************
// Method:    OverrideCameraZoom
// FullName:  AVertPlayerController::OverrideCameraZoom
// Access:    public 
// Returns:   void
// Qualifier:
// Parameter: int32 cameraZoomAmount
//************************************
void AVertPlayerController::OverrideCameraZoom(int32 cameraZoomAmount)
{
	if (AVertGameMode* mode = Cast<AVertGameMode>(GetWorld()->GetAuthGameMode()))
	{
		AVertPlayerCameraActor* cam = mode->GetActivePlayerCamera();
		if (cam)
		{
			cam->OverrideCameraZoom(cameraZoomAmount);
		}
	}
}

//************************************
// Method:    EnableDebugInfo
// FullName:  AVertPlayerController::EnableDebugInfo
// Access:    public 
// Returns:   void
// Qualifier:
// Parameter: bool enable
//************************************
void AVertPlayerController::EnableDebugInfo(bool enable)
{
	for (UActorComponent* component : GetComponents())
	{
		if (IDebuggable* debuggable = Cast<IDebuggable>(component))
		{
			debuggable->EnableDebug(enable);
		}
	}

	if (GetPawn())
	{
		for (UActorComponent* component : GetPawn()->GetComponents())
		{
			if (IDebuggable* debuggable = Cast<IDebuggable>(component))
			{
				debuggable->EnableDebug(enable);
			}
		}
	}
}

void AVertPlayerController::ShowTimer()
{
	if (!TimerWidget)
	{
		return;
	}
	else
	{
		TimerWidget->SetVisibility(ESlateVisibility::Visible);
	}
}

void AVertPlayerController::HideTimer()
{
	if (TimerWidget)
	{
		TimerWidget->SetVisibility(ESlateVisibility::Hidden);
	}
}

/** Starts the online game using the session name in the PlayerState */
//************************************
// Method:    ClientStartOnlineGame_Implementation
// FullName:  AVertPlayerController::ClientStartOnlineGame_Implementation
// Access:    public 
// Returns:   void
// Qualifier:
//************************************
void AVertPlayerController::ClientStartOnlineGame_Implementation()
{
	if (!IsPrimaryPlayer())
		return;

	AVertPlayerState* VertPlayerState = Cast<AVertPlayerState>(PlayerState);
	if (VertPlayerState)
	{
		IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get();
		if (OnlineSub)
		{
			IOnlineSessionPtr Sessions = OnlineSub->GetSessionInterface();
			if (Sessions.IsValid())
			{
				UE_LOG(LogOnline, Log, TEXT("Starting session %s on client"), *VertPlayerState->SessionName.ToString());
				Sessions->StartSession(VertPlayerState->SessionName);
			}
		}
	}
	else
	{
		// Keep retrying until player state is replicated
		GetWorldTimerManager().SetTimer(mTimerHandle_ClientStartOnlineGame, this, &AVertPlayerController::ClientStartOnlineGame_Implementation, 0.2f, false);
	}
}

//************************************
// Method:    ClientEndOnlineGame_Implementation
// FullName:  AVertPlayerController::ClientEndOnlineGame_Implementation
// Access:    public 
// Returns:   void
// Qualifier:
//************************************
void AVertPlayerController::ClientEndOnlineGame_Implementation()
{
	if (!IsPrimaryPlayer())
		return;

	AVertPlayerState* VertPlayerState = Cast<AVertPlayerState>(PlayerState);
	if (VertPlayerState)
	{
		IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get();
		if (OnlineSub)
		{
			IOnlineSessionPtr Sessions = OnlineSub->GetSessionInterface();
			if (Sessions.IsValid())
			{
				UE_LOG(LogOnline, Log, TEXT("Ending session %s on client"), *VertPlayerState->SessionName.ToString());
				Sessions->EndSession(VertPlayerState->SessionName);
			}
		}
	}
}

//************************************
// Method:    DisableMovementInput
// FullName:  AVertPlayerController::DisableMovementInput
// Access:    public 
// Returns:   void
// Qualifier:
//************************************
void AVertPlayerController::DisableMovementInput()
{
	mEnableMovement = false;
}

//************************************
// Method:    EnableMovementInput
// FullName:  AVertPlayerController::EnableMovementInput
// Access:    public 
// Returns:   void
// Qualifier:
//************************************
void AVertPlayerController::EnableMovementInput()
{
	mEnableMovement = true;
}

//************************************
// Method:    IsMovementInputEnabled
// FullName:  AVertPlayerController::IsMovementInputEnabled
// Access:    public 
// Returns:   bool
// Qualifier: const
//************************************
bool AVertPlayerController::IsMovementInputEnabled() const
{
	return mEnableMovement;
}

//************************************
// Method:    RestartGame
// FullName:  AVertPlayerController::RestartGame
// Access:    public 
// Returns:   void
// Qualifier:
//************************************
void AVertPlayerController::RestartGame()
{
	AVertGameState* GameState = GetWorld()->GetGameState<AVertGameState>();
	if (GameState)
	{
		GameState->RequestFinishAndExitToMainMenu();
	}
}

//************************************
// Method:    ShowScoreboard
// FullName:  AVertPlayerController::ShowScoreboard
// Access:    public 
// Returns:   void
// Qualifier:
// Should only be called on the primary controller
//************************************
void AVertPlayerController::ShowScoreboard()
{
	if (wScoreboard && ScoreboardWidget == nullptr)
	{
		ScoreboardWidget = CreateWidget<UUserWidget>(GetGameInstance(), wScoreboard);
		if(ScoreboardWidget)
		{
			ScoreboardWidget->AddToViewport();
			ScoreboardWidget->SetVisibility(ESlateVisibility::Visible);
		}
	}
}

//************************************
// Method:    GameHasEnded
// FullName:  AVertPlayerController::GameHasEnded
// Access:    virtual public 
// Returns:   void
// Qualifier:
// Parameter: class AActor * EndGameFocus
// Parameter: bool bIsWinner
//************************************
void AVertPlayerController::GameHasEnded(class AActor* EndGameFocus /* = NULL */, bool bIsWinner /* = false */)
{
	if (IsPrimaryPlayer())
	{
		ShowScoreboard();
	}

	AVertGameMode* GameMode = GetWorld()->GetAuthGameMode<AVertGameMode>();
	AVertPlayerState* VPS = Cast<AVertPlayerState>(PlayerState);
	if (GameMode && GetPawn())
	{
		GameMode->ScoreLastManStanding(VPS);
	}

	mGameFinished = true;
	Super::GameHasEnded(EndGameFocus, bIsWinner);
}