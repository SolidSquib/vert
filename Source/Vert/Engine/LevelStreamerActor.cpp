// Copyright Inside Out Games Ltd. 2017

#include "LevelStreamerActor.h"
#include "Kismet/GameplayStatics.h"

DECLARE_LOG_CATEGORY_CLASS(LogStreamerActor, Log, All);

// Sets default values
ALevelStreamerActor::ALevelStreamerActor()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	OverlapVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("OverlapVolume"));
	RootComponent = OverlapVolume;

	OverlapVolume->SetCollisionResponseToAllChannels(ECR_Ignore);
	OverlapVolume->SetCollisionObjectType(ECC_StreamingBounds);
	OverlapVolume->SetCollisionResponseToChannel(ECC_CameraPlaceholder, ECR_Overlap);
	OverlapVolume->bGenerateOverlapEvents = true;
}

void ALevelStreamerActor::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	OverlapVolume->OnComponentBeginOverlap.AddUniqueDynamic(this, &ALevelStreamerActor::OnBeginOverlap);
	OverlapVolume->OnComponentEndOverlap.AddUniqueDynamic(this, &ALevelStreamerActor::OnEndOverlap);
}

//************************************
// Method:    OnBeginOverlap
// FullName:  ALevelStreamerActor::OnBeginOverlap
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
void ALevelStreamerActor::OnBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	UE_LOG(LogStreamerActor, Log, TEXT("Began overlap with [%s]"), (OtherActor) ? *OtherActor->GetName() : *OtherComp->GetName());

	AVertPlayerCameraActor* camera = Cast<AVertPlayerCameraActor>(OtherActor);
	if (camera && camera->IsCameraActive())
	{
		// load new levels 
		for (auto level : LevelsToLoad)
		{
			FLatentActionInfo LatentInfo;
			UGameplayStatics::LoadStreamLevel(this, level, true, true, LatentInfo);
		}
	}
}

//************************************
// Method:    OnEndOverlap
// FullName:  ALevelStreamerActor::OnEndOverlap
// Access:    protected 
// Returns:   void
// Qualifier:
// Parameter: UPrimitiveComponent * OverlappedComponent
// Parameter: AActor * OtherActor
// Parameter: UPrimitiveComponent * OtherComp
// Parameter: int32 OtherBodyIndex
//************************************
void ALevelStreamerActor::OnEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	UE_LOG(LogStreamerActor, Log, TEXT("Ended overlap with [%s]"), (OtherActor) ? *OtherActor->GetName() : *OtherComp->GetName());

	AVertPlayerCameraActor* camera = Cast<AVertPlayerCameraActor>(OtherActor);
	if (camera && camera->IsCameraActive())
	{
		//Unload unwanted levels
		for (auto level : LevelsToLoad)
		{
			FLatentActionInfo LatentInfo;
			UGameplayStatics::UnloadStreamLevel(this, level, LatentInfo);
		}
	}	
}