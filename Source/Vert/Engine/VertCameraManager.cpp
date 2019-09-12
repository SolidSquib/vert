// Copyright Inside Out Games Ltd. 2017

#include "VertCameraManager.h"

AVertCameraManager::AVertCameraManager()
{

}

//************************************
// Method:    DeprojectScreenSpaceToWorld
// FullName:  AVertCameraManager::DeprojectScreenSpaceToWorld
// Access:    protected 
// Returns:   bool
// Qualifier:
// Parameter: float screenX
// Parameter: float screenY
// Parameter: FVector & outWorldLocation
// Parameter: FVector & outWorldDirection
// Parameter: APlayerController * controller
// Parameter: const FVector2D viewportSize
//************************************
bool AVertCameraManager::DeprojectScreenSpaceToWorld(float screenX, float screenY, FVector& outWorldLocation, FVector& outWorldDirection, APlayerController* controller, const FVector2D viewportSize)
{
	if (!controller)
		return false;

	return controller->DeprojectScreenPositionToWorld((screenX * viewportSize.X + viewportSize.X) * 0.5f, (screenY * viewportSize.Y + viewportSize.Y) * 0.5f, outWorldLocation, outWorldDirection);
}

bool AVertCameraManager::CanPlayerDie() const
{
	AVertGameMode* gameMode = GetWorld()->GetAuthGameMode<AVertGameMode>();
	if (gameMode)
	{
		return gameMode->CanPlayersDie();
	}
	
	return false;
}

//************************************
// Method:    UpdateCamera
// FullName:  AVertCameraManager::UpdateCamera
// Access:    virtual public 
// Returns:   void
// Qualifier:
// Parameter: float DeltaTime
//************************************
void AVertCameraManager::UpdateCamera(float DeltaTime)
{
	Super::UpdateCamera(DeltaTime);

	UpdateCameraBounds();
}

void AVertCameraManager::UpdateCameraBounds()
{
	constexpr float farPlane = 10000.f;
	constexpr float nearPlane = 1000.f;

	// The vectors in world space that we'll be finding
	FVector BoundaryTopLeft;
	FVector BoundaryTopRight;
	FVector BoundaryBottomLeft;
	FVector BoundaryBottomRight;
	FVector BoundaryCenter;

	// We need to find the boundaries of our desired screen space:
	APlayerController* primaryController = GetWorld()->GetFirstLocalPlayerFromController() ? GetWorld()->GetFirstLocalPlayerFromController()->GetPlayerController(GetWorld()) : nullptr;
	APlayerController* controller = GetOwningPlayerController();
	AVertLevelScriptActor* level = Cast<AVertLevelScriptActor>(GetWorld()->GetLevelScriptActor());

	if (primaryController && level && controller)
	{
		FVector dir;
		APawn* pawn = controller->GetPawn();
		const FVector2D ViewportSize = FVector2D(GEngine->GameViewport->Viewport->GetSizeXY());
		
		// You'd likely have these two vectors to adjust what percent of the screen that you're finding the bounds of. For the whole screen
		// it'd be as below. But you could also find the bounds of just the central portion of the screen [-0.5, 0.5] if you wanted to
		// do safe zones etc.
		FVector2D ScreenSpaceMin = FVector2D(-1.f, -1.f);
		FVector2D ScreenSpaceMax = FVector2D(1.f, 1.f);

		// The result of this deprojection is that BoundaryTopLeft will have the starting location of the desired screen bounds
		// and dir will have the direction vector that lets us travel along that screen bound.
		if (DeprojectScreenSpaceToWorld(ScreenSpaceMin.X, ScreenSpaceMin.Y, BoundaryTopLeft, dir, primaryController, ViewportSize))
		{
			// This calculation uses the distance from the camera to the player to determine the world space boundary location
			// that is on the same plane as the player pawn simply by traveling along the boundary that was previously found.
			FVector farTopLeft = BoundaryTopLeft + (dir * farPlane);
			mCharPlaneTopLeft = FMath::LinePlaneIntersection(BoundaryTopLeft, farTopLeft, level->PlayerMovementPlane, -FVector::RightVector);
			BoundaryTopLeft = BoundaryTopLeft + (dir * nearPlane);

			// The same for top right
			DeprojectScreenSpaceToWorld(ScreenSpaceMax.X, ScreenSpaceMin.Y, BoundaryTopRight, dir, primaryController, ViewportSize);
			FVector farTopRight = BoundaryTopRight + (dir * farPlane);
			mCharPlaneTopRight = FMath::LinePlaneIntersection(BoundaryTopRight, farTopRight, level->PlayerMovementPlane, -FVector::RightVector);

			// The same for bottom left
			DeprojectScreenSpaceToWorld(ScreenSpaceMin.X, ScreenSpaceMax.Y, BoundaryBottomLeft, dir, primaryController, ViewportSize);
			FVector farBottomLeft = BoundaryBottomLeft + (dir * farPlane);
			mCharPlaneBottomLeft = FMath::LinePlaneIntersection(BoundaryBottomLeft, farBottomLeft, level->PlayerMovementPlane, -FVector::RightVector);

			// Now the same thing for the other two world space locations.
			DeprojectScreenSpaceToWorld(ScreenSpaceMax.X, ScreenSpaceMax.Y, BoundaryBottomRight, dir, primaryController, ViewportSize);
			FVector farBottomRight = BoundaryBottomRight + (dir * farPlane);
			mCharPlaneBottomRight = FMath::LinePlaneIntersection(BoundaryBottomRight, farBottomRight, level->PlayerMovementPlane, -FVector::RightVector);
			BoundaryBottomRight = BoundaryBottomRight + (dir * nearPlane);

			DeprojectScreenSpaceToWorld(0, 0, BoundaryCenter, dir, primaryController, ViewportSize);
			BoundaryCenter = BoundaryCenter + (dir * nearPlane);

			mCurrentBoundaryCenter = BoundaryCenter;
			mCurrentBoundaryTopLeft = BoundaryTopLeft;
			mCurrentBoundaryBottomRight = BoundaryBottomRight;

			// You now have world space coordinate of the left/right/center. You can calculate the width and height of this play area as follows
			mCurrentCameraWidth = ((BoundaryBottomRight - BoundaryTopLeft) * 0.5f).ProjectOnTo(GetActorRightVector()).Size();
			mCurrentCameraHeight = ((BoundaryTopLeft - BoundaryBottomRight) * 0.5f).ProjectOnTo(GetActorUpVector()).Size();

			if (pawn && !pawn->IsPendingKill() && CanPlayerDie() && IsPawnOffOfCamera(pawn))
			{
				// Check if the player is alive and if they are not already off the frustum
				if (!GetWorldTimerManager().IsTimerActive(mTimerHandle_LifeSaver))
				{
					AVertPlayerController* playerController = Cast<AVertPlayerController>(controller);
					GetWorldTimerManager().SetTimer(mTimerHandle_LifeSaver, this, &AVertCameraManager::LifeSaverTimeExpired, playerController ? playerController->LifeSaverTime : 3.f, false);
				}

				// Check if the pawn is out of the total bounds.
				if (IsPawnOutOfBounds(pawn, mCharPlaneTopLeft, mCharPlaneBottomRight))
				{
					if (AVertCharacter* character = Cast<AVertCharacter>(pawn))
					{
						character->Die();
						GetWorldTimerManager().ClearTimer(mTimerHandle_LifeSaver);
					}
				}
			}
			else
			{
				GetWorldTimerManager().ClearTimer(mTimerHandle_LifeSaver);
			}

#if ENABLE_DRAW_DEBUG
			if (mShowDebug)
			{
				DrawDebugPoint(GetWorld(), mCurrentBoundaryCenter, 32.f, FColor::Red, false);
				DrawDebugPoint(GetWorld(), mCurrentBoundaryTopLeft, 32.f, FColor::Red, false);
				DrawDebugPoint(GetWorld(), mCurrentBoundaryBottomRight, 32.f, FColor::Red, false);

				DrawDebugLine(GetWorld(), mCurrentBoundaryTopLeft, FVector(mCurrentBoundaryBottomRight.X, mCurrentBoundaryTopLeft.Y, mCurrentBoundaryTopLeft.Z), FColor::Red, false);
				DrawDebugLine(GetWorld(), FVector(mCurrentBoundaryBottomRight.X, mCurrentBoundaryTopLeft.Y, mCurrentBoundaryTopLeft.Z), mCurrentBoundaryBottomRight, FColor::Red, false);
				DrawDebugLine(GetWorld(), mCurrentBoundaryBottomRight, FVector(mCurrentBoundaryTopLeft.X, mCurrentBoundaryBottomRight.Y, mCurrentBoundaryBottomRight.Z), FColor::Red, false);
				DrawDebugLine(GetWorld(), FVector(mCurrentBoundaryTopLeft.X, mCurrentBoundaryBottomRight.Y, mCurrentBoundaryBottomRight.Z), mCurrentBoundaryTopLeft, FColor::Red, false);

				DrawDebugPoint(GetWorld(), mCharPlaneTopLeft, 48.f, FColor::Blue, false);
				DrawDebugPoint(GetWorld(), mCharPlaneBottomRight, 48.f, FColor::Blue, false);
				DrawDebugPoint(GetWorld(), mCharPlaneBottomLeft, 48.f, FColor::Blue, false);
				DrawDebugPoint(GetWorld(), mCharPlaneTopRight, 48.f, FColor::Blue, false);

				DrawDebugLine(GetWorld(), mCharPlaneTopLeft, mCharPlaneTopRight, FColor::Blue, false, -1.f, 0, 10.f);
				DrawDebugLine(GetWorld(), mCharPlaneTopRight, mCharPlaneBottomRight, FColor::Blue, false, -1.f, 0, 10.f);
				DrawDebugLine(GetWorld(), mCharPlaneBottomRight, mCharPlaneBottomLeft, FColor::Blue, false, -1.f, 0, 10.f);
				DrawDebugLine(GetWorld(), mCharPlaneBottomLeft, mCharPlaneTopLeft, FColor::Blue, false, -1.f, 0, 10.f);
			}
#endif
		}
	}
}

//************************************
// Method:    GetBoundsXMinMax
// FullName:  AVertCameraManager::GetBoundsXMinMax
// Access:    public 
// Returns:   void
// Qualifier:
// Parameter: float & minX
// Parameter: float & maxX
//************************************
void AVertCameraManager::GetBoundsXMinMax(float& minX, float& maxX)
{
	APlayerController* controller = GetOwningPlayerController();
	AVertGameMode* gameMode = GetWorld()->GetAuthGameMode<AVertGameMode>();
	APawn* pawn = controller ? controller->GetPawn() : nullptr;

	if (pawn && !pawn->IsPendingKill() && controller && gameMode && gameMode->GetActivePlayerCamera())
	{
		FVector camLocation = gameMode->GetActivePlayerCamera()->GetActorLocation();
		FVector playerLocation = pawn->GetActorLocation();

		FVector minIntersect = FMath::LinePlaneIntersection(
			FVector(camLocation.X - 10000.f, playerLocation.Y, playerLocation.Z),
			FVector(camLocation.X + 10000.f, playerLocation.Y, playerLocation.Z),
			mCharPlaneBottomLeft,
			FVector::CrossProduct(FVector::RightVector, (mCharPlaneBottomLeft - mCharPlaneTopLeft).GetSafeNormal()));

		FVector maxIntersect = FMath::LinePlaneIntersection(
			FVector(camLocation.X + 10000.f, playerLocation.Y, playerLocation.Z),
			FVector(camLocation.X - 10000.f, playerLocation.Y, playerLocation.Z),
			mCharPlaneBottomRight,
			FVector::CrossProduct(FVector::RightVector, (mCharPlaneTopRight - mCharPlaneBottomRight).GetSafeNormal()));

		minX = minIntersect.X;
		maxX = maxIntersect.X;
	}
	else
	{
		minX = 0;
		maxX = 0;
	}
}

//************************************
// Method:    SetShowCameraDebug
// FullName:  AVertCameraManager::SetShowCameraDebug
// Access:    public 
// Returns:   void
// Qualifier:
// Parameter: bool showDebug
//************************************
void AVertCameraManager::SetShowCameraDebug(bool showDebug)
{
	mShowDebug = showDebug;
}

//************************************
// Method:    IsPawnOutOfBounds
// FullName:  AVertCameraManager::IsPawnOutOfBounds
// Access:    public 
// Returns:   bool
// Qualifier:
// Parameter: APawn * pawn
// Parameter: const FVector & topLeft
// Parameter: const FVector & bottomRight
//************************************
bool AVertCameraManager::IsPawnOutOfBounds(APawn* pawn, const FVector& topLeft, const FVector& bottomRight)
{
	float safetyNet = (GetWorld()->GetAuthGameMode<AVertGameMode>()) ? GetWorld()->GetAuthGameMode<AVertGameMode>()->AllowedDistanceFromCameraBounds : 0;

	if (pawn && !pawn->IsPendingKill())
	{
		float minX, maxX;
		GetBoundsXMinMax(minX, maxX);

		FVector pawnLocation = pawn->GetActorLocation();
		float top = mCharPlaneTopLeft.Z + safetyNet,
			left = minX - safetyNet,
			bottom = mCharPlaneBottomRight.Z - safetyNet,
			right = maxX + safetyNet;

		if (pawnLocation.X < left
			|| pawnLocation.X > right
			|| pawnLocation.Z < bottom
			|| pawnLocation.Z > top)
		{
			return true;
		}
	}

	return false;
}

//************************************
// Method:    IsPawnOffOfCamera
// FullName:  AVertCameraManager::IsPawnOffOfCamera
// Access:    public 
// Returns:   bool
// Qualifier:
// Parameter: APawn * pawn
// Parameter: const FVector & boundsTopLeft
// Parameter: const FVector & boundsBottomRight
//************************************
bool AVertCameraManager::IsPawnOffOfCamera(APawn* pawn)
{
	return !UVertUtilities::IsActorInFrustum(this, pawn);
}

//************************************
// Method:    LifeSaverTimeExpired
// FullName:  AVertPlayerController::LifeSaverTimeExpired
// Access:    protected 
// Returns:   void
// Qualifier:
//************************************
void AVertCameraManager::LifeSaverTimeExpired()
{
	if (GetOwningPlayerController())
	{
		if (AVertCharacterBase* vertCharacter = Cast<AVertCharacterBase>(GetOwningPlayerController()->GetPawn()))
		{
			vertCharacter->Die();
		}
	}
}

//************************************
// Method:    GetCurrentCameraBounds
// FullName:  AVertCameraManager::GetCurrentCameraBounds
// Access:    public 
// Returns:   void
// Qualifier:
// Parameter: FVector & topLeft
// Parameter: FVector & bottomRight
// Parameter: FVector & center
//************************************
void AVertCameraManager::GetCurrentCameraBounds(FVector& topLeft, FVector& bottomRight, FVector& center)
{
	topLeft = mCurrentBoundaryTopLeft;
	bottomRight = mCurrentBoundaryBottomRight;
	center = mCurrentBoundaryCenter;
}

//************************************
// Method:    GetCurrentCameraDimensions
// FullName:  AVertCameraManager::GetCurrentCameraDimensions
// Access:    public 
// Returns:   void
// Qualifier:
// Parameter: float & width
// Parameter: float & height
//************************************
void AVertCameraManager::GetCurrentCameraDimensions(float& width, float& height)
{
	width = mCurrentCameraWidth;
	height = mCurrentCameraHeight;
}

//************************************
// Method:    GetCurrentPlayerBounds
// FullName:  AVertCameraManager::GetCurrentPlayerBounds
// Access:    public 
// Returns:   void
// Qualifier:
// Parameter: FVector & topLeft
// Parameter: FVector & bottomRight
//************************************
void AVertCameraManager::GetCurrentPlayerBounds(FVector& topLeft, FVector& bottomRight, FVector& bottomLeft, FVector& topRight)
{
	topLeft = mCharPlaneTopLeft;
	bottomRight = mCharPlaneBottomRight;
	bottomLeft = mCharPlaneBottomLeft;
	topRight = mCharPlaneTopRight;
}