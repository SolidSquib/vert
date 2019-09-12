// Copyright Inside Out Games Ltd. 2017

#pragma once

#include "CoreMinimal.h"
#include "PooledActor.h"
#include "ItemDropPod.generated.h"

UCLASS()
class VERT_API AItemDropPod : public APooledActor
{
	GENERATED_BODY()
	
public:
	UPROPERTY(BlueprintReadWrite, Category = "ItemSpawning")
	TArray<TSubclassOf<class AInteractive>> ItemQueue;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ItemSpawning")
	float SpawnRate = 0.8f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Collision")
	class UBoxComponent* BoxCollision;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation")
	class UAnimMontage* SpawnAnimation;

public:	
	// Sets default values for this actor's properties
	AItemDropPod();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	virtual void PostInitializeComponents() override;
	
private:
	TWeakObjectPtr<class AItemSpawner> mItemSpawner = nullptr;
	FTimerHandle mTimerHandle_SpawnRateTimer;
	FTimerHandle mTimerHandle_PreparingDropTimer;
};
