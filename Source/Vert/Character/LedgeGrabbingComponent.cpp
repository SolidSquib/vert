// Copyright Inside Out Games Ltd. 2017

#include "Vert.h"
#include "LedgeGrabbingComponent.h"

DECLARE_LOG_CATEGORY_CLASS(LogLedgeGrabbingComponent, Log, All);

// Sets default values for this component's properties
ULedgeGrabbingComponent::ULedgeGrabbingComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

// Called every frame
void ULedgeGrabbingComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (mCanTrace && (!mCharacterOwner.IsValid() || (!mCharacterOwner->GetCharacterMovement()->IsFlying() && !mCharacterOwner->GetCharacterMovement()->IsFalling())))
	{
		FHitResult hitV;
		FHitResult hitH;

		if (!mClimbingLedge && TraceForForwardLedge(hitH) && TraceForUpwardLedge(hitV))
		{
			GrabLedge(hitH.ImpactPoint, hitH.ImpactNormal, hitV.ImpactPoint);
		}
	}
}

void ULedgeGrabbingComponent::PostInitProperties()
{
	Super::PostInitProperties();

	OnComponentBeginOverlap.AddDynamic(this, &ULedgeGrabbingComponent::OnBeginOverlap);
	OnComponentEndOverlap.AddDynamic(this, &ULedgeGrabbingComponent::OnEndOverlap);

	if (ACharacter* character = Cast<ACharacter>(GetOwner()))
	{
		mCharacterOwner = character;
	} else { UE_LOG(LogLedgeGrabbingComponent, Error, TEXT("[%s] not attached to ACharacter, functionality may be dimished and crashes may occur."), *GetName()); }
}

void ULedgeGrabbingComponent::OnBeginOverlap(UPrimitiveComponent* overlappedComp, AActor* otherActor, UPrimitiveComponent* otherComp, int32 otherBodyIndex, bool fromSweep, const FHitResult& sweepResult)
{
	mCanTrace = true;
	PrimaryComponentTick.SetTickFunctionEnable(true);
}

void ULedgeGrabbingComponent::OnEndOverlap(UPrimitiveComponent* overlappedComp, AActor* otherActor, UPrimitiveComponent* otherComp, int32 otherBodyIndex)
{
	mCanTrace = false;
	PrimaryComponentTick.SetTickFunctionEnable(false);
}

bool ULedgeGrabbingComponent::PerformLedgeTrace(const FVector& start, const FVector& end, FHitResult& hit)
{
	static const FName sLedgeGrabTrace(TEXT("LedgeGrabTrace"));

	FCollisionQueryParams params(sLedgeGrabTrace, false);
	params.bReturnPhysicalMaterial = true;
	params.bTraceAsyncScene = false;
	params.bFindInitialOverlaps = false;
	params.AddIgnoredActors(ActorsToIgnore);
	params.AddIgnoredActor(GetOwner());

	UWorld* world = GetWorld();

	const bool foundHit = world->SweepSingleByChannel(hit, start, end, FQuat::Identity, TraceChannel, FCollisionShape::MakeSphere(TraceRadius), params);

#if ENABLE_DRAW_DEBUG
	if (mShowDebug)
	{
		float lifetime = 5.f;

		if (foundHit && hit.bBlockingHit)
		{
			// Red up to the blocking hit, green thereafter
			UVertUtilities::DrawDebugSweptSphere(world, start, hit.Location, TraceRadius, FColor::Red, false, lifetime);
			UVertUtilities::DrawDebugSweptSphere(world, hit.Location, end, TraceRadius, FColor::Green, false, lifetime);
			DrawDebugPoint(world, hit.ImpactPoint, 16.f, FColor::Red, false, lifetime);
		}
		else
		{
			// no hit means all red
			UVertUtilities::DrawDebugSweptSphere(world, start, end, TraceRadius, FColor::Red, false, lifetime);
		}
	}
#endif

	return foundHit;
}

bool ULedgeGrabbingComponent::TraceForForwardLedge(FHitResult& hit)
{
	FVector start = FVector(GetOwner()->GetActorLocation().X, GetOwner()->GetActorLocation().Y, 0.f); // #MI_TODO: Z setup pending
	FVector end = FVector(GetOwner()->GetActorRotation().Vector().X * ForwardRange, GetOwner()->GetActorRotation().Vector().Y * ForwardRange, 0.f); // #MI_TODO: Z setup pending

	return PerformLedgeTrace(start, end, hit);
}

bool ULedgeGrabbingComponent::TraceForUpwardLedge(FHitResult& hit)
{
	FVector start = FVector(GetOwner()->GetActorLocation().X, GetOwner()->GetActorLocation().Y, UpwardTraceZStartOffset) + (GetOwner()->GetActorRotation().Vector()*UpwardTraceForwardOffset);
	FVector end = FVector(start.X, start.Y, start.Z - UpwardRange); // #MI_TODO: Z setup pending

	return PerformLedgeTrace(start, end, hit);
}

void ULedgeGrabbingComponent::GrabLedge(const FVector& wallImpactPoint, const FVector& wallImpactNormal, const FVector& ledgeHeight)
{
	float radius = mCharacterOwner->GetCapsuleComponent()->GetScaledCapsuleRadius();
	float halfHeight = mCharacterOwner->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();

	FVector levelImpactPoint = wallImpactNormal * FVector(wallImpactPoint.X, wallImpactPoint.Y, 0.f);
	FVector targetRelativeLocation = FVector(
		levelImpactPoint.X + wallImpactPoint.X,
		levelImpactPoint.Y + wallImpactPoint.Y,
		ledgeHeight.Z - halfHeight
	);

	FRotator targetRelativeRotation = (mCharacterOwner->GetActorLocation() - wallImpactPoint).Rotation();
	mCharacterOwner->GetCapsuleComponent()->MoveComponent(targetRelativeLocation, targetRelativeRotation, false);
	mCharacterOwner->GetCharacterMovement()->StopMovementImmediately();
}

void ULedgeGrabbingComponent::DrawDebugInfo()
{

}