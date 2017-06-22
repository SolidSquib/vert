// Copyright Inside Out Games Ltd. 2017

#include "LevelStreamerActor.h"
#include "Kismet/GameplayStatics.h"

// Sets default values
ALevelStreamerActor::ALevelStreamerActor()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	OverlapVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("OverlapVolume"));
	RootComponent = OverlapVolume;

	OverlapVolume->SetCollisionResponseToAllChannels(ECR_Ignore);
	OverlapVolume->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

	OverlapVolume->OnComponentBeginOverlap.AddUniqueDynamic(this, &ALevelStreamerActor::OnBeginOverlap);
	OverlapVolume->OnComponentEndOverlap.AddUniqueDynamic(this, &ALevelStreamerActor::OnEndOverlap);
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
void ALevelStreamerActor::OnBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult & SweepResult)
{
	if (ACharacter* character = Cast<ACharacter>(OtherActor))
	{
		if (character->IsLocallyControlled() && LevelToLoad != NAME_None)
		{
			FLatentActionInfo LatentInfo;
			UGameplayStatics::LoadStreamLevel(this, LevelToLoad, true, true, LatentInfo);
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
	if (ACharacter* character = Cast<ACharacter>(OtherActor))
	{
		if (character->IsLocallyControlled() && LevelToLoad != NAME_None)
		{
			FLatentActionInfo LatentInfo;
			UGameplayStatics::UnloadStreamLevel(this, LevelToLoad, LatentInfo);
		}
	}
}