// Copyright Inside Out Games Ltd. 2017

#include "VertLevelScriptActor.h"
#include "Vert.h"

DECLARE_LOG_CATEGORY_CLASS(LogVertLevelScript, Log, All);

void AVertLevelScriptActor::BeginPlay()
{
	Super::BeginPlay();
}

void AVertLevelScriptActor::Tick(float DeltaTime)
{
	if (!mPlayerCameraSet)
	{
		if (UWorld* world = GetWorld())
		{
			if (AVertGameMode* gameMode = world->GetAuthGameMode<AVertGameMode>())
			{
				TWeakObjectPtr<AVertPlayerCameraActor> camera = GetStartingCamera();
				if (camera.IsValid())
				{
					gameMode->SetPlayerCamera(camera.Get());
					mPlayerCameraSet = true;
				}
				else
				{
					UE_LOG(LogTemp, Error, TEXT("Camera doesn't exist"));
				}
			}
		}
	}
}

//************************************
// Method:    GetStartingCamera
// FullName:  AVertLevelScriptActor::GetStartingCamera
// Access:    public 
// Returns:   TWeakObjectPtr<AVertPlayerCameraActor>
// Qualifier:
//************************************
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

//************************************
// Method:    SetActiveCamera
// FullName:  AVertLevelScriptActor::SetActiveCamera
// Access:    public 
// Returns:   void
// Qualifier:
// Parameter: AVertPlayerCameraActor * newCamera
// Parameter: float transitionTime
//************************************
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