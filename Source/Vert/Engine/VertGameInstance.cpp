// Copyright Inside Out Games Ltd. 2017

#include "Vert.h"
#include "VertGameInstance.h"

void UVertGameInstance::Init()
{
	Super::Init();
}

void UVertGameInstance::Shutdown()
{
	Super::Shutdown();
}

void UVertGameInstance::StartGameInstance()
{
	Super::StartGameInstance();
}

ULocalPlayer* UVertGameInstance::CreateInitialPlayer(FString& OutError)
{
	ULocalPlayer* playerOne = Super::CreateInitialPlayer(OutError);
	
	if (UVertLocalPlayer* vertPlayer = Cast<UVertLocalPlayer>(playerOne))
	{
		vertPlayer->PlayerJoinGame();
	}

	for (int32 i = 1; i < MAX_PLAYERS; ++i)
	{
		CreateLocalPlayer(-1, OutError, false);
	}

	

	return playerOne;
}

void UVertGameInstance::StartRecordingReplay(const FString& InName, const FString& FriendlyName, const TArray<FString>& AdditionalOptions /*= TArray<FString>()*/)
{
	Super::StartRecordingReplay(InName, FriendlyName, AdditionalOptions);
}

void UVertGameInstance::StopRecordingReplay()
{
	Super::StopRecordingReplay();
}

void UVertGameInstance::PlayReplay(const FString& InName, UWorld* WorldOverride /*= nullptr*/, const TArray<FString>& AdditionalOptions /*= TArray<FString>()*/)
{
	Super::PlayReplay(InName, WorldOverride, AdditionalOptions);
}

void UVertGameInstance::AddUserToReplay(const FString& UserString)
{
	Super::AddUserToReplay(UserString);
}

TArray<UVertLocalPlayer*> UVertGameInstance::GetAllLocalPlayers()
{
	TArray<UVertLocalPlayer*> localPlayers;

	for (int32 i = 0; i < GetLocalPlayers().Num(); ++i)
	{
		if (UVertLocalPlayer* vertPlayer = Cast<UVertLocalPlayer>(GetLocalPlayers()[i]))
		{
			localPlayers.Add(vertPlayer);
		}
	}

	return localPlayers;
}

TArray<UVertLocalPlayer*> UVertGameInstance::GetActiveLocalPlayers()
{
	TArray<UVertLocalPlayer*> localPlayers;

	for (int32 i = 0; i < GetLocalPlayers().Num(); ++i)
	{
		if (UVertLocalPlayer* vertPlayer = Cast<UVertLocalPlayer>(GetLocalPlayers()[i]))
		{
			if(vertPlayer->IsPlayerInGame())
				localPlayers.Add(vertPlayer);
		}
	}

	return localPlayers;
}

TArray<UVertLocalPlayer*> UVertGameInstance::GetInactiveLocalPlayers()
{
	TArray<UVertLocalPlayer*> localPlayers;

	for (int32 i = 0; i < GetLocalPlayers().Num(); ++i)
	{
		if (UVertLocalPlayer* vertPlayer = Cast<UVertLocalPlayer>(GetLocalPlayers()[i]))
		{
			if(!vertPlayer->IsPlayerInGame())
				localPlayers.Add(vertPlayer);
		}
	}

	return localPlayers;
}