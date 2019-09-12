// Copyright Inside Out Games Ltd. 2017

#pragma once

#include "Engine/GameInstance.h"
#include "VertLocalPlayer.h"
#include "VertGameInstance.generated.h"

#define MAX_PLAYERS 4

/**
 * 
 */
UCLASS()
class VERT_API UVertGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	void RemoveSplitScreenPlayers();
	void RemoveExistingLocalPlayer(ULocalPlayer* ExistingPlayer);
	bool IsControllerIDAvailable(const int32& id);

	virtual void Init() override;
	virtual void Shutdown() override;
	virtual ULocalPlayer* CreateInitialPlayer(FString& OutError) override;
	virtual void StartRecordingReplay(const FString& InName, const FString& FriendlyName, const TArray<FString>& AdditionalOptions = TArray<FString>()) override;
	virtual void StopRecordingReplay() override;
	virtual void PlayReplay(const FString& InName, UWorld* WorldOverride = nullptr, const TArray<FString>& AdditionalOptions = TArray<FString>()) override;
	virtual void AddUserToReplay(const FString& UserString) override;

	UFUNCTION()
	virtual void BeginLoadingScreen(const FString& mapName);

	UFUNCTION()
	virtual void EndLoadingScreen(UWorld* world);

	UFUNCTION(BlueprintCallable, Category = "LocalMultiplayer")
	TArray<UVertLocalPlayer*> GetVertLocalPlayers();
};
