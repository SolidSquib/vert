// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "GameFramework/GameMode.h"
#include "View/VertPlayerCameraActor.h"
#include "Engine/VertGlobals.h"
#include "VertGameMode.generated.h"

// The GameMode defines the game being played. It governs the game rules, scoring, what actors
// are allowed to exist in this game type, and who may enter the game.
//
// This game mode just sets the default pawn to be the MyCharacter asset, which is a subclass of VertCharacter

DECLARE_LOG_CATEGORY_EXTERN(LogVertGameMode, Log, All);

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FVertGameModeEventDelegate);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FVertGameModePlayerLoggedIn, AController*, player);

namespace MatchState
{
	extern VERT_API const FName IntroducingMatch;
	extern VERT_API const FName InProgressFinale;
}

USTRUCT(BlueprintType)
struct FRaceSectionScore
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rules")
	int32 PerfectScore = 12000;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rules")
	int32 InitialScore = 10000;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rules")
	int32 DeathPenalty = 1000;
};

USTRUCT(BlueprintType)
struct FHeavyHitterBonus
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rules")
	int32 MaxBonus = 500;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rules")
	int32 StartReducingAt = 50;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rules")
	int32 ReduceEvery = 10;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rules")
	int32 ReductionAmount = 100;
};

UCLASS(minimalapi)
class AVertGameMode : public AGameMode
{
	GENERATED_BODY()

	friend class APlayerDroppod;

public:
	UPROPERTY(BlueprintAssignable)
	FVertGameModeEventDelegate OnTimerExpired;

	UPROPERTY(BlueprintAssignable)
	FVertGameModeEventDelegate OnActiveCameraChanged;

	UPROPERTY(BlueprintAssignable)
	FVertGameModePlayerLoggedIn OnPlayerLoggedIn;

	UPROPERTY(BlueprintAssignable)
	FVertGameModePlayerLoggedIn OnPlayerLoggedOut;

	UPROPERTY(BlueprintAssignable)
	FVertGameModeEventDelegate OnScoreScreenSet;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawning")
	TSubclassOf<class ADropPod> DropPodClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawning")
	TArray<TSubclassOf<AInteractive>> AvailableItemSpawns;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawning")
	class UForceFeedbackEffect* IntroFeedback = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DeathZone")
	float AllowedDistanceFromCameraBounds = 250.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
	bool ShowStartupInEditor = false;

protected:
	/** score for kill */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rules")
	int32 KillScore = 2000;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rules")
	int32 DeathScore = 0;

	/* awarded when a player dealt non-killing damage to killed player within 10 seconds of their death */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rules")
	int32 AssistScore = 500;

	/* every player receives this for each second they are alive */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rules")
	int32 PassiveScorePerSecond = 100;

	/* scores awarded based on performance in race sections */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rules")
	FRaceSectionScore RaceSectionScore;

	/* awarded to all players once the catch is defeated */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rules")
	int32 CatchDefeatedScore = 10000;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rules")
	int32 MonsterHunterScore = 100;

	/* accumulative bonus score per kill before death; 1st kill = 2000, 2nd kill = 2500, 3rd kill = 3000, etc */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rules")
	int32 KillSpreeBonus = 500;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rules")
	int32 LastManStandingBonus = 2000;

	/* additional score * size of victims kill spree */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rules")
	int32 BountyHunterBonus = 500;

	/* awarded per kill for each every 100 damage modifier the killing player has */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rules")
	int32 GlassCannonBonus = 500;

	/* Bonus awarded for killing players on a low damage modifier */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rules")
	FHeavyHitterBonus HeavyHitterBonus;

	/* awarded on kill or assist, per previous death without a kill (minor rubber banding) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rules")
	int32 ComebackBonus = 100;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rules", meta = (UIMin = 0.1, UIMax = 10.0))
	float DamageRatio = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rules", meta = (UIMin = 0.0, UIMax = 100.0))
	UCurveFloat* LaunchAngleCurve;

	/** scale for self instigated damage */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rules")
	float DamageSelfScale = 1.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PlayerColours")
	TArray<struct FPlayerColours> PlayerColours;

	UPROPERTY()
	class UObjectPool* PodPool = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<class UUserWidget> CountdownWidgetClass = nullptr;

	UPROPERTY()
	UUserWidget* CountdownTimerWidget = nullptr;

	UPROPERTY(EditAnywhere, Category = "Audio")
	class UAkAudioEvent* CrowdApplauseSound = nullptr;

public:
	AVertGameMode();

	/*Finishes the match and bumps everyone to main menu.*/
	/*Only GameInstance should call this function */
	void RequestFinishAndExitToMainMenu();
	void ScoreMonsterHunter(float DamageDealt, class AVertPlayerState* PlayerState);
	void ScoreLastManStanding(AVertPlayerState* PlayerState);

	virtual bool HasMatchStarted() const override;
	virtual bool IsMatchInProgress() const override;
	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
	virtual void BeginPlay() override;
	virtual void PreInitializeComponents() override;
	virtual void PostInitializeComponents() override;
	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual void Logout(AController* Exiting) override;
	virtual APlayerController* SpawnPlayerController(ENetRole InRemoteRole, FVector const& SpawnLocation, FRotator const& SpawnRotation) override;
	virtual float ModifyDamage(float Damage, AActor* DamagedActor, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) const; /** prevents friendly fire */
	virtual void Killed(AController* Killer, AController* KilledPlayer, APawn* KilledPawn, const UDamageType* DamageType); /** notify about kills */
	virtual bool CanDealDamage(class AVertPlayerState* DamageInstigator, AVertPlayerState* DamagedPlayer) const; /** can players damage each other? */
	virtual bool AllowCheats(APlayerController* controller) override; /** always create cheat manager */
	virtual void RestartPlayer(AController* newPlayer) override;
	virtual void FinishRestartPlayer(AController* NewPlayer, const FRotator& StartRotation) override;

	UFUNCTION(BlueprintCallable, Category = Spawning)
	ADropPod* PrepareDroppod(const TArray<TSubclassOf<AActor>>& payload, const FTransform& spawnTransform, const FVector& direction = FVector(0,0,-1.f));

	UFUNCTION(BlueprintCallable, Category = "Camera")
	void SetPlayerCamera(AVertPlayerCameraActor* newCamera, float transitionTime = 0);
	
	UFUNCTION(BlueprintCallable, Category = "Players")
	const TArray<AActor*>& GetFollowedActors();

	UFUNCTION(BlueprintCallable, Category = "Gameplay Events|Timers")
	void StartTimer(float time, bool showTimer = true, bool resetProgress = false);

	UFUNCTION(BlueprintCallable, Category = "Gameplay Events|Timers")
	float GetCountdownTimerRemainingTime() const;

	UFUNCTION(BlueprintCallable, Category = "Gameplay Events|Timers")
	int32 GetRemainingTime() const;

	UFUNCTION(BlueprintCallable, Category = "Gameplay Events|Timers")
	int32 GetRemainingTimeMilliseconds() const;

	UFUNCTION(BlueprintCallable, Category = "PlayerCamera")
	void RegisterPlayerPawn(class APawn* pawnToFollow);

	UFUNCTION(BlueprintCallable, Category = "PlayerCamera")
	void UnregisterPlayerPawn(class APawn* pawnToFollow);

	UFUNCTION(BlueprintCallable, Category = "PlayerCamera")
	void RegisterActorToFollow(AActor* actorToFollow);

	UFUNCTION(BlueprintCallable, Category = "PlayerCamera")
	void UnregisterActorToFollow(AActor* actorToFollow);

	UFUNCTION(BlueprintCallable, Category = "PlayerCamera")
	AVertPlayerCameraActor* GetActivePlayerCamera() const { return mPlayerCamera.Get(); }

	UFUNCTION(BlueprintNativeEvent, Category = "PlayerController")
	void OnPlayerControllerPossessedPawn(class AVertPlayerController* possessor, APawn* pawn);

	UFUNCTION(BlueprintNativeEvent, Category = "PlayerController")
	void OnPlayerControllerUnPossessedPawn(class AVertPlayerController* possessor, APawn* pawn);

	UFUNCTION(exec)
	void FinishMatch();

	UFUNCTION(BlueprintCallable)
	FVector GetAmmendedLaunchAngle(const FVector& launchDirection, float knockback) const;

	UFUNCTION(BlueprintCallable)
	FORCEINLINE float GetDamageRatio() const { return DamageRatio; }

	UFUNCTION(BlueprintCallable, Category = "ObjectPools")
	AInteractive* RequestItemSpawn(TSubclassOf<AInteractive> interactive, const FTransform& spawnTransform, AActor* itemOwner = nullptr);

	UFUNCTION(BlueprintCallable, Category = "ObjectPools")
	AWeaponProjectile* RequestProjectileSpawn(TSubclassOf<AWeaponProjectile> projectile, const FTransform& spawnTransform, AActor* projectileOwner = nullptr);

	UFUNCTION(BlueprintCallable, Category = "ObjectPools")
	AInteractive* RequestRandomItemSpawn(const FTransform& spawnTransform, AActor* itemOwner = nullptr);

	UFUNCTION(BlueprintCallable, Category = "Utility")
	TArray<APlayerController*> GetPlayerControllerArray();

	UFUNCTION(BlueprintCallable, Category = "Gameplay")
	void StartInvincibilityPeriod();

	UFUNCTION(BlueprintCallable, Category = "Gameplay")
	void EndInvincibilityPeriod();

	UFUNCTION(BlueprintCallable, Category = "Gameplay")
	bool CanPlayersDie() const;

	UFUNCTION(BlueprintCallable, Category = "Gameplay")
	bool GetCanPlayersRespawn() const { return mCanPlayersRespawn; }

	UFUNCTION(BlueprintCallable, Category = "Gameplay")
	void SetCanPlayersRespawn(bool newValue) { mCanPlayersRespawn = newValue; }

	UFUNCTION(BlueprintImplementableEvent)
	void OnPlayerKilled(AController* Killer, APawn* KilledPawn);

	UFUNCTION(BlueprintCallable)
	void StartFinale();
	
protected:
	void HandleIntroductionHasStarted();
	
	void DefaultTimer();
	void CreateObjectPools();
	void FindAssistors(AVertPlayerState* killerPlayerState, AVertPlayerState* victimPlayerState, APawn* killedPawn);
	TMap<EScoreTrack, int32> FindBonusScores(AVertPlayerState* killerPlayerState, AVertPlayerState* victimPlayerState, bool killingBlow);
	AInteractive* SpawnItemFromPool(UObjectPool* targetPool, const FTransform& spawnTransform, AActor* itemOwner = nullptr);
	AWeaponProjectile* SpawnProjectileFromPool(UObjectPool* targetPool, const FTransform& spawnTransform, AActor* projectileOwner);

	virtual void DetermineMatchWinner();
	virtual bool IsWinner(AVertPlayerState* playerState) const;
	virtual bool PlayerCanRestart_Implementation(APlayerController* Player) override;
	virtual APawn* SpawnDefaultPawnAtTransform_Implementation(AController* NewPlayer, const FTransform& SpawnTransform) override;

	UFUNCTION(BlueprintNativeEvent, Category = "Input")
	void OnControllerConnectionChange(bool connected, int32 userID, int32 controllerID);

	UFUNCTION(BlueprintNativeEvent, Category = "GameStates")
	void HandleFinaleHasStarted();

private:
	void AddScoreOverTime();
	void StartRaceSection();
	void EndRaceSection();
	void CorrectSpawnPositionForPodRecursive(UClass* PoDClass, FTransform& SpawnTransform, int32 iteration);

	virtual void StartMatch() override;
	virtual void IntroducePlayer(int32 playerIndex);

protected:
	FDelegateHandle mOnControllerChangedHandle;
	TWeakObjectPtr<AVertPlayerCameraActor> mPlayerCamera;
	TArray<AActor*> mActorsToFollow;
	TArray<class AVertPlayerController*> mPlayerControllers;
	TMap<TSubclassOf<AInteractive>, UObjectPool*> mItemPools;
	TMap<TSubclassOf<AWeaponProjectile>, UObjectPool*> mProjectilePools;
	FTimerHandle mTimerHandle_MapTimer;
	FTimerHandle mTimerHandle_DefaultTimer;
	FTimerHandle mTimerHandle_ScoreTimer;

	TArray<ADropPod*> mBoundPods;
	bool mCanPlayersDie = false;

	// intro state stuff
	int32 mCurrentIntroPlayerIndex = 0;
	bool mCurrentIntroDone = false;
	FTimerHandle mTimerHandle_Intro;
	FTimerHandle mTimerHandle_StartTimer;

	// race section tracking
	bool mIsRaceSection = false;
	TMap<AVertPlayerState*, int32> mPreRaceDeaths;

	bool mCanPlayersRespawn = true;
};
