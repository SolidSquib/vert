// Copyright Inside Out Games Ltd. 2017

#include "LedgeGrabbingComponent.h"
#include "Vert.h"

DECLARE_LOG_CATEGORY_CLASS(LogLedgeGrabbingComponent, Log, All);

// Sets default values for this component's properties
ULedgeGrabbingComponent::ULedgeGrabbingComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;

	bGenerateOverlapEvents = true;
	SetCollisionObjectType(ECC_SphereTracer);
	SetCollisionResponseToAllChannels(ECR_Ignore);
	//SetCollisionResponseToChannel(ECC_LedgeTracer, ECR_Block);
	SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Overlap);
	CanCharacterStepUpOn = ECanBeCharacterBase::ECB_No;

	TraceChannel = ECC_LedgeTracer;
}

// Called every frame
void ULedgeGrabbingComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (mLerping)
	{
		LerpToLedge(DeltaTime);
	}
	else if (Enable && mCanTrace)
	{
		FHitResult hitV;
		FHitResult hitH;

		if (TraceForUpwardLedge(hitV) && TraceForForwardLedge(hitH) && InGrabbingRange(hitV.ImpactPoint) && !mClimbingLedge)
		{
			GrabLedge(hitH, hitV);
		}
	}
}

void ULedgeGrabbingComponent::BeginPlay()
{
	if (ACharacter* character = Cast<ACharacter>(GetOwner()))
	{
		mCharacterOwner = character;
	}
	else { UE_LOG(LogLedgeGrabbingComponent, Error, TEXT("[%s] not attached to ACharacter, functionality may be dimished and crashes may occur."), *GetName()); }
}

void ULedgeGrabbingComponent::PostInitProperties()
{
	Super::PostInitProperties();

	OnComponentBeginOverlap.AddDynamic(this, &ULedgeGrabbingComponent::OnBeginOverlap);
	OnComponentEndOverlap.AddDynamic(this, &ULedgeGrabbingComponent::OnEndOverlap);
}

//************************************
// Method:    LerpToLedge
// FullName:  ULedgeGrabbingComponent::LerpToLedge
// Access:    private 
// Returns:   void
// Qualifier:
// Parameter: float deltaTime
//************************************
void ULedgeGrabbingComponent::LerpToLedge(float deltaTime)
{
	mCharacterOwner->SetActorLocation(FMath::VInterpConstantTo(mCharacterOwner->GetActorLocation(), mLerpTarget, deltaTime, 1000.f));
	FVector diff = mLerpTarget - mCharacterOwner->GetActorLocation();

	if (FMath::Abs(diff.X) < KINDA_SMALL_NUMBER && FMath::Abs(diff.Y) < KINDA_SMALL_NUMBER && FMath::Abs(diff.Z) < KINDA_SMALL_NUMBER)
	{
		mLerping = false;
		mLerpTarget = FVector::ZeroVector;
	}
}

//************************************
// Method:    OnBeginOverlap
// FullName:  ULedgeGrabbingComponent::OnBeginOverlap
// Access:    protected 
// Returns:   void
// Qualifier:
// Parameter: UPrimitiveComponent * overlappedComp
// Parameter: AActor * otherActor
// Parameter: UPrimitiveComponent * otherComp
// Parameter: int32 otherBodyIndex
// Parameter: bool fromSweep
// Parameter: const FHitResult & sweepResult
//************************************
void ULedgeGrabbingComponent::OnBeginOverlap(UPrimitiveComponent* overlappedComp, AActor* otherActor, UPrimitiveComponent* otherComp, int32 otherBodyIndex, bool fromSweep, const FHitResult& sweepResult)
{
	mNumOverlaps++;
	mCanTrace = true;
	PrimaryComponentTick.SetTickFunctionEnable(true);
}

//************************************
// Method:    OnEndOverlap
// FullName:  ULedgeGrabbingComponent::OnEndOverlap
// Access:    protected 
// Returns:   void
// Qualifier:
// Parameter: UPrimitiveComponent * overlappedComp
// Parameter: AActor * otherActor
// Parameter: UPrimitiveComponent * otherComp
// Parameter: int32 otherBodyIndex
//************************************
void ULedgeGrabbingComponent::OnEndOverlap(UPrimitiveComponent* overlappedComp, AActor* otherActor, UPrimitiveComponent* otherComp, int32 otherBodyIndex)
{
	if (--mNumOverlaps <= 0)
	{
		mCanTrace = false;
		PrimaryComponentTick.SetTickFunctionEnable(false);
	}
}

//************************************
// Method:    InGrabbingRange
// FullName:  ULedgeGrabbingComponent::InGrabbingRange
// Access:    private 
// Returns:   bool
// Qualifier:
// Parameter: const FVector & ledgeHeight
//************************************
bool ULedgeGrabbingComponent::InGrabbingRange(const FVector& ledgeHeight)
{
	if (mCharacterOwner.IsValid())
	{
		FVector hipHeight = (HipSocket != NAME_None)
			? mCharacterOwner->GetMesh()->GetSocketLocation(HipSocket)
			: mCharacterOwner->GetActorLocation();

		return UKismetMathLibrary::InRange_FloatFloat(hipHeight.Z - ledgeHeight.Z, HipHeightThreshold, 0);
	}

	return false;
}

//************************************
// Method:    PerformLedgeTrace
// FullName:  ULedgeGrabbingComponent::PerformLedgeTrace
// Access:    private 
// Returns:   bool
// Qualifier:
// Parameter: const FVector & start
// Parameter: const FVector & end
// Parameter: FHitResult & hit
//************************************
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
		if (foundHit && hit.bBlockingHit)
		{
			// Red up to the blocking hit, green thereafter
			UVertUtilities::DrawDebugSweptSphere(world, start, hit.Location, TraceRadius, FColor::Red, false);
			UVertUtilities::DrawDebugSweptSphere(world, hit.Location, end, TraceRadius, FColor::Green, false);
			DrawDebugPoint(world, hit.ImpactPoint, 16.f, FColor::Red, false);
		}
		else
		{
			// no hit means all red
			UVertUtilities::DrawDebugSweptSphere(world, start, end, TraceRadius, FColor::Red, false);
		}
	}
#endif

	return foundHit;
}

//************************************
// Method:    TraceForForwardLedge
// FullName:  ULedgeGrabbingComponent::TraceForForwardLedge
// Access:    private 
// Returns:   bool
// Qualifier:
// Parameter: FHitResult & hit
//************************************
bool ULedgeGrabbingComponent::TraceForForwardLedge(FHitResult& hit)
{
	FVector forward = FRotationMatrix(GetOwner()->GetActorRotation()).GetScaledAxis(EAxis::X);

	FVector start = GetOwner()->GetActorLocation(); // #MI_TODO: Z setup pending
	FVector end = start + (forward * FVector(ForwardRange, ForwardRange, 0.f)); // #MI_TODO: Z setup pending

	return PerformLedgeTrace(start, end, hit);
}

//************************************
// Method:    TraceForUpwardLedge
// FullName:  ULedgeGrabbingComponent::TraceForUpwardLedge
// Access:    private 
// Returns:   bool
// Qualifier:
// Parameter: FHitResult & hit
//************************************
bool ULedgeGrabbingComponent::TraceForUpwardLedge(FHitResult& hit)
{
	FVector start = FVector(GetOwner()->GetActorLocation().X, GetOwner()->GetActorLocation().Y, GetOwner()->GetActorLocation().Z + UpwardTraceZStartOffset) + (GetOwner()->GetActorRotation().Vector()*UpwardTraceForwardOffset);
	FVector end = FVector(start.X, start.Y, start.Z - UpwardRange); // #MI_TODO: Z setup pending

	return PerformLedgeTrace(start, end, hit);
}

//************************************
// Method:    GrabLedge
// FullName:  ULedgeGrabbingComponent::GrabLedge
// Access:    private 
// Returns:   void
// Qualifier:
// Parameter: const FVector & wallImpactPoint
// Parameter: const FVector & wallImpactNormal
// Parameter: const FVector & ledgeHeight
//************************************
void ULedgeGrabbingComponent::GrabLedge(const FHitResult& forwardHit, const FHitResult& downwardHit)
{
	UE_LOG(LogLedgeGrabbingComponent, Log, TEXT("%s grabbing ledge."), *mCharacterOwner->GetName());
	mLastLedgeHeight = downwardHit.ImpactPoint;
	
	float radius = mCharacterOwner->GetCapsuleComponent()->GetScaledCapsuleRadius();
	float halfHeight = mCharacterOwner->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	
	FVector playerOffset = FVector(radius, 0.f, 0.f) * forwardHit.ImpactNormal;
	mLerpTarget = playerOffset + FVector(
		downwardHit.ImpactPoint.X,
		downwardHit.ImpactPoint.Y,
		downwardHit.ImpactPoint.Z - halfHeight
	);

	FRotator targetRotation = (mCharacterOwner->GetActorLocation() - forwardHit.ImpactPoint).Rotation();
	targetRotation.Pitch = 0;
	mCharacterOwner->SetActorRotation(targetRotation);
	mCharacterOwner->GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_Flying);
	mCharacterOwner->GetCharacterMovement()->StopMovementImmediately();

	mLastGrabLedgeNormal = targetRotation.Vector().GetSafeNormal();

	OnLedgeGrabbed.Broadcast(forwardHit, downwardHit);

	mClimbingLedge = true;
	mLerping = true;
}

//************************************
// Method:    GetHipLocation
// FullName:  ULedgeGrabbingComponent::GetHipLocation
// Access:    private 
// Returns:   FVector
// Qualifier: const
//************************************
FVector ULedgeGrabbingComponent::GetHipLocation() const 
{
	return (HipSocket != NAME_None)
		? mCharacterOwner->GetMesh()->GetSocketLocation(HipSocket)
		: mCharacterOwner->GetActorLocation();
}

//************************************
// Method:    DropLedge
// FullName:  ULedgeGrabbingComponent::DropLedge
// Access:    public 
// Returns:   void
// Qualifier:
//************************************
void ULedgeGrabbingComponent::DropLedge()
{
	if (mClimbingLedge && mCharacterOwner.IsValid())
	{
		UE_LOG(LogLedgeGrabbingComponent, Log, TEXT("%s dropping ledge."), *mCharacterOwner->GetName());
		mCharacterOwner->GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_Falling);
		mClimbingLedge = false;
	}
	else {
		UE_LOG(LogLedgeGrabbingComponent, Warning, TEXT("%s can't drop ledge while not climbing, check call."), *GetName());
	}
}

//************************************
// Method:    TransitionLedge
// FullName:  ULedgeGrabbingComponent::TransitionLedge
// Access:    public 
// Returns:   void
// Qualifier:
// Parameter: ELedgeTransition transition
//************************************
void ULedgeGrabbingComponent::TransitionLedge(ELedgeTransition transition)
{
	if (!mLerping && mClimbingLedge)
	{
		OnLedgeTransition.Broadcast(transition);
		DropLedge();
	}
	else {
		if (mLerping) {
			FVector diff = mLerpTarget - mCharacterOwner->GetActorLocation();
			UE_LOG(LogLedgeGrabbingComponent, Error, TEXT("%s is still lerping, remaining distance = [%f, %f, %f]"), *GetName(), FMath::Abs(diff.X), FMath::Abs(diff.Y), FMath::Abs(diff.Z));
		}
		else {
			UE_LOG(LogLedgeGrabbingComponent, Warning, TEXT("%s transition from ledge while not climbing, check call."), *GetName());
		}	
	}
}

//************************************
// Method:    GetLedgeDirection
// FullName:  ULedgeGrabbingComponent::GetLedgeDirection
// Access:    public 
// Returns:   FVector
// Qualifier: const
// Parameter: EAimFreedom freedom
//************************************
FVector ULedgeGrabbingComponent::GetLedgeDirection(EAimFreedom freedom /* = EAimFreedom::Free */) const
{
	FVector direction = FVector::ZeroVector;

	if (mClimbingLedge)
	{
		direction = UVertUtilities::LimitAimTrajectory(freedom, mLastGrabLedgeNormal);
	}

	return direction;
}

//************************************
// Method:    DrawDebugInfo
// FullName:  ULedgeGrabbingComponent::DrawDebugInfo
// Access:    virtual protected 
// Returns:   void
// Qualifier:
//************************************
void ULedgeGrabbingComponent::DrawDebugInfo()
{

}