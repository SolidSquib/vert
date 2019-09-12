// Copyright Inside Out Games Ltd. 2017

#include "VertPlayerState.h"
#include "Vert.h"

#define SHOW_SCORE_DEBUG 1

DECLARE_LOG_CATEGORY_CLASS(LogVertPlayerState, Log, All);

static FColor sTeamColours[] = {
	FColor::Blue,
	FColor::Emerald,
	FColor::Red,
	FColor::Yellow
};

void AVertPlayerState::SetController(class AController* controller)
{
	if (controller)
	{
		mController = controller;
	}
}

AController* AVertPlayerState::GetController() const
{
	if (mController.IsValid())
		return mController.Get();

	return nullptr;
}

AVertPlayerController* AVertPlayerState::GetVertController() const
{
	if (mController.IsValid())
	{
		if (AVertPlayerController* vpc = Cast<AVertPlayerController>(mController.Get()))
			return vpc;
	}

	return nullptr;
}

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

int32 AVertPlayerState::GetCurrentKillStreak() const
{
	return mCurrentKillStreak;
}

int32 AVertPlayerState::GetCurrentDeathsWithoutGlory() const
{
	return mDeathsWithoutGlory;
}

int32 AVertPlayerState::GetScoreForBonusTrack(EScoreTrack Track) const
{
	if (BonusScoreTrackRecord.Contains(Track))
	{
		return BonusScoreTrackRecord[Track];
	}

	return 0;
}

bool AVertPlayerState::IsLastManStanding() const
{
	return mIsLastManStanding;
}

bool AVertPlayerState::IsHighestScoringOverall() const
{
	bool result = true;
	AGameState* GameState = GetWorld()->GetGameState<AGameState>();
	if (GameState)
	{
		for (auto iter = GameState->PlayerArray.CreateConstIterator(); iter; ++iter)
		{
			APlayerState* PlayerState = *iter;
			if (PlayerState && PlayerState != this && Score < PlayerState->Score)
			{
				result = false;
				break;
			}
		}
	}

	return result;
}

bool AVertPlayerState::IsHighestScoringBonusTrack(EScoreTrack Track) const
{
	bool result = true;
	AGameState* GameState = GetWorld()->GetGameState<AGameState>();
	if (GameState)
	{
		for (auto iter = GameState->PlayerArray.CreateConstIterator(); iter; ++iter)
		{
			AVertPlayerState* PlayerState = Cast<AVertPlayerState>(*iter);
			if (PlayerState && PlayerState != this && GetScoreForBonusTrack(Track) < PlayerState->GetScoreForBonusTrack(Track))
			{
				result = false;
				break;
			}
		}
	}

	return result;
}

bool AVertPlayerState::HasMostKills() const
{
	bool result = true;
	AGameState* GameState = GetWorld()->GetGameState<AGameState>();
	if (GameState)
	{
		for (auto iter = GameState->PlayerArray.CreateConstIterator(); iter; ++iter)
		{
			AVertPlayerState* PlayerState = Cast<AVertPlayerState>(*iter);
			if (PlayerState && PlayerState != this && NumKills < PlayerState->NumKills)
			{
				result = false;
				break;
			}
		}
	}

	return result;
}

bool AVertPlayerState::HasLeastDeaths() const
{
	bool result = true;
	AGameState* GameState = GetWorld()->GetGameState<AGameState>();
	if (GameState)
	{
		for (auto iter = GameState->PlayerArray.CreateConstIterator(); iter; ++iter)
		{
			AVertPlayerState* PlayerState = Cast<AVertPlayerState>(*iter);
			if (PlayerState && PlayerState != this && NumDeaths > PlayerState->NumDeaths)
			{
				result = false;
				break;
			}
		}
	}

	return result;
}

bool AVertPlayerState::HasMostAssists() const 
{
	bool result = true;
	AGameState* GameState = GetWorld()->GetGameState<AGameState>();
	if (GameState)
	{
		for (auto iter = GameState->PlayerArray.CreateConstIterator(); iter; ++iter)
		{
			AVertPlayerState* PlayerState = Cast<AVertPlayerState>(*iter);
			if (PlayerState && PlayerState != this && NumAssists < PlayerState->NumAssists)
			{
				result = false;
				break;
			}
		}
	}

	return result;
}

void AVertPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AVertPlayerState, ActualDamageTaken);
	DOREPLIFETIME(AVertPlayerState, ShownDamageTaken);
	DOREPLIFETIME(AVertPlayerState, Lives);
}

void AVertPlayerState::ScoreKill(AVertPlayerState* victim, int32 points, const TMap<EScoreTrack, int32> bonusPoints)
{
	NumKills++;
	mCurrentKillStreak++;
	mDeathsWithoutGlory = 0;

	if (mCurrentKillStreak > mBestKillingSpree)
	{
		mBestKillingSpree = mCurrentKillStreak;
	}

	int32 totalPointsEarned = points;

	for (auto it = bonusPoints.CreateConstIterator(); it; ++it)
	{
		totalPointsEarned += it.Value();
		RecordBonusPoints(it.Key(), it.Value());
	}

#if SHOW_SCORE_DEBUG
	GEngine->AddOnScreenDebugMessage(-1, 5.f, PlayerColour.PrimaryColour.ToFColor(true), FString::Printf(TEXT("Player scored for KILL: %i points"), points));
#endif

	ScorePoints(totalPointsEarned);
}

void AVertPlayerState::ScoreAssist(AVertPlayerState* victim, int32 points, const TMap<EScoreTrack, int32> bonusPoints)
{
	NumAssists++;
	mDeathsWithoutGlory = 0;

	int32 totalPointsEarned = points;

	for (auto it = bonusPoints.CreateConstIterator(); it; ++it)
	{
		totalPointsEarned += it.Value();
		RecordBonusPoints(it.Key(), it.Value());
	}

#if SHOW_SCORE_DEBUG
	GEngine->AddOnScreenDebugMessage(-1, 5.f, PlayerColour.PrimaryColour.ToFColor(true), FString::Printf(TEXT("Player scored for ASSIST: %i points"), points));
#endif

	ScorePoints(totalPointsEarned);
}

void AVertPlayerState::ScoreDeath(AVertPlayerState* killer, int32 points)
{
	NumDeaths++;

	if (mCurrentKillStreak <= 0)
	{
		mDeathsWithoutGlory++;
	}

	mCurrentKillStreak = 0;
	ScorePoints(points);
}

void AVertPlayerState::ScoreSurvivalTime(int32 points)
{
	if (mController.IsValid() && !mController->IsPendingKill() && mController->GetPawn() && !mController->GetPawn()->IsPendingKill())
	{
		RecordBonusPoints(EScoreTrack::SCORE_Survivor, points);
		ScorePoints(points);
	}
}

void AVertPlayerState::ScoreRace(int32 points)
{
	if (!BonusScoreTrackRecord.Contains(EScoreTrack::SCORE_SpeedRacer))
	{
		BonusScoreTrackRecord.Add(EScoreTrack::SCORE_SpeedRacer, 0);
	}

#if SHOW_SCORE_DEBUG
	GEngine->AddOnScreenDebugMessage(-1, 5.f, PlayerColour.PrimaryColour.ToFColor(true), FString::Printf(TEXT("Player scored for SPEED RACER: %i points"), points));
#endif

	BonusScoreTrackRecord[EScoreTrack::SCORE_SpeedRacer] += points;
	ScorePoints(points);
}

void AVertPlayerState::ScoreMonsterHunter(int32 points)
{
	if (!BonusScoreTrackRecord.Contains(EScoreTrack::SCORE_MonsterHunter))
	{
		BonusScoreTrackRecord.Add(EScoreTrack::SCORE_MonsterHunter, 0);
	}

#if SHOW_SCORE_DEBUG
	GEngine->AddOnScreenDebugMessage(-1, 5.f, PlayerColour.PrimaryColour.ToFColor(true), FString::Printf(TEXT("Player scored for MONSTER HUNTER: %i points"), points));
#endif

	BonusScoreTrackRecord[EScoreTrack::SCORE_MonsterHunter] += points;
	ScorePoints(points);
}

void AVertPlayerState::ScoreLastManStanding(int32 points)
{
	mIsLastManStanding = true;
	Score += points;

#if SHOW_SCORE_DEBUG
	GEngine->AddOnScreenDebugMessage(-1, 5.f, PlayerColour.PrimaryColour.ToFColor(true), FString::Printf(TEXT("Player scored for LAST MAN STANDING: %i points"), points));
#endif
}

void AVertPlayerState::SetPlayerColour(const FPlayerColours& newColour)
{
	PlayerColour = newColour;

	UpdateTeamColours();
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

//************************************
// Method:    RecordBonusPoints
// FullName:  AVertPlayerState::RecordBonusPoints
// Access:    protected 
// Returns:   void
// Qualifier:
// Parameter: EScoreTrack track
// Parameter: int32 points
//************************************
void AVertPlayerState::RecordBonusPoints(EScoreTrack track, int32 points)
{
	if (BonusScoreTrackRecord.Contains(track))
	{
		BonusScoreTrackRecord[track] += points;
	}
	else
	{
		BonusScoreTrackRecord.Add(track, points);
	}

#if SHOW_SCORE_DEBUG
	switch (track)
	{
	case EScoreTrack::SCORE_BountyHunter:
		GEngine->AddOnScreenDebugMessage(-1, 5.f, PlayerColour.PrimaryColour.ToFColor(true), FString::Printf(TEXT("Player scored for BOUNTY HUNTER: %i points"), points));
		break;
	case EScoreTrack::SCORE_HeavyHitter:
		GEngine->AddOnScreenDebugMessage(-1, 5.f, PlayerColour.PrimaryColour.ToFColor(true), FString::Printf(TEXT("Player scored for HEAVY HITTER: %i points"), points));
		break;
	case EScoreTrack::SCORE_KillingSpree:
		GEngine->AddOnScreenDebugMessage(-1, 5.f, PlayerColour.PrimaryColour.ToFColor(true), FString::Printf(TEXT("Player scored for KILLING SPREE: %i points"), points));
		break;
	case EScoreTrack::SCORE_GlassCannon:
		GEngine->AddOnScreenDebugMessage(-1, 5.f, PlayerColour.PrimaryColour.ToFColor(true), FString::Printf(TEXT("Player scored for GLASS CANNON: %i points"), points));
		break;
	case EScoreTrack::SCORE_MasterOfArms:
		GEngine->AddOnScreenDebugMessage(-1, 5.f, PlayerColour.PrimaryColour.ToFColor(true), FString::Printf(TEXT("Player scored for MASTER OF ARMS: %i points"), points));
		break;
	case EScoreTrack::SCORE_RubberBand:
		GEngine->AddOnScreenDebugMessage(-1, 5.f, PlayerColour.PrimaryColour.ToFColor(true), FString::Printf(TEXT("Player scored for UNDERDOG: %i points"), points));
		break;
	default:
		break;
	}
#endif
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

//************************************
// Method:    OnRep_TeamColour
// FullName:  AVertPlayerState::OnRep_TeamColour
// Access:    public 
// Returns:   void
// Qualifier:
//************************************
void AVertPlayerState::OnRep_TeamColour()
{
	UpdateTeamColours();
}

//************************************
// Method:    UpdateTeamColours
// FullName:  AVertPlayerState::UpdateTeamColours
// Access:    public 
// Returns:   void
// Qualifier:
//************************************
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

//************************************
// Method:    Reset
// FullName:  AVertPlayerState::Reset
// Access:    virtual public 
// Returns:   void
// Qualifier:
//************************************
void AVertPlayerState::Reset()
{
	Super::Reset();

	//PlayerStates persist across seamless travel.  Keep the same teams as previous match.
	NumKills = 0;
	NumAssists = 0;
	NumDeaths = 0;
	ActualDamageTaken = 0;
	ShownDamageTaken = 0;
	Quitter = false;
	BonusScoreTrackRecord.Empty();
	mCurrentKillStreak = 0;
	mDeathsWithoutGlory = 0;
	mBestKillingSpree = 0;
}

//************************************
// Method:    ClientInitialize
// FullName:  AVertPlayerState::ClientInitialize
// Access:    virtual public 
// Returns:   void
// Qualifier:
// Parameter: AController * InController
//************************************
void AVertPlayerState::ClientInitialize(AController* InController)
{
	Super::ClientInitialize(InController);

	UpdateTeamColours();
}