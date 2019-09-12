// Copyright Inside Out Games Ltd. 2017

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CameraTransitionVolume.generated.h"

UCLASS()
class VERT_API ACameraTransitionVolume : public AActor
{
	GENERATED_BODY()

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Transition")
	class AVertPlayerCameraActor* NewCamera = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Transition")
	float TransitionTime = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Transition")
	bool WaitForTimer = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Transition", meta = (EditCondition = "WaitForTimer"))
	float TimeToWait = 30.f;

private:
	// Overlap volume to trigger level streaming
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	class UBoxComponent* OverlapVolume;

public:
	// Sets default values for this actor's properties
	ACameraTransitionVolume();

protected:
	bool TransitionActiveCamera();

	virtual void PostLoad() override;
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnTimerExpired();

	UFUNCTION()
	void OnBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult & SweepResult);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	bool CameraOverlapped(AVertPlayerCameraActor* camera);

	UFUNCTION()
	void CheckAllOverlappedActors();

private:
	TArray<TWeakObjectPtr<AVertPlayerCameraActor>> mOverlappingCameras;
};
