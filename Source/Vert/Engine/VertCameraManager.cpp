// Copyright Inside Out Games Ltd. 2017

#include "Vert.h"
#include "VertCameraManager.h"

AVertCameraManager::AVertCameraManager()
{

}

void AVertCameraManager::UpdateCamera(float DeltaTime)
{
	AVertPlayerController* vertPC = PCOwner ? Cast<AVertPlayerController>(PCOwner) : nullptr;
	if (vertPC)
	{
		const float targetFOV = vertPC->IsTestingFOV() ? mTestFOV : mDefaultFOV;
		DefaultFOV = FMath::FInterpTo(DefaultFOV, targetFOV, DeltaTime, 20.f);
	}

	Super::UpdateCamera(DeltaTime);
}