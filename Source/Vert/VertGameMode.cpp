// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "Vert.h"
#include "VertGameMode.h"
#include "VertCharacter.h"

AVertGameMode::AVertGameMode()
{
	// set default pawn class to our character
	DefaultPawnClass = AVertCharacter::StaticClass();	
}

void AVertGameMode::BeginPlay()
{
	Super::BeginPlay();
	UE_LOG(VertCritical, Warning, TEXT("[VertGameMode] BeginPlay"));
	mOnControllerChangedHandle = FCoreDelegates::OnControllerConnectionChange.AddUFunction(this, TEXT("OnControllerConnectionChange"));
}

APlayerController* AVertGameMode::SpawnPlayerController(ENetRole InRemoteRole, FVector const& SpawnLocation, FRotator const& SpawnRotation)
{
	UE_LOG(VertCritical, Warning, TEXT("[VertGameMode] SpawnPlayerController"));
	return Super::SpawnPlayerController(InRemoteRole, SpawnLocation, SpawnRotation);
}

void AVertGameMode::OnControllerConnectionChange_Implementation(bool connected, int32 userID, int32 controllerID)
{
	UE_LOG(VertCritical, Warning, TEXT("CONTROLLER CONNECTION CHANGE! connected: %s, user: %i, controller: %i"), (connected) ? TEXT("true") : TEXT("false"), userID, controllerID);
}