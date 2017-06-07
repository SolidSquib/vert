// Copyright Inside Out Games Ltd. 2017

#include "VertGameState.h"
#include "Vert.h"
#include "VertPlayerState.h"
#include "VertGameInstance.h"

AVertGameState::AVertGameState(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	NumTeams = 0;
	RemainingTime = 0;
	TimerPaused = false;
}

//************************************
// Method:    GetLifetimeReplicatedProps
// FullName:  AVertGameState::GetLifetimeReplicatedProps
// Access:    public 
// Returns:   void
// Qualifier: const
// Parameter: TArray< FLifetimeProperty > & OutLifetimeProps
//************************************
void AVertGameState::GetLifetimeReplicatedProps(TArray< FLifetimeProperty > & OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AVertGameState, NumTeams);
	DOREPLIFETIME(AVertGameState, RemainingTime);
	DOREPLIFETIME(AVertGameState, TimerPaused);
	DOREPLIFETIME(AVertGameState, TeamScores);
}

//************************************
// Method:    GetRankedMap
// FullName:  AVertGameState::GetRankedMap
// Access:    public 
// Returns:   void
// Qualifier: const
// Parameter: int32 teamIndex
// Parameter: RankedPlayerMap & outRankedMap
//************************************
void AVertGameState::GetRankedMap(int32 teamIndex, RankedPlayerMap& outRankedMap) const
{
	outRankedMap.Empty();

	//first, we need to go over all the PlayerStates, grab their score, and rank them
	TMultiMap<int32, AVertPlayerState*> sortedMap;
	for (int32 i = 0; i < PlayerArray.Num(); ++i)
	{
		int32 score = 0;
		AVertPlayerState* currentPlayerState = Cast<AVertPlayerState>(PlayerArray[i]);
		if (currentPlayerState && (currentPlayerState->GetTeamNum() == teamIndex))
		{
			sortedMap.Add(FMath::TruncToInt(currentPlayerState->Score), currentPlayerState);
		}
	}

	//sort by the keys
	sortedMap.KeySort(TGreater<int32>());

	//now, add them back to the ranked map
	outRankedMap.Empty();

	int32 Rank = 0;
	for (TMultiMap<int32, AVertPlayerState*>::TIterator it(sortedMap); it; ++it)
	{
		outRankedMap.Add(Rank++, it.Value());
	}

}

//************************************
// Method:    RequestFinishAndExitToMainMenu
// FullName:  AVertGameState::RequestFinishAndExitToMainMenu
// Access:    public 
// Returns:   void
// Qualifier:
//************************************
void AVertGameState::RequestFinishAndExitToMainMenu()
{
	if (AuthorityGameMode)
	{
		// we are server, tell the gamemode
		AVertGameMode* const gameMode = Cast<AVertGameMode>(AuthorityGameMode);
		if (gameMode)
		{
			gameMode->RequestFinishAndExitToMainMenu();
		}
	}
	else
	{
		// we are client, handle our own business
		UVertGameInstance* gameInstance = Cast<UVertGameInstance>(GetGameInstance());
		if (gameInstance)
		{
			gameInstance->RemoveSplitScreenPlayers();
		}

		AVertPlayerController* const primaryPC = Cast<AVertPlayerController>(GetGameInstance()->GetFirstLocalPlayerController());
		if (primaryPC)
		{
			check(primaryPC->GetNetMode() == ENetMode::NM_Client);
			primaryPC->HandleReturnToMainMenu();
		}
	}
}
