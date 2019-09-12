// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "VertGameMode.h"
#include "VertCharacter.h"
#include "Engine/VertPlayerState.h"

DEFINE_LOG_CATEGORY(LogVertGameMode);

namespace MatchState
{
	const FName IntroducingMatch = FName(TEXT("IntroducingMatch"));
	const FName InProgressFinale = FName(TEXT("InProgressFinale"));
}

AVertGameMode::AVertGameMode()
{
	// set default pawn class to our character
	HUDClass = AVertHUD::StaticClass();
	DefaultPawnClass = AVertCharacter::StaticClass();

	PlayerColours.Add(FPlayerColours(FLinearColor::Blue, FLinearColor::Gray, FLinearColor::Gray));
	PlayerColours.Add(FPlayerColours(FLinearColor::Green, FLinearColor::Gray, FLinearColor::Gray));
	PlayerColours.Add(FPlayerColours(FLinearColor::Red, FLinearColor::Gray, FLinearColor::Gray));
	PlayerColours.Add(FPlayerColours(FLinearColor::Yellow, FLinearColor::Gray, FLinearColor::Gray));
	
	bDelayedStart = true;
}

//************************************
// Method:    BeginPlay
// FullName:  AVertGameMode::BeginPlay
// Access:    virtual public 
// Returns:   void
// Qualifier:
//************************************
void AVertGameMode::BeginPlay()
{
	Super::BeginPlay();
	mOnControllerChangedHandle = FCoreDelegates::OnControllerConnectionChange.AddUFunction(this, TEXT("OnControllerConnectionChange"));
}

//************************************
// Method:    InitGame
// FullName:  AVertGameMode::InitGame
// Access:    virtual public 
// Returns:   void
// Qualifier:
// Parameter: const FString & MapName
// Parameter: const FString & Options
// Parameter: FString & ErrorMessage
//************************************
void AVertGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	UVertViewportClient* viewport = Cast<UVertViewportClient>(GetWorld()->GetGameViewport());
	if (viewport) // Attempt to start the level as a black screen to hide all the set up shit.
	{
		viewport->Fade(0, true);
	}

	Super::InitGame(MapName, Options, ErrorMessage);
}

//************************************
// Method:    PreInitializeComponents
// FullName:  AVertGameMode::PreInitializeComponents
// Access:    virtual public 
// Returns:   void
// Qualifier:
//************************************
void AVertGameMode::PreInitializeComponents()
{
	Super::PreInitializeComponents();
	GetWorldTimerManager().SetTimer(mTimerHandle_DefaultTimer, this, &AVertGameMode::DefaultTimer, GetWorldSettings()->GetEffectiveTimeDilation(), true);
}

//************************************
// Method:    PostInitializeComponents
// FullName:  AVertGameMode::PostInitializeComponents
// Access:    virtual public 
// Returns:   void
// Qualifier:
//************************************
void AVertGameMode::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	CreateObjectPools();
}

//************************************
// Method:    DefaultTimer
// FullName:  AVertGameMode::DefaultTimer
// Access:    protected 
// Returns:   void
// Qualifier:
//************************************
void AVertGameMode::DefaultTimer()
{
	if (GetMatchState() == MatchState::WaitingToStart)
	{
		if (mPlayerCamera.IsValid())
		{
			UVertUtilities::FadeFromBlack(this, 0.5f);
			HandleIntroductionHasStarted();
		}
	}
	else if (GetMatchState() == MatchState::IntroducingMatch)
	{
		if (mCurrentIntroDone || mCurrentIntroPlayerIndex == 0)
		{
			if (GetWorld()->IsPlayInEditor() && !ShowStartupInEditor)
			{
				StartMatch();
			}
			else
			{
				if (mCurrentIntroPlayerIndex >= GetGameState<AVertGameState>()->PlayerArray.Num())
				{
					for (FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator)
					{
						APlayerController* PlayerController = Iterator->Get();
						FViewTargetTransitionParams params;
						params.BlendTime = 0.5f;
						params.BlendFunction = VTBlend_Linear;
						params.BlendExp = 0;
						params.bLockOutgoing = false;
						PlayerController->ClientSetViewTarget(mPlayerCamera.Get(), params);
					}

					StartMatch();
				}
				else
				{
					mCurrentIntroDone = false;
					IntroducePlayer(mCurrentIntroPlayerIndex++);
				}
			}			
		}
	}
	else if (GetMatchState() == MatchState::InProgressFinale)
	{
		int32 PlayersRemaining = 0;
		for (FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator)
		{
			APlayerController* PlayerController = Iterator->Get();
			if (PlayerController->GetPawn() && !PlayerController->GetPawn()->IsPendingKill())
			{
				PlayersRemaining += 1;
			}
		}

		if (PlayersRemaining <= 1)
		{
			FinishMatch();
		}
	}
	else if (GetMatchState() == MatchState::WaitingPostMatch)
	{
		bool ReadyToLeave = true;
		for (FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator)
		{
			AVertPlayerController* PlayerController = Cast<AVertPlayerController>(Iterator->Get());
			if (PlayerController && !PlayerController->GetPlayerWantsToLeave())
			{
				ReadyToLeave = false;
			}
		}

		if (ReadyToLeave)
		{
			AVertGameState* GameState = GetWorld()->GetGameState<AVertGameState>();
			if (GameState)
			{
				GameState->RequestFinishAndExitToMainMenu();
			}
		}
	}
}

//************************************
// Method:    StartIntrodutionState
// FullName:  AVertGameMode::StartIntrodutionState
// Access:    protected 
// Returns:   void
// Qualifier:
//************************************
void AVertGameMode::HandleIntroductionHasStarted()
{
	SetMatchState(MatchState::IntroducingMatch);

	// Start human players
	for (FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator)
	{
		APlayerController* PlayerController = Iterator->Get();
		if ((PlayerController->GetPawn() == nullptr) && PlayerCanRestart(PlayerController))
		{
			RestartPlayer(PlayerController);
		}
	}

	// Start level music
	AVertLevelScriptActor* persistentLevel = Cast<AVertLevelScriptActor>(GetWorld()->GetLevelScriptActor());
	if (persistentLevel)
	{
		const FWwiseLevelMusic& music = persistentLevel->GetLevelMusic();
		if (music.Event != NULL)
		{
			UAkGameplayStatics::PostEvent(music.Event, this);
			if (music.SwitchGroup != NAME_None && music.SwitchValue != NAME_None)
			{
				UAkGameplayStatics::SetSwitch(music.SwitchGroup, music.SwitchValue, this);
			}
		}
	}

	FSlateApplication::Get().SetAllUserFocusToGameViewport();
}

//************************************
// Method:    IntroducePlayer
// FullName:  AVertGameMode::IntroducePlayer
// Access:    virtual private 
// Returns:   void
// Qualifier:
// Parameter: int32 playerIndex
//************************************
void AVertGameMode::IntroducePlayer(int32 playerIndex)
{
	static constexpr float scIntroTime = 1.5f;

	if (playerIndex < GetGameState<AVertGameState>()->PlayerArray.Num())
	{
		AVertPlayerState* playerState = Cast<AVertPlayerState>(GetGameState<AVertGameState>()->PlayerArray[playerIndex]);
		if (playerState)
		{
			APlayerController* playerToIntro = Cast<APlayerController>(playerState->GetController());
			if (playerToIntro && playerToIntro->GetPawn())
			{
				for (FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator)
				{
					APlayerController* PlayerController = Iterator->Get();
					FViewTargetTransitionParams params;
					params.BlendTime = 0.5f;
					params.BlendFunction = VTBlend_Linear;
					params.BlendExp = 0;
					params.bLockOutgoing = false;
					PlayerController->ClientSetViewTarget(playerToIntro->GetPawn(), params);
				}

				if (IntroFeedback)
				{
					playerToIntro->ClientPlayForceFeedback(IntroFeedback, false, TEXT("IntroFeedback"));
				}

				GetWorldTimerManager().SetTimer(mTimerHandle_Intro, [this] { mCurrentIntroDone = true; }, scIntroTime, false);
			}
		}
	}
}

//************************************
// Method:    GetPlayerControllerArray
// FullName:  AVertGameMode::GetPlayerControllerArray
// Access:    public 
// Returns:   TArray<APlayerController>
// Qualifier:
//************************************
TArray<APlayerController*> AVertGameMode::GetPlayerControllerArray()
{
	TArray<APlayerController*> playerControllerArray;

	for (FConstControllerIterator It = GetWorld()->GetControllerIterator(); It; ++It)
	{
		APlayerController* pc = Cast<APlayerController>(*It);
		if (pc)
		{
			playerControllerArray.Add(pc);
		}
	}

	return playerControllerArray;
}

//************************************
// Method:    SpawnPlayerController
// FullName:  AVertGameMode::SpawnPlayerController
// Access:    virtual public 
// Returns:   APlayerController*
// Qualifier:
// Parameter: ENetRole InRemoteRole
// Parameter: FVector const & SpawnLocation
// Parameter: FRotator const & SpawnRotation
//************************************
APlayerController* AVertGameMode::SpawnPlayerController(ENetRole InRemoteRole, FVector const& SpawnLocation, FRotator const& SpawnRotation)
{
	APlayerController* controller = Super::SpawnPlayerController(InRemoteRole, SpawnLocation, SpawnRotation);
	
	if (AVertPlayerController* vPlayerController = Cast<AVertPlayerController>(controller))
	{
		FScriptDelegate onPossessedDelegate;
		onPossessedDelegate.BindUFunction(this, TEXT("OnPlayerControllerPossessedPawn"));
		vPlayerController->OnPossessed.Add(onPossessedDelegate);

		FScriptDelegate onUnPossessedDelegate;
		onUnPossessedDelegate.BindUFunction(this, TEXT("OnPlayerControllerUnPossessedPawn"));
		vPlayerController->OnUnPossessed.Add(onUnPossessedDelegate);

		UE_LOG(LogVertGameMode, Log, TEXT("Player controller delegates bound for %s"), *vPlayerController->GetName());
	}

	return controller;
}

//************************************
// Method:    Killed
// FullName:  AVertGameMode::Killed
// Access:    virtual public 
// Returns:   void
// Qualifier:
// Parameter: AController * Killer
// Parameter: AController * KilledPlayer
// Parameter: APawn * KilledPawn
// Parameter: const UDamageType * DamageType
//************************************
void AVertGameMode::Killed(AController* killer, AController* victim, APawn* killedPawn, const UDamageType* damageType)
{
	AVertPlayerState* killerPlayerState = killer ? Cast<AVertPlayerState>(killer->PlayerState) : NULL;
	AVertPlayerState* victimPlayerState = victim ? Cast<AVertPlayerState>(victim->PlayerState) : NULL;

	if (killerPlayerState && killerPlayerState != victimPlayerState)
	{
		UE_LOG(LogVertGameMode, Log, TEXT("-- Kill scored by %s for %i points --"), *killerPlayerState->GetName(), KillScore);
		TMap<EScoreTrack, int32> bonusScore = FindBonusScores(killerPlayerState, victimPlayerState, true);
		killerPlayerState->ScoreKill(victimPlayerState, KillScore, bonusScore);
		killerPlayerState->InformAboutKill(killerPlayerState, damageType, victimPlayerState);
	}

	FindAssistors(killerPlayerState, victimPlayerState, killedPawn);

	if (victimPlayerState)
	{
		victimPlayerState->ScoreDeath(killerPlayerState, DeathScore);
		victimPlayerState->BroadcastDeath(killerPlayerState, damageType, victimPlayerState);
	}

	OnPlayerKilled(killer, killedPawn);
}

TMap<EScoreTrack, int32> AVertGameMode::FindBonusScores(AVertPlayerState* killerPlayerState, AVertPlayerState* victimPlayerState, bool killingBlow)
{
	TMap<EScoreTrack, int32> bonusScores;
	if (killerPlayerState && victimPlayerState && killerPlayerState != victimPlayerState)
	{
		UE_LOG(LogVertGameMode, Log, TEXT("----------- BONUS TRACKS ------------"));
		// Rubber band
		if (killerPlayerState->GetCurrentDeathsWithoutGlory() >= 0)
		{
			int32 rubberBandScore = killerPlayerState->GetCurrentDeathsWithoutGlory() * ComebackBonus;
			bonusScores.Add(EScoreTrack::SCORE_RubberBand, rubberBandScore);
			UE_LOG(LogVertGameMode, Log, TEXT("--- Rubber Band: %i points"), rubberBandScore);
		}

		if (killingBlow)
		{
			// Killing spree
			if (killerPlayerState->GetCurrentKillStreak() > 0)
			{
				int32 killingSpreeScore = killerPlayerState->GetCurrentKillStreak() * KillSpreeBonus; 
				bonusScores.Add(EScoreTrack::SCORE_KillingSpree, killingSpreeScore);
				UE_LOG(LogVertGameMode, Log, TEXT("--- Killing Spree: %i points"), killingSpreeScore);
			}

			// Bounty hunter
			if (victimPlayerState->GetCurrentKillStreak() > 0)
			{
				int32 bountyHunterScore = FMath::Max(0, victimPlayerState->GetCurrentKillStreak() - 1) * BountyHunterBonus;
				bonusScores.Add(EScoreTrack::SCORE_BountyHunter, bountyHunterScore);
				UE_LOG(LogVertGameMode, Log, TEXT("--- Bounty Hunter: %i points"), bountyHunterScore);
			}

			// Glass cannon
			if (killerPlayerState->GetActualDamageTaken() >= 100)
			{
				int32 glassCannonScore = (FMath::FloorToInt(killerPlayerState->GetActualDamageTaken() / 100)) * GlassCannonBonus;
				bonusScores.Add(EScoreTrack::SCORE_GlassCannon, glassCannonScore);
				UE_LOG(LogVertGameMode, Log, TEXT("--- Glass Cannon: %i points"), glassCannonScore);
			}

			// Heavy hitter
			int32 bonusAmount = HeavyHitterBonus.MaxBonus;
			int32 victimDamage = FMath::Max(0, victimPlayerState->GetActualDamageTaken() - HeavyHitterBonus.StartReducingAt);
			bonusAmount -= (HeavyHitterBonus.ReductionAmount * (FMath::FloorToInt(victimDamage / HeavyHitterBonus.ReduceEvery)));
			if (bonusAmount > 0)
			{
				bonusScores.Add(EScoreTrack::SCORE_HeavyHitter, bonusAmount);
				UE_LOG(LogVertGameMode, Log, TEXT("--- Heavy Hitter: %i points"), bonusAmount);
			}
		}
	}

	return bonusScores;
}

//************************************
// Method:    FindAssistors
// FullName:  AVertGameMode::FindAssistors
// Access:    protected 
// Returns:   void
// Qualifier:
// Parameter: AVertPlayerState * killedPlayerState
// Parameter: APawn * killedPawn
//************************************
void AVertGameMode::FindAssistors(AVertPlayerState* killerPlayerState, AVertPlayerState* victimPlayerState, APawn* killedPawn)
{
	AVertCharacter* VertCharacter = Cast<AVertCharacter>(killedPawn);
	if (VertCharacter && victimPlayerState)
	{
		TArray<AController*> assistors = VertCharacter->GetRecentHitters();
		for (auto it = assistors.CreateConstIterator(); it; ++it)
		{
			AController* assistor = *it;
			AVertPlayerState* assistorPlayerState = assistor ? Cast<AVertPlayerState>(assistor->PlayerState) : NULL;
			if (assistorPlayerState && assistorPlayerState != victimPlayerState && assistorPlayerState != killerPlayerState)
			{
				UE_LOG(LogVertGameMode, Log, TEXT("-- Assist scored by %s for %i points --"), *assistorPlayerState->GetName(), AssistScore);
				TMap<EScoreTrack, int32> bonusScore = FindBonusScores(assistorPlayerState, victimPlayerState, false);
				assistorPlayerState->ScoreAssist(victimPlayerState, AssistScore, bonusScore);
			}
		}
	}
}

//************************************
// Method:    ModifyDamage
// FullName:  AVertGameMode::ModifyDamage
// Access:    virtual public 
// Returns:   float
// Qualifier: const
// Parameter: float Damage
// Parameter: AActor * DamagedActor
// Parameter: struct FDamageEvent const & DamageEvent
// Parameter: AController * EventInstigator
// Parameter: AActor * DamageCauser
//************************************
float AVertGameMode::ModifyDamage(float Damage, AActor* DamagedActor, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) const
{
	float ActualDamage = Damage;

	AVertCharacter* DamagedPawn = Cast<AVertCharacter>(DamagedActor);
	if (DamagedPawn && EventInstigator)
	{
		AVertPlayerState* DamagedPlayerState = Cast<AVertPlayerState>(DamagedPawn->PlayerState);
		AVertPlayerState* InstigatorPlayerState = Cast<AVertPlayerState>(EventInstigator->PlayerState);

		// disable friendly fire
		if (!CanDealDamage(InstigatorPlayerState, DamagedPlayerState))
		{
			ActualDamage = 0.0f;
		}

		// scale self instigated damage
		if (InstigatorPlayerState == DamagedPlayerState)
		{
			ActualDamage *= DamageSelfScale;
		}
	}

	return ActualDamage;
}

//************************************
// Method:    CanDealDamage
// FullName:  AVertGameMode::CanDealDamage
// Access:    virtual public 
// Returns:   bool
// Qualifier: const
// Parameter: class AVertPlayerState * DamageInstigator
// Parameter: class AVertPlayerState * DamagedPlayer
//************************************
bool AVertGameMode::CanDealDamage(AVertPlayerState* DamageInstigator, AVertPlayerState* DamagedPlayer) const
{
	return true;
}

//************************************
// Method:    AllowCheats
// FullName:  AVertGameMode::AllowCheats
// Access:    virtual public 
// Returns:   bool
// Qualifier:
// Parameter: APlayerController * P
//************************************
bool AVertGameMode::AllowCheats(APlayerController* P)
{
	return true;
}

//************************************
// Method:    DetermineMatchWinner
// FullName:  AVertGameMode::DetermineMatchWinner
// Access:    virtual protected 
// Returns:   void
// Qualifier:
//************************************
void AVertGameMode::DetermineMatchWinner()
{
	// nothing to do here
}

//************************************
// Method:    IsWinner
// FullName:  AVertGameMode::IsWinner
// Access:    virtual protected 
// Returns:   bool
// Qualifier: const
// Parameter: class AVertPlayerState * playerState
//************************************
bool AVertGameMode::IsWinner(AVertPlayerState* playerState) const
{
	if(playerState)
		return playerState->IsHighestScoringOverall();

	return false;
}

//************************************
// Method:    GetFollowedActors
// FullName:  AVertGameMode::GetFollowedActors
// Access:    public 
// Returns:   const TArray<AActor*>
// Qualifier: const
//************************************
const TArray<AActor*>& AVertGameMode::GetFollowedActors()
{
	TArray<AActor*> markedForRemoval;

	// Level streaming sometimes make this dangerous, as actors can be killed asynchronously
	for (auto actor : mActorsToFollow) // For safety, check the actors are valid first
	{
		if (!(actor && actor->IsValidLowLevel()))
		{
			markedForRemoval.Add(actor);
		}
	}

	for (auto actor : markedForRemoval) // remove any references that are not valid
	{
		mActorsToFollow.Remove(actor);
	}

	return mActorsToFollow;
}

//************************************
// Method:    StartMatch
// FullName:  AVertGameMode::StartMatch
// Access:    virtual private 
// Returns:   void
// Qualifier:
//************************************
void AVertGameMode::StartMatch()
{
	if (!GetWorld()->IsPlayInEditor() || (GetWorld()->IsPlayInEditor() && ShowStartupInEditor))
	{
		if (!GetWorldTimerManager().IsTimerActive(mTimerHandle_StartTimer))
		{
			if (CountdownWidgetClass && !CountdownTimerWidget)
			{
				CountdownTimerWidget = CreateWidget<UUserWidget>(GetGameInstance(), CountdownWidgetClass);
				CountdownTimerWidget->AddToViewport();
				CountdownTimerWidget->SetVisibility(ESlateVisibility::Visible);
			}

			GetWorldTimerManager().SetTimer(mTimerHandle_StartTimer, [this] {
				Super::StartMatch();

				for (FConstControllerIterator It = GetWorld()->GetControllerIterator(); It; ++It)
				{
					AVertPlayerController* vertController = Cast<AVertPlayerController>(*It);
					if (vertController && !vertController->IsPendingKill() && vertController->GetPawn())
					{
						vertController->EnableMovementInput();
					}
				}

				if (mPlayerCamera.IsValid())
				{
					AVertLevelScriptActor* level = Cast<AVertLevelScriptActor>(GetWorld()->GetLevelScriptActor());

					if(!level || level->StartOnAutoSpline)
						mPlayerCamera->IsAutoSpline = true;
				}

				mCanPlayersDie = true;
			}, 3.f, false);
		}
	}
	else
	{
		Super::StartMatch();

		if (mPlayerCamera.IsValid())
		{
			AVertLevelScriptActor* level = Cast<AVertLevelScriptActor>(GetWorld()->GetLevelScriptActor());

			if (!level || level->StartOnAutoSpline)
				mPlayerCamera->IsAutoSpline = true;
		}

		mCanPlayersDie = true;

		for (FConstControllerIterator It = GetWorld()->GetControllerIterator(); It; ++It)
		{
			AVertPlayerController* vertController = Cast<AVertPlayerController>(*It);
			if (vertController && !vertController->IsPendingKill() && vertController->GetPawn())
			{
				vertController->EnableMovementInput();
			}
		}
	}

	GetWorldTimerManager().SetTimer(mTimerHandle_ScoreTimer, this, &AVertGameMode::AddScoreOverTime, 1.f, true);
}

//************************************
// Method:    AddScoreOverTime
// FullName:  AVertGameMode::AddScoreOverTime
// Access:    private 
// Returns:   void
// Qualifier:
//************************************
void AVertGameMode::AddScoreOverTime()
{
	for (FConstControllerIterator It = GetWorld()->GetControllerIterator(); It; ++It)
	{
		AVertPlayerController* vertController = Cast<AVertPlayerController>(*It);
		if (vertController && !vertController->IsPendingKill())
		{
			AVertPlayerState* playerState = Cast<AVertPlayerState>(vertController->PlayerState);
			if (playerState)
			{
				playerState->ScoreSurvivalTime(PassiveScorePerSecond);
			}
		}
	}
}

//************************************
// Method:    GetCountdownTimerRemainingTime
// FullName:  AVertGameMode::GetCountdownTimerRemainingTime
// Access:    public 
// Returns:   float
// Qualifier: const
//************************************
float AVertGameMode::GetCountdownTimerRemainingTime() const
{
	const FTimerManager& timerMan = GetWorldTimerManager();
	if (timerMan.IsTimerActive(mTimerHandle_StartTimer))
	{
		return timerMan.GetTimerRemaining(mTimerHandle_StartTimer);
	}

	return 0;
}

//************************************
// Method:    FinishMatch
// FullName:  AVertGameMode::FinishMatch
// Access:    public 
// Returns:   void
// Qualifier:
//************************************
void AVertGameMode::FinishMatch()
{
	AVertGameState* const MyGameState = Cast<AVertGameState>(GameState);
	if (IsMatchInProgress())
	{
		EndMatch();
		DetermineMatchWinner();

		OnScoreScreenSet.Broadcast();

		// notify players
		for (FConstControllerIterator It = GetWorld()->GetControllerIterator(); It; ++It)
		{
			AVertPlayerState* PlayerState = Cast<AVertPlayerState>((*It)->PlayerState);
			const bool bIsWinner = IsWinner(PlayerState);

			(*It)->GameHasEnded(mPlayerCamera.Get(), bIsWinner);
		}

		if (CrowdApplauseSound)
		{
			UAkGameplayStatics::PostEvent(CrowdApplauseSound, this);
		}
		
		UAkGameplayStatics::SetSwitch(TEXT("All_Music"), TEXT("Results"), this);

		// lock all pawns
		// pawns are not marked as keep for seamless travel, so we will create new pawns on the next match rather than
		// turning these back on.
		for (FConstPawnIterator It = GetWorld()->GetPawnIterator(); It; ++It)
		{
			(*It)->DisableInput(nullptr);
			(*It)->TurnOff();
		}

		GetWorldTimerManager().ClearTimer(mTimerHandle_ScoreTimer);
	}
}

//************************************
// Method:    RequestFinishAndExitToMainMenu
// FullName:  AVertGameMode::RequestFinishAndExitToMainMenu
// Access:    public 
// Returns:   void
// Qualifier:
//************************************
void AVertGameMode::RequestFinishAndExitToMainMenu()
{
	FinishMatch();

	UVertGameInstance* const gameInstance = Cast<UVertGameInstance>(GetGameInstance());
	if (gameInstance)
	{
		gameInstance->RemoveSplitScreenPlayers();
	}

	AVertPlayerController* localPrimaryController = nullptr;
	for (FConstPlayerControllerIterator it = GetWorld()->GetPlayerControllerIterator(); it; ++it)
	{
		AVertPlayerController* controller = Cast<AVertPlayerController>(*it);

		if (controller == NULL)
		{
			continue;
		}

		if (!controller->IsLocalController())
		{
			const FString remoteReturnReason = NSLOCTEXT("NetworkErrors", "HostHasLeft", "Host has left the game.").ToString();
			controller->ClientReturnToMainMenu(remoteReturnReason);
		}
		else
		{
			localPrimaryController = controller;
		}
	}

	// GameInstance should be calling this from an EndState.  So call the PC function that performs cleanup, not the one that sets GI state.
	if (localPrimaryController != NULL)
	{
		localPrimaryController->HandleReturnToMainMenu();
	}
}

//************************************
// Method:    RegisterPlayerPawn
// FullName:  AVertPlayerCameraActor::RegisterPlayerPawn
// Access:    public 
// Returns:   void
// Qualifier:
// Parameter: APawn * pawnToFollow
//************************************
void AVertGameMode::RegisterPlayerPawn(APawn* pawnToFollow)
{
	if (mActorsToFollow.Find(pawnToFollow) == INDEX_NONE)
	{
		mActorsToFollow.Add(pawnToFollow);
		UE_LOG(LogVertGameMode, Warning, TEXT("Pawn added to follow list with name [%s]"), *pawnToFollow->GetName());

		if (AVertPlayerController* controller = Cast<AVertPlayerController>(pawnToFollow->GetController()))
		{
			mPlayerControllers.Add(controller);
		}
	}
}

//************************************
// Method:    UnregisterPlayerPawn
// FullName:  AVertPlayerCameraActor::UnregisterPlayerPawn
// Access:    public 
// Returns:   void
// Qualifier:
// Parameter: APawn * pawnToFollow
//************************************
void AVertGameMode::UnregisterPlayerPawn(APawn* pawnToFollow)
{
	if (mActorsToFollow.Find(pawnToFollow) != INDEX_NONE)
	{
		mActorsToFollow.Remove(pawnToFollow);
		UE_LOG(LogVertGameMode, Warning, TEXT("Pawn removed from follow list with name [%s]"), *pawnToFollow->GetName());

		if (AVertPlayerController* controller = Cast<AVertPlayerController>(pawnToFollow->GetController()))
		{
			mPlayerControllers.Remove(controller);
		}
	}
}

//************************************
// Method:    RegisterActorToFollow
// FullName:  AVertGameMode::RegisterActorToFollow
// Access:    public 
// Returns:   void
// Qualifier:
// Parameter: AActor * actorToFollow
//************************************
void AVertGameMode::RegisterActorToFollow(AActor* actorToFollow)
{
	if (mActorsToFollow.Find(actorToFollow) == INDEX_NONE)
	{
		mActorsToFollow.Add(actorToFollow);
		UE_LOG(LogVertGameMode, Warning, TEXT("Pawn added to follow list with name [%s]"), *actorToFollow->GetName());
	}
}

//************************************
// Method:    UnregisterActorToFollow
// FullName:  AVertGameMode::UnregisterActorToFollow
// Access:    public 
// Returns:   void
// Qualifier:
// Parameter: AActor * actorToFollow
//************************************
void AVertGameMode::UnregisterActorToFollow(AActor* actorToFollow)
{
	if (mActorsToFollow.Find(actorToFollow) != INDEX_NONE)
	{
		mActorsToFollow.Remove(actorToFollow);
		UE_LOG(LogVertGameMode, Warning, TEXT("Actor removed from follow list with name [%s]"), *actorToFollow->GetName());
	}
}

//************************************
// Method:    PostLogin
// FullName:  AVertGameMode::PostLogin
// Access:    virtual public 
// Returns:   void
// Qualifier:
// Parameter: APlayerController * NewPlayer
//************************************
void AVertGameMode::PostLogin(APlayerController* NewPlayer)
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
			{
				newPlayerState->PlayerIndex = availableIndices[0];
				newPlayerState->SetPlayerName(FString::Printf(TEXT("Player %i"), newPlayerState->PlayerIndex + 1));
			}				
		}

		AVertWorldSettings* worldSettings = Cast<AVertWorldSettings>(GetWorldSettings());
		if (worldSettings && newPlayerState->PlayerIndex >= 0 && newPlayerState->PlayerIndex < worldSettings->PlayerColours.Num())
			newPlayerState->SetPlayerColour(worldSettings->PlayerColours[newPlayerState->PlayerIndex]);
	}	

	if (NewPlayer && mPlayerCamera.IsValid())
	{
		NewPlayer->SetViewTargetWithBlend(mPlayerCamera.Get(), 1.f);
	}
	OnPlayerLoggedIn.Broadcast(NewPlayer);
}

void AVertGameMode::Logout(AController* Exiting)
{
	Super::Logout(Exiting);
	OnPlayerLoggedOut.Broadcast(Exiting);
}

//************************************
// Method:    OnPlayerControllerPossessedPawn_Implementation
// FullName:  AVertGameMode::OnPlayerControllerPossessedPawn_Implementation
// Access:    public 
// Returns:   void
// Qualifier:
// Parameter: APawn * pawn
//************************************
void AVertGameMode::OnPlayerControllerPossessedPawn_Implementation(class AVertPlayerController* possessor, APawn* pawn)
{
	if (!HasMatchStarted())
	{
		possessor->DisableMovementInput();
	}
}

//************************************
// Method:    OnPlayerControllerUnPossessedPawn_Implementation
// FullName:  AVertGameMode::OnPlayerControllerUnPossessedPawn_Implementation
// Access:    public 
// Returns:   void
// Qualifier:
// Parameter: APawn * pawn
//************************************
void AVertGameMode::OnPlayerControllerUnPossessedPawn_Implementation(class AVertPlayerController* possessor, APawn* pawn)
{
	// do something 
}

//************************************
// Method:    OnControllerConnectionChange_Implementation
// FullName:  AVertGameMode::OnControllerConnectionChange_Implementation
// Access:    public 
// Returns:   void
// Qualifier:
// Parameter: bool connected
// Parameter: int32 userID
// Parameter: int32 controllerID
//************************************
void AVertGameMode::OnControllerConnectionChange_Implementation(bool connected, int32 userID, int32 controllerID)
{
	UE_LOG(LogVertGameMode, Log, TEXT("CONTROLLER CONNECTION CHANGE connected: %s, user: %i, controller: %i"), (connected) ? TEXT("true") : TEXT("false"), userID, controllerID);
}

//************************************
// Method:    GetAmmendedLaunchAngle
// FullName:  AVertGameMode::GetAmmendedLaunchAngle
// Access:    public 
// Returns:   FVector
// Qualifier: const
// Parameter: const FVector & launchDirection
// Parameter: float knockback
//************************************
FVector AVertGameMode::GetAmmendedLaunchAngle(const FVector& launchDirection, float knockback) const
{
	// float dot = FVector::DotProduct(launchDirection, FVector::RightVector);
	// if (FMath::Abs(dot) < 0.5)
	// 	return launchDirection;

	if (LaunchAngleCurve)
	{
		return launchDirection.RotateAngleAxis(LaunchAngleCurve->GetFloatValue(knockback), launchDirection.X > SMALL_NUMBER ? -FVector::RightVector : FVector::RightVector);
	}
	else
	{
		return FRotator(45.f, 0, launchDirection.Rotation().Yaw).Vector();
	}
}

//************************************
// Method:    IsMatchInProgress
// FullName:  AVertGameMode::IsMatchInProgress
// Access:    virtual public 
// Returns:   bool
// Qualifier: const
//************************************
bool AVertGameMode::IsMatchInProgress() const
{
	return GetMatchState() == MatchState::InProgress || GetMatchState() == MatchState::IntroducingMatch || GetMatchState() == MatchState::InProgressFinale;
}

bool AVertGameMode::HasMatchStarted() const
{
	return GetMatchState() == MatchState::InProgress || GetMatchState() == MatchState::InProgressFinale;
}

//************************************
// Method:    PlayerCanRestart_Implementation
// FullName:  AVertGameMode::PlayerCanRestart_Implementation
// Access:    virtual protected 
// Returns:   bool
// Qualifier:
// Parameter: APlayerController * Player
//************************************
bool AVertGameMode::PlayerCanRestart_Implementation(APlayerController* Player)
{
	if (!IsMatchInProgress() || GetMatchState() == MatchState::InProgressFinale)
	{
		return false;
	}

	return AGameModeBase::PlayerCanRestart_Implementation(Player);
}

APawn* AVertGameMode::SpawnDefaultPawnAtTransform_Implementation(AController* NewPlayer, const FTransform& SpawnTransform)
{
	FTransform LocalSpawnTransform = SpawnTransform;
	bool inGame = GetMatchState() == MatchState::InProgress && GetActivePlayerCamera();
	AVertPlayerController* VertPC = Cast<AVertPlayerController>(NewPlayer);

	FActorSpawnParameters SpawnInfo;
	SpawnInfo.Instigator = Instigator;
	SpawnInfo.ObjectFlags |= RF_Transient;	// We never want to save default player pawns into a map
	SpawnInfo.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	UClass* PawnClass = (inGame && VertPC && !VertPC->GetPawn()) ? VertPC->GetDropPodClass() : GetDefaultPawnClassForController(NewPlayer);

	if (inGame && VertPC && !VertPC->GetPawn())
	{
		// Correct the spawn transform so that the pod doesn't spawn inside geometry.
		CorrectSpawnPositionForPodRecursive(PawnClass, LocalSpawnTransform, 0);
	}

	APawn* ResultPawn = GetWorld()->SpawnActor<APawn>(PawnClass, LocalSpawnTransform, SpawnInfo);
	if (!ResultPawn)
	{
		UE_LOG(LogGameMode, Warning, TEXT("SpawnDefaultPawnAtTransform: Couldn't spawn Pawn of type %s at %s"), *GetNameSafe(PawnClass), *LocalSpawnTransform.ToHumanReadableString());
	}
	return ResultPawn;
}

//************************************
// Method:    CorrectSpawnPositionForPod
// FullName:  AVertGameMode::CorrectSpawnPositionForPod
// Access:    private 
// Returns:   void
// Qualifier:
// Parameter: FTransform & spawnTransform
//************************************
void AVertGameMode::CorrectSpawnPositionForPodRecursive(UClass* PodClass, FTransform& SpawnTransform, int32 iteration)
{
	static const FName scPawnSpawnTrace(TEXT("PawnSpawnTrace"));
	static constexpr int32 scMaxIterations = 3;

	if (iteration >= 3)
		return;

	ACharacter* DropPod = PodClass->GetDefaultObject<ACharacter>();
	if (DropPod)
	{
		float radius = DropPod->GetCapsuleComponent()->GetScaledCapsuleRadius();
		float halfHeight = FMath::Max(0.f, DropPod->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() - radius);
		FVector spawnPosition = SpawnTransform.GetLocation();

		// Trace down to check if we're in blocking geometry.
		FHitResult hit(ForceInit);
		FCollisionQueryParams params;
		params.bFindInitialOverlaps = true;
		params.bTraceComplex = true;
		const FVector start(spawnPosition.X, spawnPosition.Y, spawnPosition.Z - halfHeight);
		const FVector end(spawnPosition.X, spawnPosition.Y, spawnPosition.Z + halfHeight);

		const bool didHit = GetWorld()->SweepSingleByObjectType(hit, start, end, FQuat::Identity, ECC_WorldStatic, FCollisionShape::MakeSphere(radius), params);
		if (didHit || hit.bStartPenetrating)
		{
			FHitResult ComponentHit(ForceInit);
			FCollisionQueryParams params;
			FVector ComponentTraceStart(spawnPosition.X, spawnPosition.Y, spawnPosition.Z - 1000.f);
			FVector ComponentTraceEnd(spawnPosition);
			const bool ComponentDidHit = hit.Component->LineTraceComponent(ComponentHit, ComponentTraceStart, ComponentTraceEnd, params);
			if (ComponentDidHit)
			{
				FVector newLocation(spawnPosition.X, spawnPosition.Y, ComponentHit.ImpactPoint.Z - (halfHeight + radius));
				SpawnTransform.SetLocation(newLocation);
				DrawDebugPoint(GetWorld(), ComponentHit.ImpactPoint, 32.f, FColor::Red, true);
				DrawDebugPoint(GetWorld(), newLocation, 32.f, FColor::Blue, true);
			}

			
			//CorrectSpawnPositionForPodRecursive(PodClass, SpawnTransform, ++iteration);
		}
	}
}

//************************************
// Method:    RestartPlayer
// FullName:  AVertGameMode::RestartPlayer
// Access:    virtual public 
// Returns:   void
// Qualifier:
// Parameter: AController * NewPlayer
//************************************
void AVertGameMode::RestartPlayer(AController* NewPlayer)
{
	if (NewPlayer == nullptr || NewPlayer->IsPendingKillPending())
	{
		return;
	}

	if (mCanPlayersRespawn && !HasMatchEnded())
	{
		if ((GetMatchState() == MatchState::InProgress || GetMatchState() == MatchState::InProgressFinale) && GetActivePlayerCamera())
		{
			if (AVertPlayerController* VertPC = Cast<AVertPlayerController>(NewPlayer))
			{
				if (VertPC->GetDropPodClass())
				{
					FTransform spawnTransform;
					spawnTransform.SetLocation(VertPC->GetPodSpawnLocation());
					NewPlayer->SetPawn(SpawnDefaultPawnAtTransform(NewPlayer, spawnTransform));
				}

				if (NewPlayer->GetPawn() == nullptr)
				{
					VertPC->FailedToSpawnPawn();
				}
				else
				{
					VertPC->ClientSetViewTarget(GetActivePlayerCamera());
					FinishRestartPlayer(NewPlayer, NewPlayer->GetPawn()->GetActorRotation());
				}
			}

			return;
		}
		else // if the match hasn't started yet set the players at the start points.
		{
			AVertPlayerState* playerState = Cast<AVertPlayerState>(NewPlayer->PlayerState);
			if (playerState)
			{
				AActor* playerStart = nullptr;
				switch (playerState->PlayerIndex)
				{
				case 0:
					playerStart = FindPlayerStart(NewPlayer, FString(TEXT("P1")));
					break;
				case 1:
					playerStart = FindPlayerStart(NewPlayer, FString(TEXT("P2")));
					break;
				case 2:
					playerStart = FindPlayerStart(NewPlayer, FString(TEXT("P3")));
					break;
				case 3:
					playerStart = FindPlayerStart(NewPlayer, FString(TEXT("P4")));
					break;
				default:
					playerStart = FindPlayerStart(NewPlayer);
					break;
				}

				if (playerStart)
				{
					RestartPlayerAtPlayerStart(NewPlayer, playerStart);
					return;
				}
			}

			NewPlayer->FailedToSpawnPawn();
		}
	}
	else // We're in the finale, respawns aren't allowed
	{
		// Do nothing
	}
}

//************************************
// Method:    CanPlayersDie
// FullName:  AVertGameMode::CanPlayersDie
// Access:    public 
// Returns:   bool
// Qualifier: const
//************************************
bool AVertGameMode::CanPlayersDie() const
{
	return mCanPlayersDie;
}

//************************************
// Method:    StartInvincibilityPeriod
// FullName:  AVertGameMode::StartInvincibilityPeriod
// Access:    public 
// Returns:   void
// Qualifier:
//************************************
void AVertGameMode::StartInvincibilityPeriod()
{
	mCanPlayersDie = false;
}

//************************************
// Method:    EndInvincibilityPeriod
// FullName:  AVertGameMode::EndInvincibilityPeriod
// Access:    public 
// Returns:   void
// Qualifier:
//************************************
void AVertGameMode::EndInvincibilityPeriod()
{
	mCanPlayersDie = true;
}

//************************************
// Method:    FinishRestartPlayer
// FullName:  AVertGameMode::FinishRestartPlayer
// Access:    virtual public 
// Returns:   void
// Qualifier:
// Parameter: AController * NewPlayer
// Parameter: const FRotator & StartRotation
//************************************
void AVertGameMode::FinishRestartPlayer(AController* NewPlayer, const FRotator& StartRotation)
{
	NewPlayer->Possess(NewPlayer->GetPawn());

	// If the Pawn is destroyed as part of possession we have to abort
	if (NewPlayer->GetPawn() == nullptr)
	{
		NewPlayer->FailedToSpawnPawn();
	}
	else
	{
		// Set initial control rotation to starting rotation rotation
		NewPlayer->ClientSetRotation(NewPlayer->GetPawn()->GetActorRotation(), true);

		FRotator NewControllerRot = StartRotation;
		NewControllerRot.Roll = 0.f;
		NewPlayer->SetControlRotation(NewControllerRot);

		SetPlayerDefaults(NewPlayer->GetPawn());

		K2_OnRestartPlayer(NewPlayer);
	}
}

//************************************
// Method:    SetPlayerCamera
// FullName:  AVertGameMode::SetPlayerCamera
// Access:    public 
// Returns:   void
// Qualifier:
// Parameter: AVertPlayerCameraActor * newCamera
//************************************
void AVertGameMode::SetPlayerCamera(AVertPlayerCameraActor* newCamera, float transitionTime /*= 0*/)
{
	if (newCamera)
	{
		if (mPlayerCamera.IsValid())
		{
			mPlayerCamera->DeactivateCamera();
			newCamera->SetArmLength(mPlayerCamera->GetCurrentArmLength());

			if (newCamera->LevelType == ELevelType::LT_FINALE)
			{
				StartFinale();
			}
			else if (mPlayerCamera->LevelType != ELevelType::LT_RACE && newCamera->LevelType == ELevelType::LT_RACE)
			{
				StartRaceSection();
			}
			else if (mIsRaceSection && newCamera->LevelType != ELevelType::LT_RACE)
			{
				EndRaceSection();
			}
			else if (mPlayerCamera->LevelType == ELevelType::LT_BOSS && newCamera->LevelType != ELevelType::LT_BOSS)
			{
				// award points for beating catch
				for (FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator)
				{
					AVertPlayerState* PlayerState = Cast<AVertPlayerState>(Iterator->Get()->PlayerState);
					if (PlayerState)
					{
						PlayerState->ScoreMonsterHunter(CatchDefeatedScore);
					}
				}
			}
		}
		
		mPlayerCamera = newCamera;
	}

	for (FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator)
	{
		APlayerController* PlayerController = Iterator->Get();
		if (PlayerController->IsLocalController())
		{
			PlayerController->SetViewTargetWithBlend(mPlayerCamera.Get(), transitionTime);
		}
		else
		{
			FViewTargetTransitionParams params;
			params.BlendTime = transitionTime;
			params.BlendFunction = VTBlend_Linear;
			params.BlendExp = 0;
			params.bLockOutgoing = false;
			PlayerController->ClientSetViewTarget(mPlayerCamera.Get(), params);
		}
	}

	mPlayerCamera->ActivateCamera();

	OnActiveCameraChanged.Broadcast();
}

//************************************
// Method:    StartRaceSection
// FullName:  AVertGameMode::StartRaceSection
// Access:    private 
// Returns:   void
// Qualifier:
//************************************
void AVertGameMode::StartRaceSection()
{
	if (!mIsRaceSection)
	{
		for (FConstPlayerControllerIterator iter = GetWorld()->GetPlayerControllerIterator(); iter; ++iter)
		{
			APlayerController* playerController = iter->Get();
			if (playerController)
			{
				AVertPlayerState* playerState = Cast<AVertPlayerState>(playerController->PlayerState);
				if (playerState)
				{
					mPreRaceDeaths.Add(playerState, playerState->GetDeaths());
				}
			}
		}

		mIsRaceSection = true;
	}
}

//************************************
// Method:    EndRaceSection
// FullName:  AVertGameMode::EndRaceSection
// Access:    private 
// Returns:   void
// Qualifier:
//************************************
void AVertGameMode::EndRaceSection()
{
	if (mIsRaceSection)
	{
		for (auto iter = mPreRaceDeaths.CreateConstIterator(); iter; ++iter)
		{
			int32 points = RaceSectionScore.PerfectScore;
			int32 raceDeaths = iter.Key()->GetDeaths() - iter.Value();
			if(raceDeaths > 0)
			{
				points = FMath::Max(0, RaceSectionScore.InitialScore - (RaceSectionScore.DeathPenalty * raceDeaths));
			}

			iter.Key()->ScoreRace(points);
			UE_LOG(LogVertGameMode, Log, TEXT("%s scored for race section: %i points"), *GetName(), points);
			UE_LOG(LogVertGameMode, Log, TEXT("------ Died %i times during race"), raceDeaths);
		}

		mIsRaceSection = false;
	}
}

//************************************
// Method:    StartFinale
// FullName:  AVertGameMode::StartFinale
// Access:    private 
// Returns:   void
// Qualifier:
//************************************
void AVertGameMode::StartFinale()
{
	SetMatchState(MatchState::InProgressFinale);
	HandleFinaleHasStarted();
}

//************************************
// Method:    HandleFinaleHasStarted
// FullName:  AVertGameMode::HandleFinaleHasStarted
// Access:    protected 
// Returns:   void
// Qualifier:
//************************************
void AVertGameMode::HandleFinaleHasStarted_Implementation()
{

}

//************************************
// Method:    ScoreMonsterHunter
// FullName:  AVertGameMode::ScoreMonsterHunter
// Access:    public 
// Returns:   void
// Qualifier:
// Parameter: float DamageDealt
//************************************
void AVertGameMode::ScoreMonsterHunter(float DamageDealt, AVertPlayerState* PlayerState)
{
	if (PlayerState && !HasMatchEnded())
	{
		PlayerState->ScoreMonsterHunter(DamageDealt * MonsterHunterScore);
	}
}

//************************************
// Method:    ScoreLastManStanding
// FullName:  AVertGameMode::ScoreLastManStanding
// Access:    public 
// Returns:   void
// Qualifier:
// Parameter: AVertPlayerState * PlayerState
//************************************
void AVertGameMode::ScoreLastManStanding(AVertPlayerState* PlayerState)
{
	if (PlayerState)
	{
		PlayerState->ScoreLastManStanding(LastManStandingBonus);
	}
}

//************************************
// Method:    SpawnWeaponPools
// FullName:  AVertGameMode::SpawnWeaponPools
// Access:    protected 
// Returns:   void
// Qualifier:
//************************************
void AVertGameMode::CreateObjectPools()
{
	for (auto it : AvailableItemSpawns)
	{
		if (UObjectPool* newItemPool = NewObject<UObjectPool>(this))
		{
			newItemPool->PoolSize = 20;
			newItemPool->TemplateClass = it;
			mItemPools.Add(it, newItemPool);
			newItemPool->RegisterComponent();

			if (it->IsChildOf(AProjectileRangedWeapon::StaticClass()))
			{
				AProjectileRangedWeapon* weapon = it->GetDefaultObject<AProjectileRangedWeapon>();
				UE_LOG(LogVertGameMode, Warning, TEXT("Weapon class %s spawning projectile pool of class %s."), *it->GetClass()->GetName(), *weapon->GetProjectileClass()->GetName());

				if (UObjectPool* newProjectilePool = NewObject<UObjectPool>(this))
				{
					newProjectilePool->PoolSize = 100;
					newProjectilePool->TemplateClass = weapon->GetProjectileClass();
					mProjectilePools.Add(weapon->GetProjectileClass(), newProjectilePool);
					newProjectilePool->RegisterComponent();
				}
			}
		}
	}

	if (DropPodClass)
	{
		PodPool = NewObject<UObjectPool>(this);
		if (PodPool)
		{
			PodPool->PoolSize = 10;
			PodPool->TemplateClass = DropPodClass;
			PodPool->RegisterComponent();
		}
	}
}


//************************************
// Method:    PrepareDroppod
// FullName:  AVertGameMode::PrepareDroppod
// Access:    public 
// Returns:   ADropPod*
// Qualifier:
// Parameter: const TArray<TSubclassOf<AActor>> & payload
//************************************
ADropPod* AVertGameMode::PrepareDroppod(const TArray<TSubclassOf<AActor>>& payload, const FTransform& spawnTransform, const FVector& direction /*= FVector(0,0,-1.f)*/)
{
	if (PodPool)
	{
		FPoolSpawnOptions spawnOptions;
		spawnOptions.ActorTickEnabled = false;
		spawnOptions.CollisionType = EPoolCollisionType::QueryOnly;
		spawnOptions.EnableCollision = true;
		spawnOptions.SimulatePhysics = false;

		bool spawnSuccessful;
		ADropPod* pod = Cast<ADropPod>(UObjectPool::BeginDeferredSpawnFromPool(this, PodPool, spawnOptions, spawnTransform, ESpawnActorCollisionHandlingMethod::AlwaysSpawn, nullptr, true, spawnSuccessful));

		if (spawnSuccessful && pod)
		{
			for (auto item : payload)
			{
				pod->AddPayloadActor(item);
			}

			UObjectPool::FinishDeferredSpawnFromPool(pod, spawnTransform);

			// Init velocity downwards
			pod->InitPodVelocity(direction);
			return pod;
		}
	}

	return nullptr;
}

//************************************
// Method:    RequestItemSpawn
// FullName:  AVertGameMode::RequestItemSpawn
// Access:    public 
// Returns:   void
// Qualifier:
// Parameter: TSubclassOf<AInteractive> interactive
// Parameter: const FTransform & spawnTransform
//************************************
AInteractive* AVertGameMode::RequestItemSpawn(TSubclassOf<AInteractive> interactive, const FTransform& spawnTransform, AActor* itemOwner /*= nullptr*/)
{
	if (UObjectPool** pool = mItemPools.Find(interactive))
	{
		return SpawnItemFromPool(*pool, spawnTransform, itemOwner);
	}

	return nullptr;
}

//************************************
// Method:    RequestProjectileSpawn
// FullName:  AVertGameMode::RequestProjectileSpawn
// Access:    public 
// Returns:   AWeaponProjectile*
// Qualifier:
// Parameter: TSubclassOf<AWeaponProjectile> projectile
// Parameter: const FTransform & spawnTransform
//************************************
AWeaponProjectile* AVertGameMode::RequestProjectileSpawn(TSubclassOf<AWeaponProjectile> projectile, const FTransform& spawnTransform, AActor* projectileOwner /*= nullptr*/)
{
	if (UObjectPool** pool = mProjectilePools.Find(projectile))
	{
		return SpawnProjectileFromPool(*pool, spawnTransform, projectileOwner);
	}

	return nullptr;
}

//************************************
// Method:    RequestRandomItemSpawn
// FullName:  AVertGameMode::RequestRandomItemSpawn
// Access:    public 
// Returns:   void
// Qualifier:
// Parameter: const FTransform & spawnTransform
//************************************
AInteractive* AVertGameMode::RequestRandomItemSpawn(const FTransform& spawnTransform, AActor* itemOwner /*= nullptr*/)
{
	if (AvailableItemSpawns.Num() > 0)
	{
		int32 spawnIndex = FMath::RandRange(0, AvailableItemSpawns.Num() - 1);
		if (UObjectPool** pool = mItemPools.Find(AvailableItemSpawns[spawnIndex]))
		{
			return SpawnItemFromPool(*pool, spawnTransform, itemOwner);
		}
	}

	return nullptr;
}

//************************************
// Method:    SpawnItemFromPool
// FullName:  AVertGameMode::SpawnItemFromPool
// Access:    protected 
// Returns:   AInteractive*
// Qualifier:
// Parameter: UObjectPool * targetPool
// Parameter: const FTransform & spawnTransform
//************************************
AInteractive* AVertGameMode::SpawnItemFromPool(UObjectPool* targetPool, const FTransform& spawnTransform, AActor* itemOwner /*= nullptr*/)
{
	if (targetPool)
	{
		FPoolSpawnOptions spawnOptions;
		spawnOptions.ActorTickEnabled = false;
		spawnOptions.CollisionType = EPoolCollisionType::QueryOnly;
		spawnOptions.EnableCollision = true;
		spawnOptions.SimulatePhysics = false;

		bool spawnSuccessful;

		AInteractive* item = Cast<AInteractive>(UObjectPool::BeginDeferredSpawnFromPool(this, targetPool, spawnOptions, spawnTransform, ESpawnActorCollisionHandlingMethod::AlwaysSpawn, itemOwner, true, spawnSuccessful));

		if (spawnSuccessful && item)
		{
			UObjectPool::FinishDeferredSpawnFromPool(item, spawnTransform);
			return item;
		}
	}

	return nullptr;
}

//************************************
// Method:    SpawnItemFromPool
// FullName:  AVertGameMode::SpawnItemFromPool
// Access:    protected 
// Returns:   AWeaponProjectile*
// Qualifier:
// Parameter: UObjectPool * targetPool
// Parameter: const FTransform & spawnTransform
//************************************
AWeaponProjectile* AVertGameMode::SpawnProjectileFromPool(UObjectPool* targetPool, const FTransform& spawnTransform, AActor* projectileOwner /*= nullptr*/)
{
	if (targetPool)
	{
		FPoolSpawnOptions spawnOptions;
		spawnOptions.ActorTickEnabled = false;
		spawnOptions.CollisionType = EPoolCollisionType::QueryOnly;
		spawnOptions.EnableCollision = true;
		spawnOptions.SimulatePhysics = false;

		bool spawnSuccessful;

		AWeaponProjectile* projectile = Cast<AWeaponProjectile>(UObjectPool::BeginDeferredSpawnFromPool(this, targetPool, spawnOptions, spawnTransform, ESpawnActorCollisionHandlingMethod::AlwaysSpawn, projectileOwner, true, spawnSuccessful));

		if (spawnSuccessful && projectile)
		{
			projectile->SetOwner(projectileOwner);
			UObjectPool::FinishDeferredSpawnFromPool(projectile, spawnTransform);
			return projectile;
		}
	}

	return nullptr;
}

//************************************
// Method:    StartTimer
// FullName:  AVertGameMode::StartTimer
// Access:    public 
// Returns:   void
// Qualifier:
// Parameter: float time
//************************************
void AVertGameMode::StartTimer(float time, bool showTimer/* = true*/, bool resetProgress/* = false*/)
{
	if (!GetWorldTimerManager().IsTimerActive(mTimerHandle_MapTimer))
	{
		if (showTimer)
		{
			for (FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator)
			{
				if (AVertPlayerController* pc = Cast<AVertPlayerController>(*Iterator))
				{
					pc->ShowTimer();
				}
			}
		}		
	}
	
	time = (resetProgress) ? time : time + GetWorldTimerManager().GetTimerRemaining(mTimerHandle_MapTimer);

	GetWorldTimerManager().SetTimer(mTimerHandle_MapTimer, [this](void) {
		OnTimerExpired.Broadcast();

		for (FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator)
		{
			if (AVertPlayerController* pc = Cast<AVertPlayerController>(*Iterator))
			{
				pc->HideTimer();
			}
		}
	}, time, false);
}

//************************************
// Method:    GetRemainingTime
// FullName:  AVertGameMode::GetRemainingTime
// Access:    public 
// Returns:   int32
// Qualifier: const
//************************************
int32 AVertGameMode::GetRemainingTime() const
{
	if (!GetWorldTimerManager().IsTimerActive(mTimerHandle_MapTimer))
		return 0;
	else
	{
		return FMath::CeilToInt(GetWorldTimerManager().GetTimerRemaining(mTimerHandle_MapTimer));
	}	
}

//************************************
// Method:    GetRemainingTimeMilliseconds
// FullName:  AVertGameMode::GetRemainingTimeMilliseconds
// Access:    public 
// Returns:   int32
// Qualifier: const
//************************************
int32 AVertGameMode::GetRemainingTimeMilliseconds() const
{
	if (!GetWorldTimerManager().IsTimerActive(mTimerHandle_MapTimer))
		return 0;

	float remainingTime = GetWorldTimerManager().GetTimerRemaining(mTimerHandle_MapTimer);
	float milliseconds = remainingTime - FMath::FloorToFloat(remainingTime);
	return FMath::CeilToInt(milliseconds * 1000.f);
}