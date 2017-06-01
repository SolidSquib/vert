// Copyright Inside Out Games Ltd. 2017

#include "Vert.h"
#include "VertLevelScriptActor.h"

DECLARE_LOG_CATEGORY_CLASS(LogVertLevelScript, Log, All);

void AVertLevelScriptActor::BeginPlay()
{
	GetWorld()->GetAuthGameMode<AVertGameMode>()->SetPlayerCamera(GetStartingCamera().Get());
}

TWeakObjectPtr<AVertPlayerCameraActor> AVertLevelScriptActor::GetStartingCamera()
{
	if (StartCamera)
	{
		UE_LOG(LogVertLevelScript, Log, TEXT("Setting start camera [%s] as the active player camera"), *StartCamera->GetName());
		mActiveCamera = StartCamera;
	}
	else
	{
		for (TActorIterator<AVertPlayerCameraActor> ActorItr(GetWorld()); ActorItr; ++ActorItr)
		{
			// Same as with the Object Iterator, access the subclass instance with the * or -> operators.
			if (mActiveCamera.IsValid())
			{
				UE_LOG(LogVertLevelScript, Warning, TEXT("More than one AVertPlayerCameraActor found in scene, check that the correct camera is being used and the rest are removed."));
			}
			mActiveCamera = *ActorItr;
		}
	}

	if (mActiveCamera.IsValid())
	{
		for (auto iter = GetWorld()->GetPlayerControllerIterator(); iter; ++iter)
		{
			APlayerController* controller = iter->Get();
			controller->SetViewTarget(mActiveCamera.Get());
		}

		mActiveCamera->ActivateCamera();
	}
	else { UE_LOG(LogVertLevelScript, Warning, TEXT("No AVertPlayerCameraActor found, game will default to FPS view.")); }

	return mActiveCamera;
}

void AVertLevelScriptActor::SetActiveCamera(AVertPlayerCameraActor* newCamera, float transitionTime)
{
	if (newCamera == mActiveCamera)
	{
		UE_LOG(LogVertLevelScript, Warning, TEXT("Setting new camera to already active camera, check for possible optimization."));
		return;
	}

	if (mActiveCamera.IsValid())
	{
		mActiveCamera->DeactivateCamera();
	}

	mActiveCamera = newCamera;

	if (mActiveCamera.IsValid())
	{
		AVertGameMode* gameMode = GetWorld()->GetAuthGameMode<AVertGameMode>();

		gameMode->SetPlayerCamera(mActiveCamera.Get());

		for (APawn* pawn : gameMode->GetFollowedActors())
		{
			if (APlayerController* controller = Cast<APlayerController>(pawn->GetController()))
			{
				controller->SetViewTargetWithBlend(mActiveCamera.Get(), transitionTime);
			}
		}
		mActiveCamera->ActivateCamera();
	}
}