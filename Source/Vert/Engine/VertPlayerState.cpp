// Copyright Inside Out Games Ltd. 2017

#include "VertPlayerState.h"
#include "Vert.h"

static FColor sTeamColours[] = {
	FColor::Blue,
	FColor::Emerald,
	FColor::Red,
	FColor::Yellow
};

void AVertPlayerState::SetDamageTaken(int32 newDamage, int32 newShownDamage)
{
	ActualDamageTaken = newDamage;
	ShownDamageTaken = newShownDamage;
}

int32 AVertPlayerState::GetActualDamageTaken() const 
{
	return ActualDamageTaken;
}

int32 AVertPlayerState::GetShownDamageTaken() const 
{
	return ShownDamageTaken;
}

void AVertPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AVertPlayerState, ActualDamageTaken);
	DOREPLIFETIME(AVertPlayerState, ShownDamageTaken);
	DOREPLIFETIME(AVertPlayerState, Lives);
}

void AVertPlayerState::ScoreKill(AVertPlayerState* victim, int32 points)
{
	NumKills++;
	ScorePoints(points);
}

void AVertPlayerState::ScoreDeath(AVertPlayerState* perpetrator, int32 points)
{
	NumDeaths++;
	ScorePoints(points);
}

void AVertPlayerState::ScorePoints(int32 points)
{
	AVertGameState* const state = GetWorld()->GetGameState<AVertGameState>();
	if (state && TeamNumber >= 0)
	{
		if (TeamNumber >= state->TeamScores.Num())
		{
			state->TeamScores.AddZeroed(TeamNumber - state->TeamScores.Num() + 1);
		}

		state->TeamScores[TeamNumber] += points;
	}

	Score += points;
}

void AVertPlayerState::InformAboutKill_Implementation(class AVertPlayerState* killerPlayerState, const UDamageType* killerDamageType, class AVertPlayerState* killedPlayerState)
{
	//id can be null for bots
	if (killerPlayerState->UniqueId.IsValid())
	{
		//search for the actual killer before calling OnKill()	
		for (FConstPlayerControllerIterator it = GetWorld()->GetPlayerControllerIterator(); it; ++it)
		{
			AVertPlayerController* pc = Cast<AVertPlayerController>(*it);
			if (pc && pc->IsLocalController())
			{
				// a local player might not have an ID if it was created with CreateDebugPlayer.
				ULocalPlayer* localPlayer = Cast<ULocalPlayer>(pc->Player);
				TSharedPtr<const FUniqueNetId> localID = localPlayer->GetCachedUniqueNetId();
				if (localID.IsValid() && *localPlayer->GetCachedUniqueNetId() == *killerPlayerState->UniqueId)
				{
					pc->OnKill();
				}
			}
		}
	}
}

void AVertPlayerState::BroadcastDeath_Implementation(class AVertPlayerState* KillerPlayerState, const UDamageType* KillerDamageType, class AVertPlayerState* KilledPlayerState)
{
// 	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
// 	{
// 		// all local players get death messages so they can update their huds.
// 		AVertPlayerController* testPC = Cast<AVertPlayerController>(*It);
// 		if (testPC && testPC->IsLocalController())
// 		{
// 			testPC->OnDeathMessage(KillerPlayerState, this, KillerDamageType);
// 		}
// 	}
}

void AVertPlayerState::OnRep_TeamColour()
{
	UpdateTeamColours();
}

void AVertPlayerState::UpdateTeamColours()
{
	AController* ownerController = Cast<AController>(GetOwner());
	if (ownerController != NULL)
	{
		AVertCharacter* character = Cast<AVertCharacter>(ownerController->GetCharacter());
		if (character != NULL)
		{
			character->UpdateTeamColoursAllMIDs();
		}
	}
}