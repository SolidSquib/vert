// Copyright Inside Out Games Ltd. 2017

#pragma once

#include "Engine/LevelScriptActor.h"
#include "VertLevelScriptActor.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FVertLevelDelegate);

USTRUCT(BlueprintType)
struct FWwiseLevelMusic
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Music)
	class UAkAudioEvent* Event = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Music)
	FName SwitchGroup = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Music)
	FName SwitchValue = NAME_None;
};

/**
 * 
 */
UCLASS()
class VERT_API AVertLevelScriptActor : public ALevelScriptActor
{
	GENERATED_BODY()

public:
	/* Denotes the primary camera for this arena. If this level is a persistent level it will determine the starting camera for the level. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	TArray<class AVertPlayerCameraActor*> ArenaCameras;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Catch")
	class ACatchCharacter* CatchCharacter = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MovementPlane")
	FVector PlayerMovementPlane = FVector::RightVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Music")
	FWwiseLevelMusic LevelMusic;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hacks")
	bool StartOnAutoSpline = false;

public:
	TWeakObjectPtr<AVertPlayerCameraActor> GetStartingCamera();
	FVector GetPlaneConstraintOrigin() const;

	virtual void Tick(float DeltaTime) override;

	UFUNCTION(BlueprintCallable, Category = "Music")
	FWwiseLevelMusic GetLevelMusic() const { return LevelMusic; }
	
protected:
	virtual void PostInitializeComponents() override;
	virtual void BeginPlay() override;

	UFUNCTION()
	void InternalOnPrimaryCameraBecomeActive(AVertPlayerCameraActor* camera);

	UFUNCTION()
	void InternalOnPrimaryCameraBecomeInactive(AVertPlayerCameraActor* camera);
	
	UFUNCTION()
	void InternalOnPrimaryCameraReachedEndOfTrack(AVertPlayerCameraActor* camera);

	UFUNCTION(BlueprintImplementableEvent, Category = "LevelEvents")
	void OnPrimaryCameraBecomeActive(int32 cameraIndex, AVertPlayerCameraActor* camera);

	UFUNCTION(BlueprintImplementableEvent, Category = "LevelEvents")
	void OnPrimaryCameraBecomeInactive(int32 cameraIndex, AVertPlayerCameraActor* camera);

	UFUNCTION(BlueprintImplementableEvent, Category = "LevelEvents")
	void OnPrimaryCameraReachedEndOfTrack(int32 cameraIndex, AVertPlayerCameraActor* camera);

protected:
	TWeakObjectPtr<AVertPlayerCameraActor> mActiveCamera = nullptr;
	
private:
	bool mPlayerCameraSet = false;
};
