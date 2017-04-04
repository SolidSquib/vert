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
	PlayerCameraClass = AVertPlayerCameraActor::StaticClass();
}

void AVertGameMode::PreInitializeComponents()
{
	Super::PreInitializeComponents();

	if (UWorld* world = GetWorld())
	{
		//Create scannable manager.
		FActorSpawnParameters spawnParameters;
		spawnParameters.Name = TEXT("PlayerCamera");
		spawnParameters.Owner = this;
		spawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		//Spawn the actor.
		mPlayerCamera = world->SpawnActor<AVertPlayerCameraActor>(PlayerCameraClass, FVector::ZeroVector, FRotator::ZeroRotator, spawnParameters);
		if (!mPlayerCamera.IsValid())
		{
			UE_LOG(LogVertGameMode, Fatal, TEXT("Failed to create player camera"));
		}
	}
}

void AVertGameMode::BeginPlay()
{
	Super::BeginPlay();
	mOnControllerChangedHandle = FCoreDelegates::OnControllerConnectionChange.AddUFunction(this, TEXT("OnControllerConnectionChange"));

	if (mPlayerCamera.IsValid())
	{
		for (auto iter = GetWorld()->GetPlayerControllerIterator(); iter; ++iter)
		{
			APlayerController* controller = iter->Get();
			controller->SetViewTarget(mPlayerCamera.Get());
		}
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