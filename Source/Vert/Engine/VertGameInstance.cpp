// Copyright Inside Out Games Ltd. 2017

#include "VertGameInstance.h"
#include "Vert.h"
#include "View/VertViewportClient.h"
#include <MoviePlayer.h>

//************************************
// Method:    IsControllerIDAvailable
// FullName:  UVertGameInstance::IsControllerIDAvailable
// Access:    public 
// Returns:   bool
// Qualifier:
// Parameter: const int32 & id
//************************************
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

//************************************
// Method:    Init
// FullName:  UVertGameInstance::Init
// Access:    virtual public 
// Returns:   void
// Qualifier:
//************************************
void UVertGameInstance::Init()
{
	Super::Init();

	FCoreUObjectDelegates::PreLoadMap.AddUObject(this, &UVertGameInstance::BeginLoadingScreen);
	FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &UVertGameInstance::EndLoadingScreen);
}

//************************************
// Method:    Shutdown
// FullName:  UVertGameInstance::Shutdown
// Access:    virtual public 
// Returns:   void
// Qualifier:
//************************************
void UVertGameInstance::Shutdown()
{
	Super::Shutdown();
}

//************************************
// Method:    CreateInitialPlayer
// FullName:  UVertGameInstance::CreateInitialPlayer
// Access:    virtual public 
// Returns:   ULocalPlayer*
// Qualifier:
// Parameter: FString & OutError
//************************************
ULocalPlayer* UVertGameInstance::CreateInitialPlayer(FString& OutError)
{
	ULocalPlayer* playerOne = Super::CreateInitialPlayer(OutError);
 	return playerOne;
}

//************************************
// Method:    StartRecordingReplay
// FullName:  UVertGameInstance::StartRecordingReplay
// Access:    virtual public 
// Returns:   void
// Qualifier:
// Parameter: const FString & InName
// Parameter: const FString & FriendlyName
// Parameter: const TArray<FString> & AdditionalOptions
//************************************
void UVertGameInstance::StartRecordingReplay(const FString& InName, const FString& FriendlyName, const TArray<FString>& AdditionalOptions /*= TArray<FString>()*/)
{
	Super::StartRecordingReplay(InName, FriendlyName, AdditionalOptions);
}

//************************************
// Method:    StopRecordingReplay
// FullName:  UVertGameInstance::StopRecordingReplay
// Access:    virtual public 
// Returns:   void
// Qualifier:
//************************************
void UVertGameInstance::StopRecordingReplay()
{
	Super::StopRecordingReplay();
}

//************************************
// Method:    PlayReplay
// FullName:  UVertGameInstance::PlayReplay
// Access:    virtual public 
// Returns:   void
// Qualifier:
// Parameter: const FString & InName
// Parameter: UWorld * WorldOverride
// Parameter: const TArray<FString> & AdditionalOptions
//************************************
void UVertGameInstance::PlayReplay(const FString& InName, UWorld* WorldOverride /*= nullptr*/, const TArray<FString>& AdditionalOptions /*= TArray<FString>()*/)
{
	Super::PlayReplay(InName, WorldOverride, AdditionalOptions);
}

//************************************
// Method:    AddUserToReplay
// FullName:  UVertGameInstance::AddUserToReplay
// Access:    virtual public 
// Returns:   void
// Qualifier:
// Parameter: const FString & UserString
//************************************
void UVertGameInstance::AddUserToReplay(const FString& UserString)
{
	Super::AddUserToReplay(UserString);
}

//************************************
// Method:    RemoveSplitScreenPlayers
// FullName:  UVertGameInstance::RemoveSplitScreenPlayers
// Access:    public 
// Returns:   void
// Qualifier:
//************************************
void UVertGameInstance::RemoveSplitScreenPlayers()
{
	// if we had been split screen, toss the extra players now
	// remove every player, back to front, except the first one
	while (LocalPlayers.Num() > 1)
	{
		ULocalPlayer* const playerToRemove = LocalPlayers.Last();
		RemoveExistingLocalPlayer(playerToRemove);
	}
}

//************************************
// Method:    RemoveExistingLocalPlayer
// FullName:  UVertGameInstance::RemoveExistingLocalPlayer
// Access:    public 
// Returns:   void
// Qualifier:
// Parameter: ULocalPlayer * existingPlayer
//************************************
void UVertGameInstance::RemoveExistingLocalPlayer(ULocalPlayer* existingPlayer)
{
	check(existingPlayer);
	if (existingPlayer->PlayerController != NULL)
	{
		// Kill the player
		AVertCharacter* pawn = Cast<AVertCharacter>(existingPlayer->PlayerController->GetPawn());
		if (pawn)
		{
			pawn->KilledBy(NULL);
		}
	}

	// Remove local split-screen players from the list
	RemoveLocalPlayer(existingPlayer);
}

//************************************
// Method:    GetVertLocalPlayers
// FullName:  UVertGameInstance::GetVertLocalPlayers
// Access:    public 
// Returns:   TArray<UVertLocalPlayer*>
// Qualifier:
//************************************
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

//************************************
// Method:    BeginLoadingScreen
// FullName:  UVertGameInstance::BeginLoadingScreen
// Access:    virtual public 
// Returns:   void
// Qualifier:
// Parameter: const FString & mapName
//************************************
void UVertGameInstance::BeginLoadingScreen(const FString& mapName)
{
	if (!IsRunningDedicatedServer())
	{
		FLoadingScreenAttributes loadingScreen;
		loadingScreen.bAutoCompleteWhenLoadingCompletes = false;
		loadingScreen.WidgetLoadingScreen = FLoadingScreenAttributes::NewTestLoadingScreenWidget();

		GetMoviePlayer()->SetupLoadingScreen(loadingScreen);
	}
}

//************************************
// Method:    EndLoadingScreen
// FullName:  UVertGameInstance::EndLoadingScreen
// Access:    virtual public 
// Returns:   void
// Qualifier:
// Parameter: UWorld * world
//************************************
void UVertGameInstance::EndLoadingScreen(UWorld* world)
{
// 	constexpr float cFadeTime = 1.5f;
// 
// 	if(UVertViewportClient* client = Cast<UVertViewportClient>(GetGameViewportClient()))
// 	{
// 		client->Fade(cFadeTime, false);
// 	}
}