// Copyright Inside Out Games Ltd. 2017

#pragma once

#include "Engine/LocalPlayer.h"
#include "VertLocalPlayer.generated.h"

/**
 * 
 */
UCLASS()
class VERT_API UVertLocalPlayer : public ULocalPlayer
{
	GENERATED_BODY()
	
public:
	virtual void PlayerAdded(class UGameViewportClient* InViewportClient, int32 InControllerID) override;
	virtual bool SpawnPlayActor(const FString& URL, FString& OutError, UWorld* InWorld) override;

	FORCEINLINE bool IsPlayerInGame() const { return mPlayerInGame; }
	FORCEINLINE void PlayerJoinGame() { mPlayerInGame = true; }
	FORCEINLINE void PlayerLeaveGame() { mPlayerInGame = false; }

private:
	bool mPlayerInGame = false;
};
