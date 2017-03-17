// Copyright Inside Out Games Ltd. 2017

#pragma once

#include "Camera/PlayerCameraManager.h"
#include "VertCameraManager.generated.h"

/**
 * 
 */
UCLASS()
class VERT_API AVertCameraManager : public APlayerCameraManager
{
	GENERATED_BODY()
	
public:
	AVertCameraManager();

	virtual void UpdateCamera(float DeltaTime) override;

private:
	float mDefaultFOV = 90.f;
	float mTestFOV = 120.f;
};
