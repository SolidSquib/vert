// Copyright Inside Out Games Ltd. 2017

#pragma once

#include "Engine/LevelScriptActor.h"
#include "VertLevelScriptActor.generated.h"

/**
 * 
 */
UCLASS()
class VERT_API AVertLevelScriptActor : public ALevelScriptActor
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	class AVertPlayerCameraActor* StartCamera = nullptr;

public:
	TWeakObjectPtr<AVertPlayerCameraActor> GetStartingCamera();

	UFUNCTION(BlueprintCallable, Category = "ActiveCamera")
	void SetActiveCamera(AVertPlayerCameraActor* newCamera, float transitionTime);

protected:
	TWeakObjectPtr<AVertPlayerCameraActor> mActiveCamera = nullptr;
};
