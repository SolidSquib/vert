// Copyright Inside Out Games Ltd. 2017

#include "Vert.h"
#include "VertPlayerController.h"

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
// Method:    DropIn
// FullName:  AVertPlayerController::DropIn
// Access:    virtual public 
// Returns:   void
// Qualifier:
//************************************
void AVertPlayerController::DropIn()
{
	// #MI_TODO: Needs to be slightly more fancy eventually but it does the job for now.
	SetSpectatorPawn(SpawnSpectatorPawn());
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
		UE_LOG(LogVertPlayerController, Warning, TEXT("Broadcasting controller possessed"));
		OnPossessed.Broadcast(aPawn);


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
		UE_LOG(LogVertPlayerController, Warning, TEXT("Broadcasting controller unpossessed"));
		OnUnPossessed.Broadcast(GetPawn());
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
	ServerRestartPlayer();
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
		InputComponent->BindAction("DropIn", IE_Pressed, this, &AVertPlayerController::DropIn);
		InputComponent->BindAction("ToggleFOV", IE_Pressed, this, &AVertPlayerController::ToggleFOV);
	}	
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

	UE_LOG(LogVertPlayerController, Warning, TEXT("Player inactive, no spectator spawned"));

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
// Method:    RespawnDeadPawn_Implementation
// FullName:  AVertPlayerController::RespawnDeadPawn_Implementation
// Access:    public 
// Returns:   void
// Qualifier:
//************************************
void AVertPlayerController::RespawnDeadPawn_Implementation()
{

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
}