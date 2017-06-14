// Copyright Inside Out Games Ltd. 2017

#include "VertPlayerCameraActor.h"
#include "Vert.h"
#include "Kismet/KismetTextLibrary.h"

DEFINE_LOG_CATEGORY(LogVertPlayerCamera);

// Sets default values
AVertPlayerCameraActor::AVertPlayerCameraActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(SceneRoot);

	// Create a camera boom attached to the root (capsule)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->TargetArmLength = 1000.0f;
	CameraBoom->SocketOffset = FVector(0.0f, 0.0f, 75.0f);
	CameraBoom->bAbsoluteRotation = true;
	CameraBoom->bDoCollisionTest = false;
	CameraBoom->RelativeRotation = FRotator(0.0f, -90.0f, 0.0f);
	CameraBoom->SetupAttachment(RootComponent);

	// Create an orthographic camera (no perspective) and attach it to the boom
	CameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("SideViewCamera"));
	CameraComponent->ProjectionMode = ECameraProjectionMode::Perspective;
	CameraComponent->OrthoWidth = 2048.0f;
	CameraComponent->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);

	// Prevent all automatic rotation behavior on the camera, character, and camera component
	CameraBoom->bAbsoluteRotation = true;
}

void AVertPlayerCameraActor::ActivateCamera()
{
	SetActorTickEnabled(true);
	OnCameraBecomeActive.Broadcast();
}

void AVertPlayerCameraActor::DeactivateCamera()
{
	SetActorTickEnabled(false);
	OnCameraBecomeInactive.Broadcast();
}

// Called when the game starts or when spawned
void AVertPlayerCameraActor::BeginPlay()
{
	Super::BeginPlay();

	PlayerSpline = (PlayerSpline) ? PlayerSpline : ((CameraSpline) ? CameraSpline : nullptr);
	CameraSpline = (CameraSpline) ? CameraSpline : ((PlayerSpline) ? PlayerSpline : nullptr);

	mActiveGameMode = GetWorld()->GetAuthGameMode<AVertGameMode>();

	SetupDebugNumbers();
}

// Called every frame
void AVertPlayerCameraActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Find the mean location of the pawns we wish to follow
	UpdateTargetCameraPosition();

	// If we have any splines attached to this camera, figure out the new desired location from them.
	if (CameraSpline != nullptr || PlayerSpline != nullptr)
	{
		FVector zoomTarget;

		if (mHasReachedEnd&&StopAtEnd)
		{
			zoomTarget = MakePositionVectorForSpline(CameraSpline->GetLocationAtTime(1.f, ESplineCoordinateSpace::World, ConstantVelocity));
			
		}		
		else if (IsAutoSpline)
		{
			mSplineCurrentTime = FMath::FInterpConstantTo(mSplineCurrentTime, 1.f, DeltaTime, AutoSplineSpeed);
			zoomTarget = MakePositionVectorForSpline(CameraSpline->GetLocationAtTime(mSplineCurrentTime, ESplineCoordinateSpace::World, ConstantVelocity));
		}
		else
		{
			mSplineDesiredTime = RecursiveDistanceCheck(SplineIterationMax, 0.f, 1.f);
			zoomTarget = MakePositionVectorForSpline(UpdateDesiredTime(DeltaTime));
		}
		
		UpdateCamera(DeltaTime, zoomTarget);
		UpdateCameraZoom(DeltaTime, zoomTarget);
		DebugSplineMovement();
	}
	else 
	{
		UpdateCamera(DeltaTime);
		UpdateCameraZoom(DeltaTime, mTargetLocation);
	}

	
	if (LockPawnsToBounds)
	{
		int i = 0; // placeholder
		// CorrectPawnPositions(largestDistance);
	}
}

//************************************
// Method:    UpdateTargetCameraPosition
// FullName:  AVertPlayerCameraActor::UpdateTargetCameraPosition
// Access:    protected 
// Returns:   void
// Qualifier:
// Parameter: FVector & meanLocation
// Parameter: float & hLengthSqr
// Parameter: float & vLengthSqr
//************************************
void AVertPlayerCameraActor::UpdateTargetCameraPosition()
{
	const TArray<APawn*>& pawns = mActiveGameMode->GetFollowedActors();

	if (mPawnOverride > -1)
	{
		if (pawns.Num() > mPawnOverride)
		{
			SetActorLocation(pawns[mPawnOverride]->GetActorLocation());
			CameraBoom->TargetArmLength = (mZoomOverride > -1) ? mZoomOverride : 1000.f;
		} else { UE_LOG(LogVertPlayerCamera, Error, TEXT("Overriden pawn does not exist in array.")); }
	}
	else
	{
		FVector meanLocation = FVector::ZeroVector;
		float largestSquareH = 0;
		float largestSquareV = 0;

		for (APawn* pawn : pawns)
		{
			// Add the new pawn's location to the current value
			meanLocation += pawn->GetActorLocation();
		}
		// determine the average location and set this actors position to that (in the center of all pawns)
		meanLocation /= pawns.Num();
		mTargetLocation = meanLocation;
	}	
}

//************************************
// Method:    UpdateCameraZoom
// FullName:  AVertPlayerCameraActor::UpdateCameraZoom
// Access:    protected 
// Returns:   void
// Qualifier:
//************************************
void AVertPlayerCameraActor::UpdateCameraZoom(float deltaTime, const FVector& zoomTarget)
{
	const TArray<APawn*>& pawns = mActiveGameMode->GetFollowedActors();

	if (mZoomOverride < 0)
	{
		float largestSquareH = 0, largestSquareV = 0;

		// Find the largest distance (vertical and horizontal) from each pawn to the desired camera location
		for (APawn* pawn : pawns)
		{
			FVector distance = zoomTarget - pawn->GetActorLocation();
			FVector horizontal = FVector::CrossProduct(distance, CameraComponent->GetUpVector());
			FVector vertical = FVector::CrossProduct(distance, CameraComponent->GetRightVector());

			largestSquareH = FMath::Max(horizontal.SizeSquared(), largestSquareH);
			largestSquareV = FMath::Max(vertical.SizeSquared(), largestSquareV);
		}

		// Determine the final length of the largest two distances, used for camera zoom
		float targetH = FMath::Clamp(FMath::Sqrt(largestSquareH) * 2, MinArmLength, MaxArmLength);
		float targetV = FMath::Clamp(FMath::Sqrt(largestSquareV) * 3.5f, MinArmLength, MaxArmLength);

		CameraBoom->TargetArmLength = FMath::FInterpTo(CameraBoom->TargetArmLength, FMath::Max(targetH, targetV), deltaTime, InterpSpeed);
	}
	else
	{
		CameraBoom->TargetArmLength = mZoomOverride;
	}
}

//************************************
// Method:    CorrectPawnPositions
// FullName:  AVertPlayerCameraActor::CorrectPawnPositions
// Access:    protected 
// Returns:   void
// Qualifier:
// Parameter: const FVector & largestDistance
//************************************
void AVertPlayerCameraActor::CorrectPawnPositions(const FVector& largestDistance)
{
	const TArray<APawn*>& pawns = mActiveGameMode->GetFollowedActors();

	UE_LOG(LogVertPlayerCamera, Warning, TEXT("Function not fully implemented, behaviour may be undesirable."));

	for (APawn* pawn : pawns)
	{
		FVector clamped = largestDistance.GetClampedToMaxSize(MaximumDistance);
		FVector targetLocation = (FVector::DotProduct(pawn->GetActorLocation(), CameraComponent->GetRightVector()) > SMALL_NUMBER)
			? GetActorLocation() + (clamped*0.5f)
			: GetActorLocation() - (clamped*0.5f);
		pawn->SetActorLocation(targetLocation);
	}
}

//************************************
// Method:    OverridePawnFollow
// FullName:  AVertPlayerCameraActor::OverridePawnFollow
// Access:    public 
// Returns:   void
// Qualifier:
// Parameter: int32 pawnIndex
//************************************
void AVertPlayerCameraActor::OverridePawnFollow(int32 pawnIndex)
{
	mPawnOverride = pawnIndex;
}

//************************************
// Method:    OverrideCameraZoom
// FullName:  AVertPlayerCameraActor::OverrideCameraZoom
// Access:    public 
// Returns:   void
// Qualifier:
// Parameter: int32 cameraZoomAmount
//************************************
void AVertPlayerCameraActor::OverrideCameraZoom(int32 cameraZoomAmount)
{
	mZoomOverride = cameraZoomAmount;
}

//************************************
// Method:    SetupDebugNumbers
// FullName:  AVertPlayerCameraActor::SetupDebugNumbers
// Access:    private 
// Returns:   void
// Qualifier:
//************************************
void AVertPlayerCameraActor::SetupDebugNumbers()
{
	if (ShowDebugInfo)
	{
		AddSplineNumbers(PlayerSpline);
		AddSplineNumbers(CameraSpline);
	}
}

//************************************
// Method:    AddSplineNumbers
// FullName:  AVertPlayerCameraActor::AddSplineNumbers
// Access:    private 
// Returns:   void
// Qualifier:
// Parameter: ASplineActor * spline
//************************************
void AVertPlayerCameraActor::AddSplineNumbers(ASplineActor* spline)
{
	for (int i = 0; i < spline->GetNumberOfSplinePoints(); i++)
	{
		FVector location;
		FVector tangent;

		spline->GetLocationAndTangentAtSplinePoint(i, location, tangent, ESplineCoordinateSpace::Local);
		location += FVector(0.f, 0.f, 20.f);

		FRotator rotator = UKismetMathLibrary::MakeRotFromX(tangent);
		FRotator newRotator = UKismetMathLibrary::ComposeRotators(rotator, FRotator(0.f, 0.f, 180.f));

		if (UTextRenderComponent* renderText = Cast<UTextRenderComponent>(AddComponent(FName("UTextRenderComponent"), false, UKismetMathLibrary::MakeTransform(location, newRotator, FVector(1.f, 1.f, 1.f)), nullptr)))
		{
			renderText->SetText(UKismetTextLibrary::Conv_IntToText(i));
		}
	}
}

//************************************
// Method:    DebugSplineMovement
// FullName:  AVertPlayerCameraActor::DebugSplineMovement
// Access:    private 
// Returns:   void
// Qualifier:
//************************************
void AVertPlayerCameraActor::DebugSplineMovement()
{
	if (ShowDebugInfo)
	{
		FVector playerCurrentLocation = PlayerSpline->GetLocationAtTime(mSplineCurrentTime, ESplineCoordinateSpace::World, ConstantVelocity);
		FVector playerDesiredLocation = PlayerSpline->GetLocationAtTime(mSplineDesiredTime, ESplineCoordinateSpace::World, ConstantVelocity);
		FVector cameraCurrentLocation = CameraSpline->GetLocationAtTime(mSplineCurrentTime, ESplineCoordinateSpace::World, ConstantVelocity);
		FVector cameraDesiredLocation = CameraSpline->GetLocationAtTime(mSplineDesiredTime, ESplineCoordinateSpace::World, ConstantVelocity);

		FVector playerSplineRight = PlayerSpline->GetRightVectorAtTime(mSplineCurrentTime, ESplineCoordinateSpace::World, ConstantVelocity);
		FVector playerSplineForward = PlayerSpline->GetDirectionAtTime(mSplineCurrentTime, ESplineCoordinateSpace::World, ConstantVelocity);

		if (UWorld* world = GetWorld())
		{
			DrawDebugSphere(world, playerCurrentLocation, 50.f, 12, FColor::Red);
			DrawDebugSphere(world, playerDesiredLocation, 50.f, 12, FColor::Orange);
			DrawDebugSphere(world, cameraCurrentLocation, 50.f, 12, FColor::Blue);
			DrawDebugSphere(world, cameraDesiredLocation, 50.f, 12, FColor::Cyan);
			DrawDebugDirectionalArrow(world, playerDesiredLocation, playerDesiredLocation + (playerSplineForward*50.f), 12.f, FColor::Green, 0.f, 3.f);
			DrawDebugDirectionalArrow(world, playerDesiredLocation, playerDesiredLocation + (playerSplineRight*50.f), 12.f, FColor::Yellow, 0.f, 3.f);
		}
	}
}

//************************************
// Method:    RecursiveDistanceCheck
// FullName:  AVertPlayerCameraActor::RecursiveDistanceCheck
// Access:    private 
// Returns:   float
// Qualifier:
// Parameter: int iterations
// Parameter: float startTime
// Parameter: float endTime
//************************************
float AVertPlayerCameraActor::RecursiveDistanceCheck(int iterations, float startTime, float endTime)
{
	float startDistance = (PlayerSpline->GetLocationAtTime(startTime, ESplineCoordinateSpace::World, ConstantVelocity) - mTargetLocation).Size();
	float endDistance = (PlayerSpline->GetLocationAtTime(endTime, ESplineCoordinateSpace::World, ConstantVelocity) - mTargetLocation).Size();
	float best;

	if (startDistance < endDistance)
	{
		if (iterations <= 1)
		{
			best = startTime;
		}
		else
		{
			iterations -= 1;

			if (startDistance <= DistanceThreshold)
			{
				best = startTime;
			}
			else
			{
				best = RecursiveDistanceCheck(iterations, startTime, UKismetMathLibrary::Lerp(startTime, endTime, 0.5f));
			}
		}
	}
	else
	{
		if (iterations <= 0)
		{
			best = endTime;
		}
		else
		{
			iterations -= 1;

			if (endDistance <= DistanceThreshold)
			{
				best = endTime;
			}
			else
			{
				best = RecursiveDistanceCheck(iterations, UKismetMathLibrary::Lerp(startTime, endTime, 0.5f), endTime);
			}
		}
	}

	return best;
}

//************************************
// Method:    MakePositionVectorForSpline
// FullName:  AVertPlayerCameraActor::MakePositionVectorForSpline
// Access:    private 
// Returns:   FVector
// Qualifier:
// Parameter: const FVector & desiredSplineLocation
//************************************
FVector AVertPlayerCameraActor::MakePositionVectorForSpline(const FVector& desiredSplineLocation)
{
	if (LockX&&LockY&&LockZ)
		return desiredSplineLocation;

	FVector diff = (mTargetLocation - desiredSplineLocation);
	FVector targetVector = desiredSplineLocation;

	if (!LockX)
	{
		FVector maxX(diff.X, 0, 0);
		targetVector.X = desiredSplineLocation.X + maxX.GetClampedToMaxSize(SplineXFreedom).X;
	}
	if (!LockY)
	{
		FVector maxY(diff.Y, 0, 0);
		targetVector.Y = desiredSplineLocation.Y + maxY.GetClampedToMaxSize(SplineYFreedom).Y;
	}
	if (!LockZ)
	{
		FVector maxZ(diff.Z, 0, 0);
		targetVector.Z = desiredSplineLocation.Z + maxZ.GetClampedToMaxSize(SplineZFreedom).Z;
	}

	return targetVector;
}

//************************************
// Method:    UpdateDesiredTime
// FullName:  AVertPlayerCameraActor::UpdateDesiredTime
// Access:    private 
// Returns:   FVector
// Qualifier:
// Parameter: float DeltaTime
//************************************
FVector AVertPlayerCameraActor::UpdateDesiredTime(float DeltaTime)
{
	float timeDifference = mSplineDesiredTime - mSplineCurrentTime;

	if (FMath::Abs(timeDifference) >= TimeDifferenceThreshold)
	{
		mSplineCurrentTime = mSplineDesiredTime;
	}
	else
	{
		mSplineCurrentTime = FMath::FInterpTo(mSplineCurrentTime, mSplineDesiredTime, DeltaTime, InterpSpeed);
	}

	if (mSplineCurrentTime >= 1.f)
	{
		OnCameraReachEndOfTrack.Broadcast();
		mHasReachedEnd = true;
	}		

	return CameraSpline->GetLocationAtTime(mSplineCurrentTime, ESplineCoordinateSpace::World, ConstantVelocity);
}

//************************************
// Method:    UpdateCamera
// FullName:  AVertPlayerCameraActor::UpdateCamera
// Access:    private 
// Returns:   void
// Qualifier:
//************************************
void AVertPlayerCameraActor::UpdateCamera(float DeltaTime)
{
	SetActorLocation(FMath::VInterpTo(GetActorLocation(), mTargetLocation, DeltaTime, InterpSpeed));
}

//************************************
// Method:    UpdateCamera
// FullName:  AVertPlayerCameraActor::UpdateCamera
// Access:    private 
// Returns:   void
// Qualifier:
// Parameter: float DeltaTime
// Parameter: const FVector & cameraDesiredLocation
//************************************
void AVertPlayerCameraActor::UpdateCamera(float DeltaTime, const FVector& cameraDesiredLocation)
{
	if (LookAtTarget)
	{
		FRotator newRotator = UKismetMathLibrary::FindLookAtRotation(CameraComponent->GetComponentLocation(), mTargetLocation);
		CameraComponent->SetWorldRotation(newRotator);
	}
	SetActorLocation(cameraDesiredLocation);
}