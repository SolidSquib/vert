// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "Vert.h"
#include "VertGameMode.h"
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

	AVertLevelScriptActor* level = Cast<AVertLevelScriptActor>(GetWorld()->GetLevelScriptActor());
	if (level)
	{
		mPlayerCamera = level->GetStartingCamera();
	}
}

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

void AVertGameMode::OnPlayerControllerPossessedPawn_Implementation(APawn* pawn)
{
	// do something
}

void AVertGameMode::OnPlayerControllerUnPossessedPawn_Implementation(APawn* pawn)
{
	// do something 
}

void AVertGameMode::OnControllerConnectionChange_Implementation(bool connected, int32 userID, int32 controllerID)
{
	UE_LOG(LogVertGameMode, Log, TEXT("CONTROLLER CONNECTION CHANGE connected: %s, user: %i, controller: %i"), (connected) ? TEXT("true") : TEXT("false"), userID, controllerID);
}