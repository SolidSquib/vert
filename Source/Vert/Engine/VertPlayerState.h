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

protected:
	/** team number */
	UPROPERTY(Transient, ReplicatedUsing = OnRep_TeamColour)
	int32 TeamNumber;

	/** number of kills */
	UPROPERTY(Transient, Replicated)
	int32 NumKills;

	/** number of deaths */
	UPROPERTY(Transient, Replicated)
	int32 NumDeaths;
	
	/** whether the user quit the match */
	UPROPERTY()
	bool Quitter;

public:
	void SetDamageTaken(int32 newDamage, int32 newShownDamage);
	void ScoreKill(AVertPlayerState* victim, int32 points);
	void ScoreDeath(AVertPlayerState* perpetrator, int32 points);
	void UpdateTeamColours();
	int32 GetActualDamageTaken() const;
	int32 GetShownDamageTaken() const;

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

	/** replicate team colors. Updated the players mesh colors appropriately */
	UFUNCTION()
	void OnRep_TeamColour();

protected:
	void ScorePoints(int32 points);
};
