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

	// Create a camera boom attached to the root (capsule)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 1000.0f;
	CameraBoom->SocketOffset = FVector(0.0f, 0.0f, 75.0f);
	CameraBoom->bAbsoluteRotation = true;
	CameraBoom->bDoCollisionTest = false;
	CameraBoom->RelativeRotation = FRotator(0.0f, -90.0f, 0.0f);

	// Create an orthographic camera (no perspective) and attach it to the boom
	SideViewCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("SideViewCamera"));
	SideViewCameraComponent->ProjectionMode = ECameraProjectionMode::Perspective;
	SideViewCameraComponent->OrthoWidth = 2048.0f;
	SideViewCameraComponent->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);

	// Prevent all automatic rotation behavior on the camera, character, and camera component
	CameraBoom->bAbsoluteRotation = true;

	InterpSpeed = 100.f;
}

// Called when the game starts or when spawned
void AVertPlayerCameraActor::BeginPlay()
{
	Super::BeginPlay();	
	SetActorTickEnabled(true);
}

// Called every frame
void AVertPlayerCameraActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	FVector targetLocation = FVector::ZeroVector;
	FVector sumOfVectors = FVector::ZeroVector;

	for (int32 i = 0; i < mPawnsToFollow.Num(); ++i)
	{
		sumOfVectors += mPawnsToFollow[i]->GetActorLocation();
	}

	if(mPawnsToFollow.Num() > 0)
		targetLocation = sumOfVectors / mPawnsToFollow.Num();

	SetActorLocation(FMath::VInterpTo(GetActorLocation(), targetLocation, DeltaTime, InterpSpeed));
}

void AVertPlayerCameraActor::RegisterPlayerPawn(APawn* pawnToFollow)
{
	if (mPawnsToFollow.Find(pawnToFollow) == INDEX_NONE)
	{
		mPawnsToFollow.Add(pawnToFollow);
		UE_LOG(LogVertPlayerCamera, Warning, TEXT("Pawn added to follow list with name [%s]"), *pawnToFollow->GetName());
	}
}

void AVertPlayerCameraActor::UnregisterPlayerPawn(APawn* pawnToFollow)
{
	if (mPawnsToFollow.Find(pawnToFollow) != INDEX_NONE)
	{
		mPawnsToFollow.Remove(pawnToFollow);
		UE_LOG(LogVertPlayerCamera, Warning, TEXT("Pawn removed from follow list with name [%s]"), *pawnToFollow->GetName());
	}
}