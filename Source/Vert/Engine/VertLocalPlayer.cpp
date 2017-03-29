// Copyright Inside Out Games Ltd. 2017

#include "Vert.h"
#include "VertLocalPlayer.h"

void UVertLocalPlayer::PlayerAdded(UGameViewportClient* InViewportClient, int32 InControllerID)
{
	static int32 playerIndex = 0;

	Super::PlayerAdded(InViewportClient, InControllerID);

	mPlayerIndex = playerIndex++;

	UE_LOG(LogVertLocalPlayer, Log, TEXT("Player %i added with controller ID %i"), mPlayerIndex, InControllerID);
}

bool UVertLocalPlayer::SpawnPlayActor(const FString& URL, FString& OutError, UWorld* InWorld)
{
	bool shouldSpawn = Super::SpawnPlayActor(URL, OutError, InWorld);

	return shouldSpawn;
}
