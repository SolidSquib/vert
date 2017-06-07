// Copyright Inside Out Games Ltd. 2017

#include "VertLocalPlayer.h"
#include "Vert.h"

void UVertLocalPlayer::PlayerAdded(UGameViewportClient* InViewportClient, int32 InControllerID)
{
	Super::PlayerAdded(InViewportClient, InControllerID);

	UE_LOG(LogVertLocalPlayer, Log, TEXT("Player added with controller ID %i"), InControllerID);
}

bool UVertLocalPlayer::SpawnPlayActor(const FString& URL, FString& OutError, UWorld* InWorld)
{
	bool shouldSpawn = Super::SpawnPlayActor(URL, OutError, InWorld);

	return shouldSpawn;
}
