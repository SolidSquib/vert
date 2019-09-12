// Copyright Inside Out Games Ltd. 2017

#include "CameraTransitionVolume.h"
#include "Kismet/GameplayStatics.h"

DECLARE_LOG_CATEGORY_CLASS(LogCameraTransition, Log, All);

// Sets default values
ACameraTransitionVolume::ACameraTransitionVolume()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	OverlapVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("OverlapVolume"));
	OverlapVolume->SetCollisionObjectType(ECC_StreamingBounds);
	OverlapVolume->SetCollisionResponseToAllChannels(ECR_Ignore);
	OverlapVolume->SetCollisionResponseToChannel(ECC_CameraPlaceholder, ECR_Overlap);

	RootComponent = OverlapVolume;
}

void ACameraTransitionVolume::PostLoad()
{
	Super::PostLoad();

	if (OverlapVolume)
	{
		OverlapVolume->OnComponentBeginOverlap.AddUniqueDynamic(this, &ACameraTransitionVolume::OnBeginOverlap);
	}
}

void ACameraTransitionVolume::BeginPlay()
{
	Super::BeginPlay();

	if (AVertGameMode* gm = GetWorld()->GetAuthGameMode<AVertGameMode>())
	{
		if(WaitForTimer)
			gm->OnActiveCameraChanged.AddDynamic(this, &ACameraTransitionVolume::CheckAllOverlappedActors);
	}
}

//************************************
// Method:    CheckAllOverlappedActors
// FullName:  ACameraTransitionVolume::CheckAllOverlappedActors
// Access:    protected 
// Returns:   void
// Qualifier:
//************************************
void ACameraTransitionVolume::CheckAllOverlappedActors()
{
	UE_LOG(LogCameraTransition, Log, TEXT("Checking for overlapping cameras..."));

	AVertGameMode* gm = GetWorld()->GetAuthGameMode<AVertGameMode>();

	// Check for overlapping cameras when this volume is spawned, paramount for timed transitions.
	if (gm && OverlapVolume && gm->GetActivePlayerCamera())
	{
		TArray<AActor*> cameras;
		OverlapVolume->GetOverlappingActors(cameras, AVertPlayerCameraActor::StaticClass());
		if (cameras.Find(gm->GetActivePlayerCamera()) != INDEX_NONE)
		{
			CameraOverlapped(gm->GetActivePlayerCamera());
		}
	}
}

//************************************
// Method:    CameraOverlapped
// FullName:  ACameraTransitionVolume::CameraOverlapped
// Access:    protected 
// Returns:   void
// Qualifier:
// Parameter: AVertPlayerCameraActor * camera
//************************************
bool ACameraTransitionVolume::CameraOverlapped_Implementation(AVertPlayerCameraActor* camera)
{
	if (camera && camera->IsCameraActive())
	{
		if (WaitForTimer && TimeToWait > 0.0001f)
		{
			AVertGameMode* gm = GetWorld()->GetAuthGameMode<AVertGameMode>();
			if (gm)
			{
				gm->OnTimerExpired.AddDynamic(this, &ACameraTransitionVolume::OnTimerExpired);
				gm->StartTimer(TimeToWait);
			}
		}
		else
		{
			TransitionActiveCamera();
		}

		return true;
	}

	return false;
}

//************************************
// Method:    TransitionActiveCamera
// FullName:  ACameraTransitionVolume::TransitionActiveCamera
// Access:    protected 
// Returns:   void
// Qualifier:
//************************************
bool ACameraTransitionVolume::TransitionActiveCamera()
{
	if (AVertGameMode* gm = GetWorld()->GetAuthGameMode<AVertGameMode>())
	{
		gm->SetPlayerCamera(NewCamera, TransitionTime);

		if (HasAuthority())
			Destroy();

		return true;
	}

	return false;
}

//************************************
// Method:    TransitionAndUnbindDelegate
// FullName:  ACameraTransitionVolume::TransitionAndUnbindDelegate
// Access:    protected 
// Returns:   void
// Qualifier:
//************************************
void ACameraTransitionVolume::OnTimerExpired()
{
	TransitionActiveCamera();	
	if (AVertGameMode* gameMode = GetWorld()->GetAuthGameMode<AVertGameMode>())
	{
		gameMode->OnTimerExpired.RemoveDynamic(this, &ACameraTransitionVolume::OnTimerExpired);
	}
}

//************************************
// Method:    OverlapBegins
// FullName:  ALevelStreamerActor::OverlapBegins
// Access:    protected 
// Returns:   void
// Qualifier:
// Parameter: UPrimitiveComponent * OverlappedComponent
// Parameter: AActor * OtherActor
// Parameter: UPrimitiveComponent * OtherComp
// Parameter: int32 OtherBodyIndex
// Parameter: bool bFromSweep
// Parameter: const FHitResult & SweepResult
//************************************
void ACameraTransitionVolume::OnBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	UE_LOG(LogCameraTransition, Log, TEXT("Actor [%s] overlapped with CameraTransitionVolume [%s]"), *OtherActor->GetName(), *GetName());

	if (GetNetMode() == NM_DedicatedServer)
		return;

	if (AVertPlayerCameraActor* camera = Cast<AVertPlayerCameraActor>(OtherActor))
	{
		CameraOverlapped(camera);
	}	
}