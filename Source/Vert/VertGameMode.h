// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "GameFramework/GameMode.h"
#include "View/VertPlayerCameraActor.h"
#include "VertGameMode.generated.h"

// The GameMode defines the game being played. It governs the game rules, scoring, what actors
// are allowed to exist in this game type, and who may enter the game.
//
// This game mode just sets the default pawn to be the MyCharacter asset, which is a subclass of VertCharacter

DECLARE_LOG_CATEGORY_EXTERN(LogVertGameMode, Log, All);

UCLASS(minimalapi)
class AVertGameMode : public AGameMode
{
	GENERATED_BODY()
		
public:
	AVertGameMode();

	virtual void BeginPlay() override;
	virtual APlayerController* SpawnPlayerController(ENetRole InRemoteRole, FVector const& SpawnLocation, FRotator const& SpawnRotation) override;

	FORCEINLINE void SetPlayerCamera(AVertPlayerCameraActor* newCamera) { mPlayerCamera = newCamera; }
	FORCEINLINE const TArray<APawn*>& GetFollowedActors() const { return mPawnsToFollow; }

	UFUNCTION(BlueprintCallable, Category = "PlayerPosition")
	void RegisterPlayerPawn(class APawn* pawnToFollow);

	UFUNCTION(BlueprintCallable, Category = "PlayerPosition")
	void UnregisterPlayerPawn(class APawn* pawnToFollow);

	UFUNCTION(BlueprintCallable, Category = "PlayerCamera")
	AVertPlayerCameraActor* GetActivePlayerCamera() const { return mPlayerCamera.Get(); }

	UFUNCTION(BlueprintNativeEvent, Category = "PlayerController")
	void OnPlayerControllerPossessedPawn(APawn* pawn);

	UFUNCTION(BlueprintNativeEvent, Category = "PlayerController")
	void OnPlayerControllerUnPossessedPawn(APawn* pawn);

protected:
	UFUNCTION(BlueprintNativeEvent, Category = "Input")
	void OnControllerConnectionChange(bool connected, int32 userID, int32 controllerID);

protected:
	FDelegateHandle mOnControllerChangedHandle;
	TWeakObjectPtr<AVertPlayerCameraActor> mPlayerCamera;
	TArray<class APawn*> mPawnsToFollow;
	TArray<AVertPlayerController*> mPlayerControllers;
};
