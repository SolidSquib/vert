// Fill out your copyright notice in the Description page of Project Settings.

#include "Vert.h"
#include "VertUtilities.h"

//************************************
// Method:    LimitAimTrajectory
// FullName:  UVertUtilities::LimitAimTrajectory
// Access:    public static 
// Returns:   FVector
// Qualifier:
// Parameter: EAimFreedom mode
// Parameter: const FVector & vector
//************************************
FVector UVertUtilities::LimitAimTrajectory(EAimFreedom mode, const FVector& vector)
{
	FVector fixedVector = vector;

	switch (mode)
	{
	case EAimFreedom::FortyFive:
		fixedVector = SnapVectorToAngle(vector, 45.0f);
		break;

	case EAimFreedom::Ninety:
		fixedVector = SnapVectorToAngle(vector, 90.0f);
		break;

	case EAimFreedom::Horizontal:
		fixedVector = SnapVectorToAngle(vector, 180.0f);
		break;
	}

	return fixedVector;
}

//************************************
// Method:    LimitAimTrajectory2D
// FullName:  UVertUtilities::LimitAimTrajectory2D
// Access:    public static 
// Returns:   FVector2D
// Qualifier:
// Parameter: EAimFreedom mode
// Parameter: const FVector2D & vector
//************************************
FVector2D UVertUtilities::LimitAimTrajectory2D(EAimFreedom mode, const FVector2D& vector)
{
	FVector2D fixedVector = vector;

	switch (mode)
	{
	case EAimFreedom::FortyFive:
		fixedVector = SnapVector2DToAngle(vector, 45.0f);
		break;

	case EAimFreedom::Ninety:
		fixedVector = SnapVector2DToAngle(vector, 90.0f);
		break;

	case EAimFreedom::Horizontal:
		fixedVector = SnapVector2DToAngle(vector, 180.0f);
		break;
	}

	return fixedVector;
}

//************************************
// Method:    SnapVectorToAngle
// FullName:  UVertUtilities::SnapVectorToAngle
// Access:    public static 
// Returns:   FVector
// Qualifier:
// Parameter: const FVector & vector
// Parameter: float degrees
//************************************
FVector UVertUtilities::SnapVectorToAngle(const FVector& vector, float degrees)
{
	float roundAngle = FMath::DegreesToRadians(degrees);
	float angle = FMath::Atan2(vector.Z, vector.X);
	FVector newVector;

	if (FMath::Fmod(angle, roundAngle) != 0)
	{
		float newAngle = FMath::RoundToInt(angle / roundAngle) * roundAngle;
		newVector = FVector(FMath::Cos(newAngle), 0.f, FMath::Sin(newAngle));
	}
	else
		newVector = vector;

	return (newVector * 100).GetSafeNormal();
}

//************************************
// Method:    SnapVector2DToAngle
// FullName:  UVertUtilities::SnapVector2DToAngle
// Access:    public static 
// Returns:   FVector2D
// Qualifier:
// Parameter: const FVector2D & vector
// Parameter: float degrees
//************************************
FVector2D UVertUtilities::SnapVector2DToAngle(const FVector2D& vector, float degrees)
{
	FVector vector3D(vector.X, 0.f, vector.Y);
	vector3D = SnapVectorToAngle(vector3D, degrees);
	FVector2D newVector(vector3D.X, vector3D.Z);

	return (newVector * 100).GetSafeNormal();
}

//************************************
// Method:    SphereTraceSingleByChannel
// FullName:  UVertUtilities::SphereTraceSingleByChannel
// Access:    public static 
// Returns:   bool
// Qualifier:
// Parameter: const FVector & start
// Parameter: const FVector & end
// Parameter: const float radius
// Parameter: FHitResult & hitOut
// Parameter: const FCollisionQueryParams & params
// Parameter: ECollisionChannel traceChannel
//************************************
bool UVertUtilities::SphereTraceSingleByChannel(const FVector& start, const FVector& end, const float radius, FHitResult& hitOut, const FCollisionQueryParams& params, ECollisionChannel traceChannel /* = ECC_Pawn */)
{
	TObjectIterator<APlayerController> thePC;
	if (!thePC)
		return false;

	if (UWorld* world = thePC->GetWorld())
	{
		return world->SweepSingleByChannel(hitOut, start, end, FQuat(), traceChannel, FCollisionShape::MakeSphere(radius), params);
	}

	return false;
}

//************************************
// Method:    SphereTraceSingleByObjectTypes
// FullName:  UVertUtilities::SphereTraceSingleByObjectTypes
// Access:    public static 
// Returns:   bool
// Qualifier:
// Parameter: const FVector & start
// Parameter: const FVector & end
// Parameter: const float radius
// Parameter: FHitResult & hitOut
// Parameter: const FCollisionQueryParams & params
// Parameter: const FCollisionObjectQueryParams & objectTypes
//************************************
bool UVertUtilities::SphereTraceSingleByObjectTypes(const FVector& start, const FVector& end, const float radius, FHitResult& hitOut, const FCollisionQueryParams& params, const FCollisionObjectQueryParams& objectTypes)
{
	TObjectIterator<APlayerController> thePC;
	if (!thePC)
		return false;

	if (UWorld* world = thePC->GetWorld())
	{
		return world->SweepSingleByObjectType(hitOut, start, end, FQuat(), objectTypes, FCollisionShape::MakeSphere(radius), params);
	}

	return false;
}

//************************************
// Method:    SphereTraceMultiByChannel
// FullName:  UVertUtilities::SphereTraceMultiByChannel
// Access:    public static 
// Returns:   bool
// Qualifier:
// Parameter: const FVector & start
// Parameter: const FVector & end
// Parameter: const float radius
// Parameter: TArray<FHitResult> & hitOut
// Parameter: const FCollisionQueryParams & params
// Parameter: ECollisionChannel traceChannel
//************************************
bool UVertUtilities::SphereTraceMultiByChannel(const FVector& start, const FVector& end, const float radius, TArray<FHitResult>& hitOut, const FCollisionQueryParams& params, ECollisionChannel traceChannel /* = ECC_Pawn */)
{
	TObjectIterator<APlayerController> thePC;
	if (!thePC)
		return false;

	if (UWorld* world = thePC->GetWorld())
	{
		return world->SweepMultiByChannel(hitOut, start, end, FQuat(), traceChannel, FCollisionShape::MakeSphere(radius), params);
	}

	return false;
}

//************************************
// Method:    SphereTraceMultiByObjectTypes
// FullName:  UVertUtilities::SphereTraceMultiByObjectTypes
// Access:    public static 
// Returns:   bool
// Qualifier:
// Parameter: const FVector & start
// Parameter: const FVector & end
// Parameter: const float radius
// Parameter: TArray<FHitResult> & hitOut
// Parameter: const FCollisionQueryParams & params
// Parameter: const FCollisionObjectQueryParams & objectTypes
//************************************
bool UVertUtilities::SphereTraceMultiByObjectTypes(const FVector& start, const FVector& end, const float radius, TArray<FHitResult>& hitOut, const FCollisionQueryParams& params, const FCollisionObjectQueryParams& objectTypes)
{
	TObjectIterator<APlayerController> thePC;
	if (!thePC)
		return false;

	if (UWorld* world = thePC->GetWorld())
	{
		return world->SweepMultiByObjectType(hitOut, start, end, FQuat(), objectTypes, FCollisionShape::MakeSphere(radius), params);
	}

	return false;
}

//************************************
// Method:    DrawDebugSweptSphere
// FullName:  UVertUtilities::DrawDebugSweptSphere
// Access:    public static 
// Returns:   void
// Qualifier:
// Parameter: const UWorld * InWorld
// Parameter: FVector const & Start
// Parameter: FVector const & End
// Parameter: float Radius
// Parameter: FColor const & Color
// Parameter: bool bPersistentLines
// Parameter: float LifeTime
// Parameter: uint8 DepthPriority
//************************************
void UVertUtilities::DrawDebugSweptSphere(const UWorld* InWorld, FVector const& Start, FVector const& End, float Radius, FColor const& Color, bool bPersistentLines /*= false*/, float LifeTime /*= -1.f*/, uint8 DepthPriority /*= 0*/)
{
#if ENABLE_DRAW_DEBUG
	FVector const TraceVec = End - Start;
	float const Dist = TraceVec.Size();

	FVector const Center = Start + TraceVec * 0.5f;
	float const HalfHeight = (Dist * 0.5f) + Radius;

	FQuat const CapsuleRot = FRotationMatrix::MakeFromZ(TraceVec).ToQuat();
	::DrawDebugCapsule(InWorld, Center, HalfHeight, Radius, CapsuleRot, Color, bPersistentLines, LifeTime, DepthPriority);
#endif
}

//************************************
// Method:    IsActorInFrustum
// FullName:  UVertUtilities::IsActorInFrustum
// Access:    public static 
// Returns:   bool
// Qualifier:
// Parameter: AActor * Actor
//************************************
bool UVertUtilities::IsActorInFrustum(const UWorld* world, AActor* actor)
{
	if (world)
	{
		ULocalPlayer* LocalPlayer = world->GetFirstLocalPlayerFromController();
		if (LocalPlayer != nullptr && LocalPlayer->ViewportClient != nullptr && LocalPlayer->ViewportClient->Viewport)
		{
			FSceneViewFamilyContext ViewFamily(FSceneViewFamily::ConstructionValues(
				LocalPlayer->ViewportClient->Viewport,
				world->Scene,
				LocalPlayer->ViewportClient->EngineShowFlags)
				.SetRealtimeUpdate(true));

			FVector ViewLocation;
			FRotator ViewRotation;
			FSceneView* SceneView = LocalPlayer->CalcSceneView(&ViewFamily, ViewLocation, ViewRotation, LocalPlayer->ViewportClient->Viewport);
			if (SceneView != nullptr)
			{
				return SceneView->ViewFrustum.IntersectSphere(
					actor->GetActorLocation(), actor->GetSimpleCollisionRadius());
			}
		}
	}	

	return false;
}