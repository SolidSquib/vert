// Copyright Inside Out Games Ltd. 2017

#include "Vert.h"
#include "VertLocalPlayer.h"

void UVertLocalPlayer::PlayerAdded(UGameViewportClient* InViewportClient, int32 InControllerID)
{
	Super::PlayerAdded(InViewportClient, InControllerID);

	UE_LOG(VertCritical, Warning, TEXT("[UVertLocalPlayer] Player added with controller ID %i"), InControllerID);
}

bool UVertLocalPlayer::SpawnPlayActor(const FString& URL, FString& OutError, UWorld* InWorld)
{
	bool shouldSpawn = Super::SpawnPlayActor(URL, OutError, InWorld);

	//if (!mPlayerInGame) shouldSpawn = false;

	return shouldSpawn;
}
