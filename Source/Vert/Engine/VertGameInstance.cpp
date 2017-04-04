// Copyright Inside Out Games Ltd. 2017

#include "Vert.h"
#include "VertGameInstance.h"

bool UVertGameInstance::IsControllerIDAvailable(const int32& id)
{
	const TArray<ULocalPlayer*>& players = GetLocalPlayers();

	for (auto i = 0; i < players.Num(); ++i)
	{
		if (players[i]->GetControllerId() == id)
			return false;
	}

	return true;
}

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

TArray<UVertLocalPlayer*> UVertGameInstance::GetVertLocalPlayers()
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