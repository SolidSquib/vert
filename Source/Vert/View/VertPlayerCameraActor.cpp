// Copyright Inside Out Games Ltd. 2017

#include "Vert.h"
#include "VertPlayerCameraActor.h"
#include "Kismet\KismetTextLibrary.h"

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

// Called when the game starts or when spawned
void AVertPlayerCameraActor::BeginPlay()
{
	Super::BeginPlay();	
	SetActorTickEnabled(true);

	for (TActorIterator<APawn> ActorItr(GetWorld()); ActorItr; ++ActorItr)
	{
		RegisterPlayerPawn(*ActorItr);
	}
}

// Called every frame
void AVertPlayerCameraActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	FVector meanPos, largestDistance;
	float hLength, vLength;

	UpdateCameraPositionAndZoom(meanPos, largestDistance, hLength, vLength);
	if (LockPawnsToBounds)
	{
		CorrectPawnPositions(largestDistance);
	}
}

void AVertPlayerCameraActor::UpdateCameraPositionAndZoom(FVector& meanLocation, FVector& largestDistance, float& hLengthSqr, float& vLengthSqr)
{
	if (mPawnOverride > -1)
	{
		if (mPawnsToFollow.Num() > mPawnOverride)
		{
			SetActorLocation(mPawnsToFollow[mPawnOverride]->GetActorLocation());
			CameraBoom->TargetArmLength = (mZoomOverride > -1) ? mZoomOverride : 1000.f;
		} else { UE_LOG(LogVertPlayerCamera, Error, TEXT("Overriden pawn does not exist in array.")); }
	}
	else
	{
		meanLocation = FVector::ZeroVector;
		largestDistance = FVector::ZeroVector;
		float largestSquareH = 0;
		float largestSquareV = 0;

		for (APawn* pawn : mPawnsToFollow)
		{
			// Add the new pawn's location to the current value
			meanLocation += pawn->GetActorLocation();
			FVector distance = GetActorLocation() - pawn->GetActorLocation();
			FVector horizontal = FVector::CrossProduct(distance, CameraComponent->GetUpVector());
			FVector vertical = FVector::CrossProduct(distance, CameraComponent->GetRightVector()); 

			if (horizontal.SizeSquared() > largestSquareH)
			{
				largestSquareH = horizontal.SizeSquared();
				largestDistance = horizontal;
			}
			largestSquareV = FMath::Max(vertical.SizeSquared(), largestSquareV);
		}
		// determine the average location and set this actors position to that (in the center of all pawns)
		meanLocation /= mPawnsToFollow.Num();
		SetActorLocation(meanLocation);

		
	}	
}

void AVertPlayerCameraActor::UpdateCameraZoom(float hLengthSqr, float vLengthSqr)
{
	float hLength, vLength;

	if (mZoomOverride < 0)
	{
		// Determine the final length of the largest two distances, used for camera zoom
		hLength = FMath::Sqrt(hLengthSqr);
		vLength = FMath::Sqrt(vLengthSqr);

		float targetH = hLength * 2;
		float targetV = vLength * 3.5f;

		targetH = FMath::Clamp(targetH, MinArmLength, MaxArmLength);
		targetV = FMath::Clamp(targetV, MinArmLength, MaxArmLength);

		CameraBoom->TargetArmLength = FMath::Max(targetH, targetV);
	}
	else
	{
		CameraBoom->TargetArmLength = mZoomOverride;
	}
}

void AVertPlayerCameraActor::CorrectPawnPositions(const FVector& largestDistance)
{
	for (APawn* pawn : mPawnsToFollow)
	{
		FVector clamped = largestDistance.GetClampedToMaxSize(MaximumDistance);
		FVector targetLocation = (FVector::DotProduct(pawn->GetActorLocation(), CameraComponent->GetRightVector()) > SMALL_NUMBER)
			? GetActorLocation() + (clamped*0.5f)
			: GetActorLocation() - (clamped*0.5f);
		pawn->SetActorLocation(targetLocation);
	}
}

void AVertPlayerCameraActor::RegisterPlayerPawn(APawn* pawnToFollow)
{
	if (mPawnsToFollow.Find(pawnToFollow) == INDEX_NONE)
	{
		mPawnsToFollow.Add(pawnToFollow);
		UE_LOG(LogVertPlayerCamera, Warning, TEXT("Pawn added to follow list with name [%s]"), *pawnToFollow->GetName());

		if (AVertPlayerController* controller = Cast<AVertPlayerController>(pawnToFollow->GetController()))
		{
			mPlayerControllers.Add(controller);
			controller->SetViewTargetWithBlend(this);
		}
	}
}

void AVertPlayerCameraActor::UnregisterPlayerPawn(APawn* pawnToFollow)
{
	if (mPawnsToFollow.Find(pawnToFollow) != INDEX_NONE)
	{
		mPawnsToFollow.Remove(pawnToFollow);
		UE_LOG(LogVertPlayerCamera, Warning, TEXT("Pawn removed from follow list with name [%s]"), *pawnToFollow->GetName());

		if (AVertPlayerController* controller = Cast<AVertPlayerController>(pawnToFollow->GetController()))
		{
			mPlayerControllers.Remove(controller);
		}
	}
}

void AVertPlayerCameraActor::OverridePawnFollow(int32 pawnIndex)
{
	mPawnOverride = pawnIndex;
}

void AVertPlayerCameraActor::OverrideCameraZoom(int32 cameraZoomAmount)
{
	mZoomOverride = cameraZoomAmount;
}

void AVertPlayerCameraActor::ScrubCameraTime()
{
	FVector cameraLocation = CameraSpline->GetLocationAtTime(CameraDebugTime, ESplineCoordinateSpace::World, ConstantVelocity);
	FVector playerLocation = PlayerSpline->GetLocationAtTime(CameraDebugTime, ESplineCoordinateSpace::World, ConstantVelocity);

	FRotator targetLookAt = UKismetMathLibrary::FindLookAtRotation(cameraLocation, playerLocation);

	CameraComponent->SetWorldLocationAndRotation(cameraLocation, FRotator(targetLookAt.Pitch, targetLookAt.Yaw, 0.f));
}

void AVertPlayerCameraActor::SetupDebugNumbers()
{
	if (ShowDebugInfo)
	{
		AddSplineNumbers(PlayerSpline);
		AddSplineNumbers(CameraSpline);
	}
}

void AVertPlayerCameraActor::AddSplineNumbers(USplineComponent* spline)
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

		if (UObject* world = GetWorld())
		{
			UKismetSystemLibrary::DrawDebugSphere(world, playerCurrentLocation, 50.f, 12, FLinearColor::Red);
			UKismetSystemLibrary::DrawDebugSphere(world, playerDesiredLocation, 50.f, 12, FLinearColor::Red);
			UKismetSystemLibrary::DrawDebugSphere(world, cameraCurrentLocation, 50.f, 12, FLinearColor::Blue);
			UKismetSystemLibrary::DrawDebugSphere(world, cameraDesiredLocation, 50.f, 12, FLinearColor::Blue);

			UKismetSystemLibrary::DrawDebugArrow(world, playerDesiredLocation, playerDesiredLocation + (playerSplineForward*50.f), 12.f, FLinearColor::Green, 0.f, 3.f);
			UKismetSystemLibrary::DrawDebugArrow(world, playerDesiredLocation, playerDesiredLocation + (playerSplineRight*50.f), 12.f, FLinearColor::Yellow, 0.f, 3.f);
		}
	}
}

float AVertPlayerCameraActor::RecursiveDistanceCheck(int iterations, float startTime, float endTime)
{
	float startDistance = (PlayerSpline->GetLocationAtTime(startTime, ESplineCoordinateSpace::World, ConstantVelocity) - mTarget->GetActorLocation()).Size();
	float endDistance = (PlayerSpline->GetLocationAtTime(endTime, ESplineCoordinateSpace::World, ConstantVelocity) - mTarget->GetActorLocation()).Size();
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

FVector AVertPlayerCameraActor::UpdateDesiredTime(float DeltaTime)
{
	float timeDifference = mSplineDesiredTime - mSplineCurrentTime;

	if (UKismetMathLibrary::Abs(timeDifference) >= TimeDifferenceThreshold)
	{
		mSplineCurrentTime = mSplineDesiredTime;
	}
	else
	{
		mSplineCurrentTime = (timeDifference * DeltaTime) + mSplineCurrentTime;
	}

	return CameraSpline->GetLocationAtTime(mSplineDesiredTime, ESplineCoordinateSpace::World, ConstantVelocity);
}

void AVertPlayerCameraActor::UpdateCamera(float DeltaTime, const FVector& cameraDesiredLocation)
{
	if (mTarget.IsValid())
	{
		FVector newLocation = UKismetMathLibrary::VInterpTo(SplineTrackingCamera->GetComponentLocation(), cameraDesiredLocation, DeltaTime, CameraInterpSpeed);
		FRotator newRotator = UKismetMathLibrary::FindLookAtRotation(SplineTrackingCamera->GetComponentLocation(), mTarget->GetActorLocation());

		SplineTrackingCamera->SetWorldLocationAndRotation(newLocation, newRotator);
	}
}