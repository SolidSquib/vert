// Copyright Inside Out Games Ltd. 2017

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ItemSpawner.generated.h"

class AInteractive;

UCLASS()
class VERT_API AItemSpawner : public AActor
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
	float TimeForNewSpawn = 5.f;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mesh")
	class UStaticMeshComponent* SpawnerMesh;

public:
	AItemSpawner();

	virtual AInteractive* AttemptToSpawn();

protected:
	virtual AInteractive* SpawnItem();

	virtual void BeginPlay() override;

	void SpawnItemAndBind();

private:
	UFUNCTION()
	void RemoveCurrentItemReference();

private:
	TWeakObjectPtr<AInteractive> mCurrentItem;
	FTimerHandle mTimerHandle_SpawnTimer;
};
