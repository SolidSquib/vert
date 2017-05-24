// Copyright Inside Out Games Ltd. 2017

#include "Vert.h"
#include "VertPlayerCameraActor.h"

DEFINE_LOG_CATEGORY(LogVertPlayerCamera);

// Sets default values
AVertPlayerCameraActor::AVertPlayerCameraActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

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

void AVertPlayerCameraActor::UpdateCameraPositionAndZoom(FVector& meanLocation, FVector& largestDistance, float& hLength, float& vLength)
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

		if (mZoomOverride < 0)
		{
			// Determine the final length of the largest two distances, used for camera zoom
			hLength = FMath::Sqrt(largestSquareH);
			vLength = FMath::Sqrt(largestSquareV);

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