// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "GameFramework/GameMode.h"
#include "View/VertPlayerCameraActor.h"
#include "VertGameMode.generated.h"

// The GameMode defines the game being played. It governs the game rules, scoring, what actors
// are allowed to exist in this game type, and who may enter the game.
//
// This game mode just sets the default pawn to be the MyCharacter asset, which is a subclass of VertCharacter

DECLARE_LOG_CATEGORY_EXTERN(LogVertGameMode, Log, All);

UCLASS(minimalapi)
class AVertGameMode : public AGameMode
{
	GENERATED_BODY()
		
protected:
	/** score for kill */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rules")
	int32 KillScore = 1;

	/** score for death */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rules")
	int32 DeathScore = -1;
	
	/** How many lives does a player start with (-1 = Infinite)*/
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rules", meta = (UIMin = -1, UIMax = 100))
	int32 NumLives = -1;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rules", meta = (UIMin = 0.1, UIMax = 10.0))
	float DamageRatio = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rules", meta = (UIMin = 0.0, UIMax = 100.0))
	UCurveFloat* LaunchAngleCurve;

	/** scale for self instigated damage */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rules")
	float DamageSelfScale = 1.f;

public:
	AVertGameMode();

	/*Finishes the match and bumps everyone to main menu.*/
	/*Only GameInstance should call this function */
	void RequestFinishAndExitToMainMenu();

	virtual void BeginPlay() override;
	virtual void PostLogin(APlayerController* NewPlayer) override;	
	virtual APlayerController* SpawnPlayerController(ENetRole InRemoteRole, FVector const& SpawnLocation, FRotator const& SpawnRotation) override;
	virtual float ModifyDamage(float Damage, AActor* DamagedActor, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) const; /** prevents friendly fire */
	virtual void Killed(AController* Killer, AController* KilledPlayer, APawn* KilledPawn, const UDamageType* DamageType); /** notify about kills */
	virtual bool CanDealDamage(AVertPlayerState* DamageInstigator, AVertPlayerState* DamagedPlayer) const; /** can players damage each other? */
	virtual bool AllowCheats(APlayerController* controller) override; /** always create cheat manager */

	FORCEINLINE void SetPlayerCamera(AVertPlayerCameraActor* newCamera) { mPlayerCamera = newCamera; }
	FORCEINLINE const TArray<APawn*>& GetFollowedActors() const { return mPawnsToFollow; }

	UFUNCTION(BlueprintCallable, Category = "PlayerPosition")
	void RegisterPlayerPawn(class APawn* pawnToFollow);

	UFUNCTION(BlueprintCallable, Category = "PlayerPosition")
	void UnregisterPlayerPawn(class APawn* pawnToFollow);

	UFUNCTION(BlueprintCallable, Category = "PlayerCamera")
	AVertPlayerCameraActor* GetActivePlayerCamera() const { return mPlayerCamera.Get(); }

	UFUNCTION(BlueprintNativeEvent, Category = "PlayerController")
	void OnPlayerControllerPossessedPawn(APawn* pawn);

	UFUNCTION(BlueprintNativeEvent, Category = "PlayerController")
	void OnPlayerControllerUnPossessedPawn(APawn* pawn);

	UFUNCTION(exec)
	void FinishMatch();

	UFUNCTION(BlueprintCallable)
	FVector GetAmmendedLaunchAngle(const FVector& launchDirection, float knockback) const;

	UFUNCTION(BlueprintCallable)
	FORCEINLINE float GetDamageRatio() const { return DamageRatio; }
	
protected:
	virtual void DetermineMatchWinner();
	virtual bool IsWinner(AVertPlayerState* playerState) const;

	UFUNCTION(BlueprintNativeEvent, Category = "Input")
	void OnControllerConnectionChange(bool connected, int32 userID, int32 controllerID);

protected:
	FDelegateHandle mOnControllerChangedHandle;
	TWeakObjectPtr<AVertPlayerCameraActor> mPlayerCamera;
	TArray<class APawn*> mPawnsToFollow;
	TArray<AVertPlayerController*> mPlayerControllers;
};
