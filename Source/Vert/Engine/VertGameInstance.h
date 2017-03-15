// Copyright Inside Out Games Ltd. 2017

#pragma once

#include "Engine/GameInstance.h"
#include "VertGameInstance.generated.h"

/**
 * 
 */
UCLASS()
class VERT_API UVertGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	virtual void Init() override;
	virtual void Shutdown() override;
	virtual void StartGameInstance() override;
	virtual ULocalPlayer* CreateInitialPlayer(FString& OutError) override;
	virtual void StartRecordingReplay(const FString& InName, const FString& FriendlyName, const TArray<FString>& AdditionalOptions = TArray<FString>()) override;
	virtual void StopRecordingReplay() override;
	virtual void PlayReplay(const FString& InName, UWorld* WorldOverride = nullptr, const TArray<FString>& AdditionalOptions = TArray<FString>()) override;
	virtual void AddUserToReplay(const FString& UserString) override;

	UFUNCTION(BlueprintNativeEvent, Category = "Input")
	void OnControllerConnectionChange(bool connected, int32 userID, int32 controllerID);

private:
	FDelegateHandle mOnControllerChangedHandle;
};
