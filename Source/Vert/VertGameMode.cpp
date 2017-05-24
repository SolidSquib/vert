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

	for (TActorIterator<AVertPlayerCameraActor> ActorItr(GetWorld()); ActorItr; ++ActorItr)
	{
		// Same as with the Object Iterator, access the subclass instance with the * or -> operators.
		if (mPlayerCamera.IsValid())
		{
			UE_LOG(LogVertGameMode, Warning, TEXT("More than one AVertPlayerCameraActor found in scene, check that the correct camera is being used and the rest are removed."));
		}
		mPlayerCamera = *ActorItr;
	}

	if (mPlayerCamera.IsValid())
	{
		for (auto iter = GetWorld()->GetPlayerControllerIterator(); iter; ++iter)
		{
			APlayerController* controller = iter->Get();
			controller->SetViewTarget(mPlayerCamera.Get());
		}
	} else { UE_LOG(LogVertGameMode, Warning, TEXT("No AVertPlayerCaneraActor found, game will default to FPS view.")); }
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

void AVertGameMode::OnPlayerControllerPossessedPawn_Implementation(APawn* pawn)
{
	if (mPlayerCamera.IsValid())
	{
		if (pawn)
		{
			mPlayerCamera->RegisterPlayerPawn(pawn);
		}
	}
}

void AVertGameMode::OnPlayerControllerUnPossessedPawn_Implementation(APawn* pawn)
{
	if (mPlayerCamera.IsValid())
	{
		if (pawn)
		{
			mPlayerCamera->UnregisterPlayerPawn(pawn);
		}
	}
}

void AVertGameMode::OnControllerConnectionChange_Implementation(bool connected, int32 userID, int32 controllerID)
{
	UE_LOG(LogVertGameMode, Log, TEXT("CONTROLLER CONNECTION CHANGE connected: %s, user: %i, controller: %i"), (connected) ? TEXT("true") : TEXT("false"), userID, controllerID);
}