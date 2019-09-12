// Copyright Inside Out Games Limited 2017

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "PlayerDroppod.generated.h"

USTRUCT()
struct FObstacleTrace
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Range")
	float ForwardRange = 150.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Range")
	float UpwardTraceZStartOffset = 500.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Range")
	float UpwardTraceForwardOffset = 70.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Range")
	float UpwardRange = 550.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Range")
	float TraceRadius = 20.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Channel")
	TEnumAsByte<ECollisionChannel> TraceChannel;

	UPROPERTY(EditAnywhere, Category = "Debug")
	bool ShowDebug = false;
};

UCLASS()
class VERT_API APlayerDroppod : public ACharacter
{
	GENERATED_BODY()

public:
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

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Config")
	FObstacleTrace ObstacleTracing;

protected:
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Movement")
	class UProjectileMovementComponent* ProjectileMovement;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation")
	class UAnimMontage* EjectAnimation = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Audio")
	class UAkAudioEvent* OnImpactSound = nullptr;

	UPROPERTY(Transient)
	TArray<UMaterialInstanceDynamic*> MeshMIDs;

	UPROPERTY(EditDefaultsOnly, Category = "Config")
	float DespawnTriggerTime = 5.f;

	UPROPERTY(EditDefaultsOnly, Category = "Config")
	float FlashSpeed = 0.1f;

	UPROPERTY(EditDefaultsOnly, Category = "Config")
	float FlashForTimeBeforeDespawning = 3.f;

	UPROPERTY(EditDefaultsOnly, Category = "Config")
	class UParticleSystem* DespawnFX = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Config")
	float ForcedLaunchSpeed = 3000.f;

	UPROPERTY(EditDefaultsOnly, Category = "Config")
	float NaturalLaunchSpeed = 1000.f;

	UPROPERTY(EditDefaultsOnly, Category = "Config")
	float AimTime = 5.f;

public:
	void ActionDrop();
	void UpdateTeamColoursAllMIDs();

	// Sets default values for this character's properties
	APlayerDroppod();

	virtual void PossessedBy(AController* NewController) override;

	UFUNCTION(BlueprintCallable)
	void UpdateMeshColours(const FLinearColor& newColour);

protected:
	void ActionMoveRight(float value);
	void UpdateTeamColours(UMaterialInstanceDynamic* useMIDs);
	void StartDespawnTimer();
	void PrepareForDespawn();
	void CancelDespawn();
	void DespawnFlash();
	void DisableAndDestroy();

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void PostInitializeComponents() override;
	virtual void BeginPlay() override;

	UFUNCTION(BlueprintImplementableEvent)
	void OnPodInitVelocity();

	UFUNCTION(BlueprintNativeEvent)
	void OnPodStopMoving(const FHitResult& hit);

	UFUNCTION(BlueprintNativeEvent)
	void OnPodOverlap(UPrimitiveComponent* overlappedComponent, AActor* otherActor, UPrimitiveComponent* otherComp, int32 otherBodyIndex, bool fromSweep, const FHitResult& sweepResult);

	UFUNCTION()
	void EjectPlayer(bool Forced);

public:	
	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	virtual void OnRep_PlayerState() override; /** [client] perform PlayerState related setup */

private:
	bool PerformObstacleTrace(const FVector& start, const FVector& end, FHitResult& hit);
	bool TraceForwardForObstacle(FHitResult& hit);
	bool TraceUpwardsForObstacle(FHitResult& hit);

private:
	bool mLaunchedForDeploy = false;
	TWeakObjectPtr<class AVertPlayerController> mVertPlayerController = nullptr;

	// Standard droppod stuff
	FTimerHandle mTimerHandle_SpawnTimer;
	FTimerHandle mTimerHandle_AutoDrop;
	FTimerHandle mTimerHandle_Despawn;
	FTimerHandle mTimerHandle_DespawnFlash;
	FTimerHandle mTimerHandle_DespawnFinish;

	bool mCollisionTriggered = false;
	bool mMadeImpact = false;
	bool mPassedScreen = false;
	bool mEjectedPayload = false;
	FVector mDigTargetLocation = FVector::ZeroVector;
	FCollisionResponseContainer mSavedCollisionProfile;
};
