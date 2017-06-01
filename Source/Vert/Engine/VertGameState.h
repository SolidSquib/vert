// Copyright Inside Out Games Ltd. 2017

#pragma once

#include "GameFramework/GameState.h"
#include "VertGameState.generated.h"

/** ranked PlayerState map, created from the GameState */
typedef TMap<int32, TWeakObjectPtr<AVertPlayerState>> RankedPlayerMap;

UCLASS()
class VERT_API AVertGameState : public AGameState
{
	GENERATED_UCLASS_BODY()
	
public:
	/** number of teams in current game (doesn't deprecate when no players are left in a team) */
	UPROPERTY(Transient, Replicated)
	int32 NumTeams;

	/** accumulated score per team */
	UPROPERTY(Transient, Replicated)
	TArray<int32> TeamScores;

	/** time left for warmup / match */
	UPROPERTY(Transient, Replicated)
	int32 RemainingTime;

	/** is timer paused? */
	UPROPERTY(Transient, Replicated)
	bool TimerPaused;

public:
	void GetRankedMap(int32 TeamIndex, RankedPlayerMap& OutRankedMap) const; /** gets ranked PlayerState map for specific team */
	void RequestFinishAndExitToMainMenu();
};
