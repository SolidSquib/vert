// Copyright Inside Out Games Limited 2017

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WeaponSpawnVolume.generated.h"

UCLASS()
class VERT_API AWeaponSpawnVolume : public AActor
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
	TArray<AActor*> TargetSpawnPoints;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
	int32 NumberOfPods = 4;

	/* The number of items that each pod will spawn, if -1 each pod will spawn a random number from 1 - 4 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config", meta = (UIMax = 10, UIMin = 1))
	int32 NumberOfItemsPerPod = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Config")
	float TimeBetweenSpawns = 0.5f;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Warning")
	class UParticleSystem* WarningFX = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Warning")
	FName WarningBeamEndParam = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Warning")
	float WarningTime = 2.f;

protected:
	UPROPERTY(VisibleAnywhere, Category = "TriggerVolume")
	class UBoxComponent* TriggerVolume;

public:	
	AWeaponSpawnVolume();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void PostInitializeComponents() override;
	virtual void InitiateDroppodsRecursive();
	void CallSinglePod(int32 targetIndex);

	UFUNCTION(BlueprintNativeEvent)
	void OnTriggerOverlap(UPrimitiveComponent* overlappedComponent, AActor* otherActor, UPrimitiveComponent* otherComp, int32 otherBodyIndex, bool fromSweep, const FHitResult& sweepResult);

private:
	bool mTriggered = false;
	TArray<int32> mChosenTargets;
	int32 mCurrentIndex = 0;
};
