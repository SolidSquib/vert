// Copyright Inside Out Games Ltd. 2017

#pragma once

#include "CoreMinimal.h"
#include "PooledActor.h"
#include "DropPod.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDroppodDelegate, AActor*, spawnedActor);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDroppodPlayerSpawnDelegate, class APawn*, spawnedPawn);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FDroppodPlayerSpawnFailedDelegate);

UCLASS()
class VERT_API ADropPod : public APooledActor
{
	GENERATED_BODY()
	
public:
	UPROPERTY(BlueprintAssignable)
	FDroppodDelegate OnActorSpawned;

	UPROPERTY(BlueprintAssignable)
	FDroppodPlayerSpawnDelegate OnPlayerPawnSpawned;

	UPROPERTY(BlueprintAssignable)
	FDroppodPlayerSpawnFailedDelegate OnPlayerPawnSpawnFailed;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hit")
	int32 Damage = 10;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hit")
	float Knockback = 50;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hit")
	float KnockbackScaling = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hit")
	TSubclassOf<UDamageType> ImpactDamageType;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hit")
	float DigInterpSpeed = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hit")
	float DigTargetDepth = 50.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Payload)
	float DefaultImpulseMagnitude = 1000.f;

protected:
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Mesh")
	class USkeletalMeshComponent* MeshComponent;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Movement")
	class UProjectileMovementComponent* ProjectileMovement;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = Collision)
	class UBoxComponent* PodCollision;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = Collision)
	class USphereComponent* CorrectionSphere;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation")
	class UAnimMontage* EjectAnimation = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Payload")
	TArray<TSubclassOf<AActor>> PayloadActors;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement")
	bool AutoDrop = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement")
	bool AutoManageCollision = true;

	UPROPERTY(Transient)
	TArray<UMaterialInstanceDynamic*> MeshMIDs;

	UPROPERTY()
	class AController* OwningController = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Config|ObjectPool")
	float DespawnTriggerTime = 5.f;

	UPROPERTY(EditDefaultsOnly, Category = "Config|ObjectPool")
	float FlashSpeed = 0.1f;

	UPROPERTY(EditDefaultsOnly, Category = "Config|ObjectPool")
	float FlashForTimeBeforeDespawning = 3.f;

	UPROPERTY(EditDefaultsOnly, Category = "Config|ObjectPool")
	class UParticleSystem* DespawnFX = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Audio")
	class UAkAudioEvent* OnImpactSound = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Audio")
	class UAkAudioEvent* OnEjectSound = nullptr;

public:	
	// Sets default values for this actor's properties
	ADropPod();

	void AddPayloadPawn(TSubclassOf<APawn> pawnClass, AController* owningController);
	void AddPayloadActor(TSubclassOf<AActor> actorClass);
	void InitPodVelocity(const FVector& direction);

	virtual void FellOutOfWorld(const class UDamageType& dmgType) override;
	virtual void Tick(float DeltaSeconds) override;
	
	UFUNCTION(BlueprintCallable)
	void UpdateMeshColours(const FLinearColor& newColour);

	UFUNCTION(BlueprintCallable)
	void EnablePodCollision();

	UFUNCTION(BlueprintCallable)
	FVector GetCollisionBoxExtents() const;

	UFUNCTION(BlueprintCallable)
	float GetCorrectionSphereRadius() const;

protected:
	void StartDespawnTimer();
	void PrepareForDespawn();
	void CancelDespawn();
	void DespawnFlash();

	virtual AActor* SpawnActorFromPod(int32 index);
	virtual void DespawnPod();
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void PostInitializeComponents() override;

	UFUNCTION(BlueprintCallable, Category = "Payload")
	virtual void EjectPayload();

	UFUNCTION(BlueprintImplementableEvent)
	void OnPodInitVelocity();

	UFUNCTION(BlueprintImplementableEvent)
	void OnPodMadeImpact();

	UFUNCTION()
	virtual void PoolBeginPlay();

	UFUNCTION()
	virtual void PoolEndPlay();

	UFUNCTION(BlueprintCallable)
	void DisableAndDestroy();

	UFUNCTION(BlueprintNativeEvent)
	void OnPodHit(const FHitResult& hit);

	UFUNCTION(BlueprintNativeEvent)
	void OnPodOverlap(UPrimitiveComponent* overlappedComponent, AActor* otherActor, UPrimitiveComponent* otherComp, int32 otherBodyIndex, bool fromSweep, const FHitResult& sweepResult);

private:
	FTimerHandle mTimerHandle_SpawnTimer;
	FTimerHandle mTimerHandle_Collision;
	FTimerHandle mTimerHandle_Despawn;
	FTimerHandle mTimerHandle_DespawnFlash;
	FTimerHandle mTimerHandle_DespawnFinish;

	bool mCollisionTriggered = false;
	bool mMadeImpact = false;
	bool mPassedScreen = false;
	bool mEjectedPayload = false;
	FVector mDigTargetLocation = FVector::ZeroVector;
	FCollisionResponseContainer mSavedCollisionProfile;

	TMap<int32, TWeakObjectPtr<AController>> mPayloadPawnControllers;
};
