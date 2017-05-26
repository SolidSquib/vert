// Copyright Inside Out Games Ltd. 2017

#include "Vert.h"
#include "VertLevelScriptActor.h"

DECLARE_LOG_CATEGORY_CLASS(LogVertLevelScript, Log, All);

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

		mActiveCamera->SetActorTickEnabled(true);
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
		mActiveCamera->SetActorTickEnabled(false);
	}

	mActiveCamera = newCamera;

	if (mActiveCamera.IsValid())
	{
		mActiveCamera->SetActorTickEnabled(true);
		for (APawn* pawn : mRegisteredPawns)
		{
			mActiveCamera->RegisterPlayerPawn(pawn);
			if (APlayerController* controller = Cast<APlayerController>(pawn->GetController()))
			{
				controller->SetViewTargetWithBlend(mActiveCamera.Get(), transitionTime);
			}
		}
	}
}

void AVertLevelScriptActor::RegisterPlayerPawn(APawn* pawnToFollow)
{
	if (mRegisteredPawns.Find(pawnToFollow) == INDEX_NONE)
	{
		mRegisteredPawns.Add(pawnToFollow);
	}
}

void AVertLevelScriptActor::UnregisterPlayerPawn(APawn* pawnToFollow)
{
	if (mRegisteredPawns.Find(pawnToFollow) != INDEX_NONE)
	{
		mRegisteredPawns.Remove(pawnToFollow);
	}
}
