// Copyright Inside Out Games Ltd. 2017

#include "VertLevelScriptActor.h"
#include "Vert.h"

DECLARE_LOG_CATEGORY_CLASS(LogVertLevelScript, Log, All);

//************************************
// Method:    BeginPlay
// FullName:  AVertLevelScriptActor::BeginPlay
// Access:    virtual protected 
// Returns:   void
// Qualifier:
//************************************
void AVertLevelScriptActor::BeginPlay()
{
	Super::BeginPlay();
	UAkGameplayStatics::StartAllAmbientSounds(this);
}

//************************************
// Method:    Tick
// FullName:  AVertLevelScriptActor::Tick
// Access:    virtual public 
// Returns:   void
// Qualifier:
// Parameter: float DeltaTime
//************************************
void AVertLevelScriptActor::Tick(float DeltaTime)
{
	if (!mPlayerCameraSet)
	{
		if (UWorld* world = GetWorld())
		{
			if (AVertGameMode* gameMode = world->GetAuthGameMode<AVertGameMode>())
			{
				if (!gameMode->GetActivePlayerCamera())
				{
					TWeakObjectPtr<AVertPlayerCameraActor> camera = GetStartingCamera();
					if (camera.IsValid())
					{
						gameMode->SetPlayerCamera(camera.Get());
					}
				}
			}
		}

		mPlayerCameraSet = true;
	}

	Super::Tick(DeltaTime);
}

//************************************
// Method:    PostInitializeComponents
// FullName:  AVertLevelScriptActor::PostInitializeComponents
// Access:    protected 
// Returns:   void
// Qualifier:
//************************************
void AVertLevelScriptActor::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	for(auto ArenaCamera : ArenaCameras)
	{
		if (ArenaCamera)
		{
			ArenaCamera->OnCameraBecomeActive.AddDynamic(this, &AVertLevelScriptActor::InternalOnPrimaryCameraBecomeActive);
			ArenaCamera->OnCameraBecomeInactive.AddDynamic(this, &AVertLevelScriptActor::InternalOnPrimaryCameraBecomeInactive);
			ArenaCamera->OnCameraReachEndOfTrack.AddDynamic(this, &AVertLevelScriptActor::InternalOnPrimaryCameraReachedEndOfTrack);
		}
	}
}

//************************************
// Method:    InternalOnPrimaryCameraBecomeActive
// FullName:  AVertLevelScriptActor::InternalOnPrimaryCameraBecomeActive
// Access:    protected 
// Returns:   void
// Qualifier:
// Parameter: AVertPlayerCameraActor * camera
//************************************
void AVertLevelScriptActor::InternalOnPrimaryCameraBecomeActive(AVertPlayerCameraActor* camera)
{
	int32 cameraIndex = ArenaCameras.Find(camera);
	OnPrimaryCameraBecomeActive(cameraIndex, camera);
}

//************************************
// Method:    InternalOnPrimaryCameraBecomeInactive
// FullName:  AVertLevelScriptActor::InternalOnPrimaryCameraBecomeInactive
// Access:    protected 
// Returns:   void
// Qualifier:
// Parameter: AVertPlayerCameraActor * camera
//************************************
void AVertLevelScriptActor::InternalOnPrimaryCameraBecomeInactive(AVertPlayerCameraActor* camera)
{
	int32 cameraIndex = ArenaCameras.Find(camera);
	OnPrimaryCameraBecomeInactive(cameraIndex, camera);
}

//************************************
// Method:    InternalOnPrimaryCameraReachedEndOfTrack
// FullName:  AVertLevelScriptActor::InternalOnPrimaryCameraReachedEndOfTrack
// Access:    protected 
// Returns:   void
// Qualifier:
// Parameter: AVertPlayerCameraActor * camera
//************************************
void AVertLevelScriptActor::InternalOnPrimaryCameraReachedEndOfTrack(AVertPlayerCameraActor* camera)
{
	if (camera->AutoTransition)
	{
		int32 cameraIndex = ArenaCameras.Find(camera);
		OnPrimaryCameraReachedEndOfTrack(cameraIndex, camera);

		if (AVertGameMode* gameMode = GetWorld()->GetAuthGameMode<AVertGameMode>())
		{
			if ((cameraIndex + 1) < ArenaCameras.Num())
			{
				gameMode->SetPlayerCamera(ArenaCameras[cameraIndex + 1], ArenaCameras[cameraIndex]->NextTransitionTime);
			}
		}
	}
}

//************************************
// Method:    GetPlaneConstraintOrigin
// FullName:  AVertLevelScriptActor::GetPlaneConstraintOrigin
// Access:    public 
// Returns:   FVector
// Qualifier: const
//************************************
FVector AVertLevelScriptActor::GetPlaneConstraintOrigin() const
{
	return PlayerMovementPlane;
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
	if (ArenaCameras.Num() > 0 && ArenaCameras[0])
	{
		UE_LOG(LogVertLevelScript, Log, TEXT("Setting start camera [%s] as the active player camera"), *ArenaCameras[0]->GetName());
		mActiveCamera = ArenaCameras[0];
	}

	if (mActiveCamera.IsValid())
	{
		for (auto iter = GetWorld()->GetPlayerControllerIterator(); iter; ++iter)
		{
			APlayerController* controller = iter->Get();
			controller->ClientSetViewTarget(mActiveCamera.Get());
		}

		mActiveCamera->ActivateCamera();
	}
	else { UE_LOG(LogVertLevelScript, Warning, TEXT("No AVertPlayerCameraActor found, game will default to FPS view.")); }

	return mActiveCamera;
}