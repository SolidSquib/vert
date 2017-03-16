// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "GameFramework/GameMode.h"
#include "VertGameMode.generated.h"

// The GameMode defines the game being played. It governs the game rules, scoring, what actors
// are allowed to exist in this game type, and who may enter the game.
//
// This game mode just sets the default pawn to be the MyCharacter asset, which is a subclass of VertCharacter

UCLASS(minimalapi)
class AVertGameMode : public AGameMode
{
	GENERATED_BODY()
public:
	AVertGameMode();

	virtual void BeginPlay() override;
	virtual APlayerController* SpawnPlayerController(ENetRole InRemoteRole, FVector const& SpawnLocation, FRotator const& SpawnRotation) override;

protected:
	UFUNCTION(BlueprintNativeEvent, Category = "Input")
	void OnControllerConnectionChange(bool connected, int32 userID, int32 controllerID);

protected:
	FDelegateHandle mOnControllerChangedHandle;
};
