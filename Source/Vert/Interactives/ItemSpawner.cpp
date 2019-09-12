// Copyright Inside Out Games Ltd. 2017

#include "ItemSpawner.h"
#include "Interactive.h"
#include "Components/StaticMeshComponent.h"
#include "VertGameMode.h"

// Sets default values
AItemSpawner::AItemSpawner()
	: SpawnerMesh(CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SpawnerMesh")))
{
	SpawnerMesh->SetCollisionObjectType(ECC_WorldStatic);
	SpawnerMesh->SetCollisionResponseToAllChannels(ECR_Ignore);

 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
}

void AItemSpawner::BeginPlay()
{
	Super::BeginPlay();

	GetWorldTimerManager().SetTimer(mTimerHandle_SpawnTimer, this, &AItemSpawner::SpawnItemAndBind, 1.f, false);
}

//************************************
// Method:    AttemptToSpawn
// FullName:  AItemSpawner::AttemptToSpawn
// Access:    virtual public 
// Returns:   AInteractive*
// Qualifier:
//************************************
AInteractive* AItemSpawner::AttemptToSpawn()
{
	if (!mCurrentItem.IsValid())
	{
		return SpawnItem();
	}

	return nullptr;
}

//************************************
// Method:    SpawnItem
// FullName:  AItemSpawner::SpawnItem
// Access:    virtual protected 
// Returns:   AInteractive*
// Qualifier:
//************************************
AInteractive* AItemSpawner::SpawnItem()
{
	if (AVertGameMode* gm = GetWorld()->GetAuthGameMode<AVertGameMode>())
	{
		FVector location = GetActorLocation();
		FTransform spawnTransform(GetActorRotation(), { location.X, location.Y, location.Z + 100.f });
		mCurrentItem = gm->RequestRandomItemSpawn(spawnTransform);

		if (mCurrentItem.IsValid())
		{
			mCurrentItem->EnableInteractionDetection();
			return mCurrentItem.Get();
		}
	}

	return nullptr;
}

//************************************
// Method:    SpawnItemAndBind
// FullName:  AItemSpawner::SpawnItemAndBind
// Access:    private 
// Returns:   void
// Qualifier:
//************************************
void AItemSpawner::SpawnItemAndBind()
{
	if (WasRecentlyRendered())
	{
		if (AInteractive* item = AttemptToSpawn())
		{
			item->OnInteract.AddDynamic(this, &AItemSpawner::RemoveCurrentItemReference);
		}
	}
	else if(!GetWorldTimerManager().IsTimerActive(mTimerHandle_SpawnTimer))
	{
		GetWorldTimerManager().SetTimer(mTimerHandle_SpawnTimer, this, &AItemSpawner::SpawnItemAndBind, TimeForNewSpawn, false);
		//GetWorldTimerManager().SetTimer(mTimerHandle_SpawnTimer, [this](void) {
		//	SpawnItemAndBind();
		//}, TimeForNewSpawn, false);
	}
}

//************************************
// Method:    RemoveCurrentItemReference
// FullName:  AItemSpawner::RemoveCurrentItemReference
// Access:    private 
// Returns:   void
// Qualifier:
//************************************
void AItemSpawner::RemoveCurrentItemReference()
{
	if (mCurrentItem.IsValid())
	{
		mCurrentItem->OnInteract.RemoveDynamic(this, &AItemSpawner::RemoveCurrentItemReference);
		mCurrentItem = nullptr;
		GetWorldTimerManager().SetTimer(mTimerHandle_SpawnTimer, this, &AItemSpawner::SpawnItemAndBind, TimeForNewSpawn, false);
	}
}