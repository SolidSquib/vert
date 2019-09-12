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

	void SetShowCameraDebug(bool showDebug);
	void GetBoundsXMinMax(float& minX, float& maxX);

	virtual void UpdateCamera(float DeltaTime) override;
	
	UFUNCTION(BlueprintCallable, Category = "Camera|Utility")
	bool IsPawnOffOfCamera(APawn* pawn);

	UFUNCTION(BlueprintCallable, Category = "Camera|Utility")
	bool IsPawnOutOfBounds(APawn* pawn, const FVector& charPlaneTopLeft, const FVector& charPlaneBottomRight);

	UFUNCTION(BlueprintCallable, Category = "Camera|Utility")
	void GetCurrentCameraBounds(FVector& topLeft, FVector& bottomRight, FVector& center);

	UFUNCTION(BlueprintCallable, Category = "Camera|Utility")
	void GetCurrentCameraDimensions(float& width, float& height);

	UFUNCTION(BlueprintCallable, Category = "Camera|Utility")
	void GetCurrentPlayerBounds(FVector& topLeft, FVector& bottomRight, FVector& bottomLeft, FVector& topRight);

protected:
	void LifeSaverTimeExpired();
	bool DeprojectScreenSpaceToWorld(float screenX, float screenY, FVector& outWorldLocation, FVector& outWorldDirection, APlayerController* controller, const FVector2D viewportSize);
	bool CanPlayerDie() const;

	virtual void UpdateCameraBounds();

private:
	bool mShowDebug = false;
	float mCurrentCameraHeight = 0;
	float mCurrentCameraWidth = 0;
	FVector mCurrentBoundaryCenter = FVector::ZeroVector;
	FVector mCurrentBoundaryTopLeft = FVector::ZeroVector;
	FVector mCurrentBoundaryBottomRight = FVector::ZeroVector;
	FVector mCharPlaneTopLeft = FVector::ZeroVector;
	FVector mCharPlaneBottomRight = FVector::ZeroVector;
	FVector mCharPlaneTopRight = FVector::ZeroVector;
	FVector mCharPlaneBottomLeft = FVector::ZeroVector;
	FTimerHandle mTimerHandle_LifeSaver;
};
