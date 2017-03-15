// Copyright Inside Out Games Ltd. 2017

#include "Vert.h"
#include "VertGameInstance.h"

void UVertGameInstance::Init()
{
	Super::Init();
}

void UVertGameInstance::Shutdown()
{
	Super::Shutdown();
}

void UVertGameInstance::StartGameInstance()
{
	Super::StartGameInstance();

	mOnControllerChangedHandle = FCoreDelegates::OnControllerConnectionChange.AddUFunction(this, TEXT("OnControllerConnectionChange"));
}

ULocalPlayer* UVertGameInstance::CreateInitialPlayer(FString& OutError)
{
	return Super::CreateInitialPlayer(OutError);
}

void UVertGameInstance::StartRecordingReplay(const FString& InName, const FString& FriendlyName, const TArray<FString>& AdditionalOptions /*= TArray<FString>()*/)
{
	Super::StartRecordingReplay(InName, FriendlyName, AdditionalOptions);
}

void UVertGameInstance::StopRecordingReplay()
{
	Super::StopRecordingReplay();
}

void UVertGameInstance::PlayReplay(const FString& InName, UWorld* WorldOverride /*= nullptr*/, const TArray<FString>& AdditionalOptions /*= TArray<FString>()*/)
{
	Super::PlayReplay(InName, WorldOverride, AdditionalOptions);
}

void UVertGameInstance::AddUserToReplay(const FString& UserString)
{
	Super::AddUserToReplay(UserString);
}

void UVertGameInstance::OnControllerConnectionChange_Implementation(bool connected, int32 userID, int32 controllerID)
{
	UE_LOG(VertCritical, Warning, TEXT("[GameInstance] CONTROLLER CONNECTION CHANGE! connected: %s, user: %i, controller: %i"), (connected) ? TEXT("true") : TEXT("false"), userID, controllerID);
}