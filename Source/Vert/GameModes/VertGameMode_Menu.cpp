// Fill out your copyright notice in the Description page of Project Settings.

#include "VertGameMode_Menu.h"
#include "Engine/VertPlayerController_Menu.h"
#include "Online/VertGameSession.h"

AVertGameMode_Menu::AVertGameMode_Menu(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	PlayerControllerClass = AVertPlayerController_Menu::StaticClass();
	GameStateClass = AVertGameState::StaticClass();
	PlayerStateClass = AVertPlayerState::StaticClass();
}

//************************************
// Method:    RestartPlayer
// FullName:  AVertGameMode_Menu::RestartPlayer
// Access:    virtual public 
// Returns:   void
// Qualifier:
// Parameter: class AController * NewPlayer
//************************************
void AVertGameMode_Menu::RestartPlayer(class AController* NewPlayer)
{
	// don't restart
}

/** Returns game session class to use */
//************************************
// Method:    GetGameSessionClass
// FullName:  AVertGameMode_Menu::GetGameSessionClass
// Access:    virtual public 
// Returns:   TSubclassOf<AGameSession>
// Qualifier: const
//************************************
TSubclassOf<AGameSession> AVertGameMode_Menu::GetGameSessionClass() const
{
	return AVertSession::StaticClass();
}

//************************************
// Method:    StartPlay
// FullName:  AVertGameMode_Menu::StartPlay
// Access:    virtual public 
// Returns:   void
// Qualifier:
//************************************
void AVertGameMode_Menu::StartPlay()
{
	Super::StartPlay();

	if (!MainMenuWidget && MainMenuWidgetClass != nullptr)
	{
		MainMenuWidget = CreateWidget<UUserWidget>(GetWorld(), MainMenuWidgetClass);
		if (MainMenuWidget)
		{
			MainMenuWidget->AddToViewport();
			MainMenuWidget->SetVisibility(ESlateVisibility::Visible);
		}
	}

	UVertUtilities::FadeFromBlack(this, FadeInTime);
}

//************************************
// Method:    PostLogin
// FullName:  AVertGameMode_Menu::PostLogin
// Access:    virtual public 
// Returns:   void
// Qualifier:
// Parameter: APlayerController * NewPlayer
//************************************
void AVertGameMode_Menu::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	AVertPlayerState* newPlayerState = Cast<AVertPlayerState>(NewPlayer->PlayerState);
	if (newPlayerState)
	{
		newPlayerState->SetController(NewPlayer);

		if (newPlayerState->PlayerIndex == -1) // New player with index not set
		{
			TArray<int32> availableIndices = { 0, 1, 2, 3 };
			for (int32 i = 0; i < GameState->PlayerArray.Num(); ++i)
			{
				if (AVertPlayerState* ps = Cast<AVertPlayerState>(GameState->PlayerArray[i]))
				{
					if (availableIndices.Find(ps->PlayerIndex) != INDEX_NONE)
					{
						availableIndices.Remove(ps->PlayerIndex);
					}
				}
			}

			if (availableIndices.Num() >= 1)
				newPlayerState->PlayerIndex = availableIndices[0];
		}

		AVertWorldSettings* worldSettings = Cast<AVertWorldSettings>(GetWorldSettings());
		if (worldSettings && newPlayerState->PlayerIndex >= 0 && newPlayerState->PlayerIndex < worldSettings->PlayerColours.Num())
			newPlayerState->SetPlayerColour(worldSettings->PlayerColours[newPlayerState->PlayerIndex]);
	}

	OnPlayerLoggedIn.Broadcast(NewPlayer);
}

//************************************
// Method:    Logout
// FullName:  AVertGameMode_Menu::Logout
// Access:    virtual public 
// Returns:   void
// Qualifier:
// Parameter: AController * Exiting
//************************************
void AVertGameMode_Menu::Logout(AController* Exiting)
{
	Super::Logout(Exiting);
	OnPlayerLoggedOut.Broadcast(Exiting);
}