// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "VertGameMode.h"
#include "Vert.h"
#include "VertCharacter.h"

DEFINE_LOG_CATEGORY(LogVertGameMode);

AVertGameMode::AVertGameMode()
{
	// set default pawn class to our character
	HUDClass = AVertHUD::StaticClass();
	DefaultPawnClass = AVertCharacter::StaticClass();
}

void AVertGameMode::BeginPlay()
{
	Super::BeginPlay();
	mOnControllerChangedHandle = FCoreDelegates::OnControllerConnectionChange.AddUFunction(this, TEXT("OnControllerConnectionChange"));
}

//************************************
// Method:    SpawnPlayerController
// FullName:  AVertGameMode::SpawnPlayerController
// Access:    virtual public 
// Returns:   APlayerController*
// Qualifier:
// Parameter: ENetRole InRemoteRole
// Parameter: FVector const & SpawnLocation
// Parameter: FRotator const & SpawnRotation
//************************************
APlayerController* AVertGameMode::SpawnPlayerController(ENetRole InRemoteRole, FVector const& SpawnLocation, FRotator const& SpawnRotation)
{
	APlayerController* controller = Super::SpawnPlayerController(InRemoteRole, SpawnLocation, SpawnRotation);

	if (AVertPlayerController* vPlayerController = Cast<AVertPlayerController>(controller))
	{
		FScriptDelegate onPossessedDelegate;
		onPossessedDelegate.BindUFunction(this, TEXT("OnPlayerControllerPossessedPawn"));
		vPlayerController->OnPossessed.Add(onPossessedDelegate);

		FScriptDelegate onUnPossessedDelegate;
		onUnPossessedDelegate.BindUFunction(this, TEXT("OnPlayerControllerUnPossessedPawn"));
		vPlayerController->OnUnPossessed.Add(onUnPossessedDelegate);

		UE_LOG(LogVertGameMode, Log, TEXT("Player controller delegates bound for %s"), *vPlayerController->GetName());
	}

	return controller;
}

//************************************
// Method:    Killed
// FullName:  AVertGameMode::Killed
// Access:    virtual public 
// Returns:   void
// Qualifier:
// Parameter: AController * Killer
// Parameter: AController * KilledPlayer
// Parameter: APawn * KilledPawn
// Parameter: const UDamageType * DamageType
//************************************
void AVertGameMode::Killed(AController* killer, AController* victim, APawn* killedPawn, const UDamageType* damageType)
{
	AVertPlayerState* killerPlayerState = killer ? Cast<AVertPlayerState>(killer->PlayerState) : NULL;
	AVertPlayerState* victimPlayerState = victim ? Cast<AVertPlayerState>(victim->PlayerState) : NULL;

	if (killerPlayerState && killerPlayerState != victimPlayerState)
	{
		killerPlayerState->ScoreKill(victimPlayerState, KillScore);
		killerPlayerState->InformAboutKill(killerPlayerState, damageType, victimPlayerState);
	}

	if (victimPlayerState)
	{
		victimPlayerState->ScoreDeath(killerPlayerState, DeathScore);
		victimPlayerState->BroadcastDeath(killerPlayerState, damageType, victimPlayerState);
	}
}

//************************************
// Method:    ModifyDamage
// FullName:  AVertGameMode::ModifyDamage
// Access:    virtual public 
// Returns:   float
// Qualifier: const
// Parameter: float Damage
// Parameter: AActor * DamagedActor
// Parameter: struct FDamageEvent const & DamageEvent
// Parameter: AController * EventInstigator
// Parameter: AActor * DamageCauser
//************************************
float AVertGameMode::ModifyDamage(float Damage, AActor* DamagedActor, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) const
{
	float ActualDamage = Damage;

	AVertCharacter* DamagedPawn = Cast<AVertCharacter>(DamagedActor);
	if (DamagedPawn && EventInstigator)
	{
		AVertPlayerState* DamagedPlayerState = Cast<AVertPlayerState>(DamagedPawn->PlayerState);
		AVertPlayerState* InstigatorPlayerState = Cast<AVertPlayerState>(EventInstigator->PlayerState);

		// disable friendly fire
		if (!CanDealDamage(InstigatorPlayerState, DamagedPlayerState))
		{
			ActualDamage = 0.0f;
		}

		// scale self instigated damage
		if (InstigatorPlayerState == DamagedPlayerState)
		{
			ActualDamage *= DamageSelfScale;
		}
	}

	return ActualDamage;
}

//************************************
// Method:    CanDealDamage
// FullName:  AVertGameMode::CanDealDamage
// Access:    virtual public 
// Returns:   bool
// Qualifier: const
// Parameter: class AVertPlayerState * DamageInstigator
// Parameter: class AVertPlayerState * DamagedPlayer
//************************************
bool AVertGameMode::CanDealDamage(class AVertPlayerState* DamageInstigator, class AVertPlayerState* DamagedPlayer) const
{
	return true;
}

//************************************
// Method:    AllowCheats
// FullName:  AVertGameMode::AllowCheats
// Access:    virtual public 
// Returns:   bool
// Qualifier:
// Parameter: APlayerController * P
//************************************
bool AVertGameMode::AllowCheats(APlayerController* P)
{
	return true;
}

//************************************
// Method:    DetermineMatchWinner
// FullName:  AVertGameMode::DetermineMatchWinner
// Access:    virtual protected 
// Returns:   void
// Qualifier:
//************************************
void AVertGameMode::DetermineMatchWinner()
{
	// nothing to do here
}

//************************************
// Method:    IsWinner
// FullName:  AVertGameMode::IsWinner
// Access:    virtual protected 
// Returns:   bool
// Qualifier: const
// Parameter: class AVertPlayerState * playerState
//************************************
bool AVertGameMode::IsWinner(class AVertPlayerState* playerState) const
{
	return false;
}

//************************************
// Method:    FinishMatch
// FullName:  AVertGameMode::FinishMatch
// Access:    public 
// Returns:   void
// Qualifier:
//************************************
void AVertGameMode::FinishMatch()
{
	AVertGameState* const MyGameState = Cast<AVertGameState>(GameState);
	if (IsMatchInProgress())
	{
		EndMatch();
		DetermineMatchWinner();

		// notify players
		for (FConstControllerIterator It = GetWorld()->GetControllerIterator(); It; ++It)
		{
			AVertPlayerState* PlayerState = Cast<AVertPlayerState>((*It)->PlayerState);
			const bool bIsWinner = IsWinner(PlayerState);

			(*It)->GameHasEnded(NULL, bIsWinner);
		}

		// lock all pawns
		// pawns are not marked as keep for seamless travel, so we will create new pawns on the next match rather than
		// turning these back on.
		for (FConstPawnIterator It = GetWorld()->GetPawnIterator(); It; ++It)
		{
			(*It)->TurnOff();
		}
	}
}

//************************************
// Method:    RequestFinishAndExitToMainMenu
// FullName:  AVertGameMode::RequestFinishAndExitToMainMenu
// Access:    public 
// Returns:   void
// Qualifier:
//************************************
void AVertGameMode::RequestFinishAndExitToMainMenu()
{
	FinishMatch();

	UVertGameInstance* const gameInstance = Cast<UVertGameInstance>(GetGameInstance());
	if (gameInstance)
	{
		gameInstance->RemoveSplitScreenPlayers();
	}

	AVertPlayerController* localPrimaryController = nullptr;
	for (FConstPlayerControllerIterator it = GetWorld()->GetPlayerControllerIterator(); it; ++it)
	{
		AVertPlayerController* controller = Cast<AVertPlayerController>(*it);

		if (controller == NULL)
		{
			continue;
		}

		if (!controller->IsLocalController())
		{
			const FString remoteReturnReason = NSLOCTEXT("NetworkErrors", "HostHasLeft", "Host has left the game.").ToString();
			controller->ClientReturnToMainMenu(remoteReturnReason);
		}
		else
		{
			localPrimaryController = controller;
		}
	}

	// GameInstance should be calling this from an EndState.  So call the PC function that performs cleanup, not the one that sets GI state.
	if (localPrimaryController != NULL)
	{
		localPrimaryController->HandleReturnToMainMenu();
	}
}

//************************************
// Method:    RegisterPlayerPawn
// FullName:  AVertPlayerCameraActor::RegisterPlayerPawn
// Access:    public 
// Returns:   void
// Qualifier:
// Parameter: APawn * pawnToFollow
//************************************
void AVertGameMode::RegisterPlayerPawn(APawn* pawnToFollow)
{
	if (mPawnsToFollow.Find(pawnToFollow) == INDEX_NONE)
	{
		mPawnsToFollow.Add(pawnToFollow);
		UE_LOG(LogVertGameMode, Warning, TEXT("Pawn added to follow list with name [%s]"), *pawnToFollow->GetName());

		if (AVertPlayerController* controller = Cast<AVertPlayerController>(pawnToFollow->GetController()))
		{
			mPlayerControllers.Add(controller);
		}
	}
}

//************************************
// Method:    UnregisterPlayerPawn
// FullName:  AVertPlayerCameraActor::UnregisterPlayerPawn
// Access:    public 
// Returns:   void
// Qualifier:
// Parameter: APawn * pawnToFollow
//************************************
void AVertGameMode::UnregisterPlayerPawn(APawn* pawnToFollow)
{
	if (mPawnsToFollow.Find(pawnToFollow) != INDEX_NONE)
	{
		mPawnsToFollow.Remove(pawnToFollow);
		UE_LOG(LogVertGameMode, Warning, TEXT("Pawn removed from follow list with name [%s]"), *pawnToFollow->GetName());

		if (AVertPlayerController* controller = Cast<AVertPlayerController>(pawnToFollow->GetController()))
		{
			mPlayerControllers.Remove(controller);
		}
	}
}

//************************************
// Method:    PostLogin
// FullName:  AVertGameMode::PostLogin
// Access:    virtual public 
// Returns:   void
// Qualifier:
// Parameter: APlayerController * NewPlayer
//************************************
void AVertGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	if (NewPlayer && mPlayerCamera.IsValid())
	{
		NewPlayer->SetViewTargetWithBlend(mPlayerCamera.Get(), 1.f);
	}
}

//************************************
// Method:    OnPlayerControllerPossessedPawn_Implementation
// FullName:  AVertGameMode::OnPlayerControllerPossessedPawn_Implementation
// Access:    public 
// Returns:   void
// Qualifier:
// Parameter: APawn * pawn
//************************************
void AVertGameMode::OnPlayerControllerPossessedPawn_Implementation(APawn* pawn)
{
	// do something
}

//************************************
// Method:    OnPlayerControllerUnPossessedPawn_Implementation
// FullName:  AVertGameMode::OnPlayerControllerUnPossessedPawn_Implementation
// Access:    public 
// Returns:   void
// Qualifier:
// Parameter: APawn * pawn
//************************************
void AVertGameMode::OnPlayerControllerUnPossessedPawn_Implementation(APawn* pawn)
{
	// do something 
}

//************************************
// Method:    OnControllerConnectionChange_Implementation
// FullName:  AVertGameMode::OnControllerConnectionChange_Implementation
// Access:    public 
// Returns:   void
// Qualifier:
// Parameter: bool connected
// Parameter: int32 userID
// Parameter: int32 controllerID
//************************************
void AVertGameMode::OnControllerConnectionChange_Implementation(bool connected, int32 userID, int32 controllerID)
{
	UE_LOG(LogVertGameMode, Log, TEXT("CONTROLLER CONNECTION CHANGE connected: %s, user: %i, controller: %i"), (connected) ? TEXT("true") : TEXT("false"), userID, controllerID);
}