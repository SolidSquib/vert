// Copyright Inside Out Games Ltd. 2017

#pragma once

#include "GameFramework/PlayerState.h"
#include "VertPlayerState.generated.h"

/**
 * 
 */
UCLASS()
class VERT_API AVertPlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	UPROPERTY(Transient, Replicated)
	int32 ActualDamageTaken = 0;

	UPROPERTY(Transient, Replicated)
	int32 ShownDamageTaken = 0;

	UPROPERTY(Transient, Replicated)
	int32 Lives = -1;

	UPROPERTY(Transient, BlueprintReadOnly, Replicated)
	int32 PlayerIndex = -1;

	UPROPERTY()
	TMap<EScoreTrack, int32> BonusScoreTrackRecord;

protected:
	/** team number */
	UPROPERTY(Transient, ReplicatedUsing = OnRep_TeamColour)
	int32 TeamNumber;

	UPROPERTY(Transient, ReplicatedUsing = OnRep_TeamColour)
	FPlayerColours PlayerColour;

	/** number of kills */
	UPROPERTY(Transient, BlueprintReadOnly, Replicated)
	int32 NumKills;

	UPROPERTY(BlueprintReadOnly)
	int32 NumAssists;

	/** number of deaths */
	UPROPERTY(Transient, BlueprintReadOnly, Replicated)
	int32 NumDeaths;
	
	/** whether the user quit the match */
	UPROPERTY()
	bool Quitter;

public:
	void SetDamageTaken(int32 newDamage, int32 newShownDamage);
	void ScoreKill(AVertPlayerState* victim, int32 points, const TMap<EScoreTrack, int32> bonusPoints);
	void ScoreAssist(AVertPlayerState* victim, int32 points, const TMap<EScoreTrack, int32> bonusPoints);
	void ScoreDeath(AVertPlayerState* perpetrator, int32 points);
	void ScoreSurvivalTime(int32 points);
	void ScoreRace(int32 points);
	void ScoreMonsterHunter(int32 points);
	void ScoreLastManStanding(int32 points);
	void UpdateTeamColours();
	int32 GetActualDamageTaken() const;
	int32 GetShownDamageTaken() const;
	int32 GetCurrentKillStreak() const;
	int32 GetCurrentDeathsWithoutGlory() const;
	void SetPlayerColour(const FPlayerColours& newColour);
	void SetController(class AController* controller);

	virtual void Reset() override;
	virtual void ClientInitialize(class AController* InController) override;

	FORCEINLINE int32 GetTeamNum() const { return TeamNumber; }
	FORCEINLINE int32 GetKills() const { return NumKills; }
	FORCEINLINE int32 GetDeaths() const { return NumDeaths; }
	FORCEINLINE float GetScore() const { return Score; }
	FORCEINLINE bool IsQuitter() const { return Quitter; }

	/** Sends kill (excluding self) to clients */
	UFUNCTION(Reliable, Client)
	void InformAboutKill(class AVertPlayerState* KillerPlayerState, const UDamageType* KillerDamageType, class AVertPlayerState* KilledPlayerState);

	/** broadcast death to local clients */
	UFUNCTION(Reliable, NetMulticast)
	void BroadcastDeath(class AVertPlayerState* KillerPlayerState, const UDamageType* KillerDamageType, class AVertPlayerState* KilledPlayerState);

	/** replicate team colors. Update the players mesh colors appropriately */
	UFUNCTION()
	void OnRep_TeamColour();

	UFUNCTION(BlueprintCallable, Category = "Colours")
	FPlayerColours GetPlayerColours() const { return PlayerColour; }

	UFUNCTION(BlueprintCallable, Category = "Controller")
	AController* GetController() const;

	UFUNCTION(BlueprintCallable, Category = "Controller")
	AVertPlayerController* GetVertController() const;

	UFUNCTION(BlueprintCallable, Category = "Score")
	int32 GetScoreForBonusTrack(EScoreTrack Track) const;

	UFUNCTION(BlueprintCallable, Category = "Score")
	bool IsLastManStanding() const;

	UFUNCTION(BlueprintCallable, Category = "Score")
	bool IsHighestScoringOverall() const;

	UFUNCTION(BlueprintCallable, Category = "Score")
	bool IsHighestScoringBonusTrack(EScoreTrack Track) const;

	UFUNCTION(BlueprintCallable, Category = "Score")
	bool HasMostKills() const;

	UFUNCTION(BlueprintCallable, Category = "Score")
	bool HasLeastDeaths() const;

	UFUNCTION(BlueprintCallable, Category = "Score")
	bool HasMostAssists() const;

	UFUNCTION(BlueprintCallable, Category = "Score")
	int32 GetBestKillingSpree() const { return mBestKillingSpree; }

protected:
	void ScorePoints(int32 points);
	void RecordBonusPoints(EScoreTrack track, int32 points);

private:
	TWeakObjectPtr<AController> mController = nullptr;
	int32 mCurrentKillStreak = 0;
	int32 mDeathsWithoutGlory = 0;
	int32 mBestKillingSpree = 0;
	bool mIsLastManStanding = false;
};
