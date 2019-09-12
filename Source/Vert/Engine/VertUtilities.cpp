// Fill out your copyright notice in the Description page of Project Settings.

#include "VertUtilities.h"
#include "VertGlobals.h"
#include "Particles/ParticleSystemComponent.h"

DECLARE_LOG_CATEGORY_CLASS(LogVertUtilities, Log, All);

static const float TRACE_DEBUG_IMPACTPOINT_SIZE = 16.f;

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

#if ENABLE_DRAW_DEBUG
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
	FVector const TraceVec = End - Start;
	float const Dist = TraceVec.Size();

	FVector const Center = Start + TraceVec * 0.5f;
	float const HalfHeight = (Dist * 0.5f) + Radius;

	FQuat const CapsuleRot = FRotationMatrix::MakeFromZ(TraceVec).ToQuat();
	::DrawDebugCapsule(InWorld, Center, HalfHeight, Radius, CapsuleRot, Color, bPersistentLines, LifeTime, DepthPriority);
}

//************************************
// Method:    DrawDebugSweptBox
// FullName:  UVertUtilities::DrawDebugSweptBox
// Access:    public static 
// Returns:   void
// Qualifier:
// Parameter: const UWorld * InWorld
// Parameter: FVector const & Start
// Parameter: FVector const & End
// Parameter: FRotator const & Orientation
// Parameter: FVector const & HalfSize
// Parameter: FColor const & Color
// Parameter: bool bPersistentLines
// Parameter: float LifeTime
// Parameter: uint8 DepthPriority
//************************************
void UVertUtilities::DrawDebugSweptBox(const UWorld* InWorld, FVector const& Start, FVector const& End, FRotator const & Orientation, FVector const & HalfSize, FColor const& Color, bool bPersistentLines/* = false*/, float LifeTime/* = -1.f*/, uint8 DepthPriority/* = 0*/)
{
	FVector const TraceVec = End - Start;
	float const Dist = TraceVec.Size();

	FVector const Center = Start + TraceVec * 0.5f;

	FQuat const CapsuleRot = Orientation.Quaternion();
	::DrawDebugBox(InWorld, Start, HalfSize, CapsuleRot, Color, bPersistentLines, LifeTime, DepthPriority);

	//now draw lines from vertices
	FVector Vertices[8];
	Vertices[0] = Start + CapsuleRot.RotateVector(FVector(-HalfSize.X, -HalfSize.Y, -HalfSize.Z));	//flt
	Vertices[1] = Start + CapsuleRot.RotateVector(FVector(-HalfSize.X, HalfSize.Y, -HalfSize.Z));	//frt
	Vertices[2] = Start + CapsuleRot.RotateVector(FVector(-HalfSize.X, -HalfSize.Y, HalfSize.Z));	//flb
	Vertices[3] = Start + CapsuleRot.RotateVector(FVector(-HalfSize.X, HalfSize.Y, HalfSize.Z));	//frb
	Vertices[4] = Start + CapsuleRot.RotateVector(FVector(HalfSize.X, -HalfSize.Y, -HalfSize.Z));	//blt
	Vertices[5] = Start + CapsuleRot.RotateVector(FVector(HalfSize.X, HalfSize.Y, -HalfSize.Z));	//brt
	Vertices[6] = Start + CapsuleRot.RotateVector(FVector(HalfSize.X, -HalfSize.Y, HalfSize.Z));	//blb
	Vertices[7] = Start + CapsuleRot.RotateVector(FVector(HalfSize.X, HalfSize.Y, HalfSize.Z));		//brb
	for (int32 VertexIdx = 0; VertexIdx < 8; ++VertexIdx)
	{
		::DrawDebugLine(InWorld, Vertices[VertexIdx], Vertices[VertexIdx] + TraceVec, Color, bPersistentLines, LifeTime, DepthPriority);
	}

	::DrawDebugBox(InWorld, End, HalfSize, CapsuleRot, Color, bPersistentLines, LifeTime, DepthPriority);
}

//************************************
// Method:    DrawDebugLineTraceSingle
// FullName:  UVertUtilities::DrawDebugLineTraceSingle
// Access:    public static 
// Returns:   void
// Qualifier:
// Parameter: const UWorld * World
// Parameter: const FVector & Start
// Parameter: const FVector & End
// Parameter: EDrawDebugTrace::Type DrawDebugType
// Parameter: bool bHit
// Parameter: FHitResult & OutHit
// Parameter: FLinearColor TraceColor
// Parameter: FLinearColor TraceHitColor
// Parameter: float DrawTime
//************************************
void UVertUtilities::DrawDebugLineTraceSingle(const UWorld* World, const FVector& Start, const FVector& End, EDrawDebugTrace::Type DrawDebugType, bool bHit, FHitResult& OutHit, FLinearColor TraceColor, FLinearColor TraceHitColor, float DrawTime)
{
	if (DrawDebugType != EDrawDebugTrace::None)
	{
		bool bPersistent = DrawDebugType == EDrawDebugTrace::Persistent;
		float LifeTime = (DrawDebugType == EDrawDebugTrace::ForDuration) ? DrawTime : 0.f;

		// @fixme, draw line with thickness = 2.f?
		if (bHit && OutHit.bBlockingHit)
		{
			// Red up to the blocking hit, green thereafter
			::DrawDebugLine(World, Start, OutHit.ImpactPoint, TraceColor.ToFColor(true), bPersistent, LifeTime);
			::DrawDebugLine(World, OutHit.ImpactPoint, End, TraceHitColor.ToFColor(true), bPersistent, LifeTime);
			::DrawDebugPoint(World, OutHit.ImpactPoint, TRACE_DEBUG_IMPACTPOINT_SIZE, TraceColor.ToFColor(true), bPersistent, LifeTime);
		}
		else
		{
			// no hit means all red
			::DrawDebugLine(World, Start, End, TraceColor.ToFColor(true), bPersistent, LifeTime);
		}
	}
}

//************************************
// Method:    DrawDebugLineTraceMulti
// FullName:  UVertUtilities::DrawDebugLineTraceMulti
// Access:    public static 
// Returns:   void
// Qualifier:
// Parameter: const UWorld * World
// Parameter: const FVector & Start
// Parameter: const FVector & End
// Parameter: EDrawDebugTrace::Type DrawDebugType
// Parameter: bool bHit
// Parameter: const TArray<FHitResult> & OutHits
// Parameter: FLinearColor TraceColor
// Parameter: FLinearColor TraceHitColor
// Parameter: float DrawTime
//************************************
void UVertUtilities::DrawDebugLineTraceMulti(const UWorld* World, const FVector& Start, const FVector& End, EDrawDebugTrace::Type DrawDebugType, bool bHit, const TArray<FHitResult>& OutHits, FLinearColor TraceColor, FLinearColor TraceHitColor, float DrawTime)
{
	if (DrawDebugType != EDrawDebugTrace::None)
	{
		bool bPersistent = DrawDebugType == EDrawDebugTrace::Persistent;
		float LifeTime = (DrawDebugType == EDrawDebugTrace::ForDuration) ? DrawTime : 0.f;

		// @fixme, draw line with thickness = 2.f?
		if (bHit && OutHits.Last().bBlockingHit)
		{
			// Red up to the blocking hit, green thereafter
			FVector const BlockingHitPoint = OutHits.Last().ImpactPoint;
			::DrawDebugLine(World, Start, BlockingHitPoint, TraceColor.ToFColor(true), bPersistent, LifeTime);
			::DrawDebugLine(World, BlockingHitPoint, End, TraceHitColor.ToFColor(true), bPersistent, LifeTime);
		}
		else
		{
			// no hit means all red
			::DrawDebugLine(World, Start, End, TraceColor.ToFColor(true), bPersistent, LifeTime);
		}

		// draw hits
		for (int32 HitIdx = 0; HitIdx < OutHits.Num(); ++HitIdx)
		{
			FHitResult const& Hit = OutHits[HitIdx];
			::DrawDebugPoint(World, Hit.ImpactPoint, TRACE_DEBUG_IMPACTPOINT_SIZE, (Hit.bBlockingHit ? TraceColor.ToFColor(true) : TraceHitColor.ToFColor(true)), bPersistent, LifeTime);
		}
	}
}

//************************************
// Method:    DrawDebugBoxTraceSingle
// FullName:  UVertUtilities::DrawDebugBoxTraceSingle
// Access:    public static 
// Returns:   void
// Qualifier:
// Parameter: const UWorld * World
// Parameter: const FVector & Start
// Parameter: const FVector & End
// Parameter: const FVector HalfSize
// Parameter: const FRotator Orientation
// Parameter: EDrawDebugTrace::Type DrawDebugType
// Parameter: bool bHit
// Parameter: FHitResult & OutHit
// Parameter: FLinearColor TraceColor
// Parameter: FLinearColor TraceHitColor
// Parameter: float DrawTime
//************************************
void UVertUtilities::DrawDebugBoxTraceSingle(const UWorld* World, const FVector& Start, const FVector& End, const FVector HalfSize, const FRotator Orientation, EDrawDebugTrace::Type DrawDebugType, bool bHit, FHitResult& OutHit, FLinearColor TraceColor, FLinearColor TraceHitColor, float DrawTime)
{
	if (DrawDebugType != EDrawDebugTrace::None && (World != nullptr))
	{
		bool bPersistent = DrawDebugType == EDrawDebugTrace::Persistent;
		float LifeTime = (DrawDebugType == EDrawDebugTrace::ForDuration) ? DrawTime : 0.f;

		if (bHit && OutHit.bBlockingHit)
		{
			// Red up to the blocking hit, green thereafter
			DrawDebugSweptBox(World, Start, OutHit.Location, Orientation, HalfSize, TraceColor.ToFColor(true), bPersistent, LifeTime);
			DrawDebugSweptBox(World, OutHit.Location, End, Orientation, HalfSize, TraceHitColor.ToFColor(true), bPersistent, LifeTime);
			::DrawDebugPoint(World, OutHit.ImpactPoint, TRACE_DEBUG_IMPACTPOINT_SIZE, TraceColor.ToFColor(true), bPersistent, LifeTime);
		}
		else
		{
			// no hit means all red
			DrawDebugSweptBox(World, Start, End, Orientation, HalfSize, TraceColor.ToFColor(true), bPersistent, LifeTime);
		}
	}
}

//************************************
// Method:    DrawDebugBoxTraceMulti
// FullName:  UVertUtilities::DrawDebugBoxTraceMulti
// Access:    public static 
// Returns:   void
// Qualifier:
// Parameter: const UWorld * World
// Parameter: const FVector & Start
// Parameter: const FVector & End
// Parameter: const FVector HalfSize
// Parameter: const FRotator Orientation
// Parameter: EDrawDebugTrace::Type DrawDebugType
// Parameter: bool bHit
// Parameter: TArray<FHitResult> & OutHits
// Parameter: FLinearColor TraceColor
// Parameter: FLinearColor TraceHitColor
// Parameter: float DrawTime
//************************************
void UVertUtilities::DrawDebugBoxTraceMulti(const UWorld* World, const FVector& Start, const FVector& End, const FVector HalfSize, const FRotator Orientation, EDrawDebugTrace::Type DrawDebugType, bool bHit, TArray<FHitResult>& OutHits, FLinearColor TraceColor, FLinearColor TraceHitColor, float DrawTime)
{
	if (DrawDebugType != EDrawDebugTrace::None && (World != nullptr))
	{
		bool bPersistent = DrawDebugType == EDrawDebugTrace::Persistent;
		float LifeTime = (DrawDebugType == EDrawDebugTrace::ForDuration) ? DrawTime : 0.f;

		if (bHit && OutHits.Last().bBlockingHit)
		{
			// Red up to the blocking hit, green thereafter
			FVector const BlockingHitPoint = OutHits.Last().Location;
			DrawDebugSweptBox(World, Start, BlockingHitPoint, Orientation, HalfSize, TraceColor.ToFColor(true), bPersistent, LifeTime);
			DrawDebugSweptBox(World, BlockingHitPoint, End, Orientation, HalfSize, TraceHitColor.ToFColor(true), bPersistent, LifeTime);
		}
		else
		{
			// no hit means all red
			DrawDebugSweptBox(World, Start, End, Orientation, HalfSize, TraceColor.ToFColor(true), bPersistent, LifeTime);
		}

		// draw hits
		for (int32 HitIdx = 0; HitIdx < OutHits.Num(); ++HitIdx)
		{
			FHitResult const& Hit = OutHits[HitIdx];
			::DrawDebugPoint(World, Hit.ImpactPoint, TRACE_DEBUG_IMPACTPOINT_SIZE, (Hit.bBlockingHit ? TraceColor.ToFColor(true) : TraceHitColor.ToFColor(true)), bPersistent, LifeTime);
		}
	}
}

//************************************
// Method:    DrawDebugSphereTraceSingle
// FullName:  UVertUtilities::DrawDebugSphereTraceSingle
// Access:    public static 
// Returns:   void
// Qualifier:
// Parameter: const UWorld * World
// Parameter: const FVector & Start
// Parameter: const FVector & End
// Parameter: float Radius
// Parameter: EDrawDebugTrace::Type DrawDebugType
// Parameter: bool bHit
// Parameter: FHitResult & OutHit
// Parameter: FLinearColor TraceColor
// Parameter: FLinearColor TraceHitColor
// Parameter: float DrawTime
//************************************
void UVertUtilities::DrawDebugSphereTraceSingle(const UWorld* World, const FVector& Start, const FVector& End, float Radius, EDrawDebugTrace::Type DrawDebugType, bool bHit, FHitResult& OutHit, FLinearColor TraceColor, FLinearColor TraceHitColor, float DrawTime)
{
	if (DrawDebugType != EDrawDebugTrace::None)
	{
		bool bPersistent = DrawDebugType == EDrawDebugTrace::Persistent;
		float LifeTime = (DrawDebugType == EDrawDebugTrace::ForDuration) ? DrawTime : 0.f;

		if (bHit && OutHit.bBlockingHit)
		{
			// Red up to the blocking hit, green thereafter
			DrawDebugSweptSphere(World, Start, OutHit.Location, Radius, TraceColor.ToFColor(true), bPersistent, LifeTime);
			DrawDebugSweptSphere(World, OutHit.Location, End, Radius, TraceHitColor.ToFColor(true), bPersistent, LifeTime);
			::DrawDebugPoint(World, OutHit.ImpactPoint, TRACE_DEBUG_IMPACTPOINT_SIZE, TraceColor.ToFColor(true), bPersistent, LifeTime);
		}
		else
		{
			// no hit means all red
			DrawDebugSweptSphere(World, Start, End, Radius, TraceColor.ToFColor(true), bPersistent, LifeTime);
		}
	}
}

//************************************
// Method:    DrawDebugSphereTraceMulti
// FullName:  UVertUtilities::DrawDebugSphereTraceMulti
// Access:    public static 
// Returns:   void
// Qualifier:
// Parameter: const UWorld * World
// Parameter: const FVector & Start
// Parameter: const FVector & End
// Parameter: float Radius
// Parameter: EDrawDebugTrace::Type DrawDebugType
// Parameter: bool bHit
// Parameter: TArray<FHitResult> & OutHits
// Parameter: FLinearColor TraceColor
// Parameter: FLinearColor TraceHitColor
// Parameter: float DrawTime
//************************************
void UVertUtilities::DrawDebugSphereTraceMulti(const UWorld* World, const FVector& Start, const FVector& End, float Radius, EDrawDebugTrace::Type DrawDebugType, bool bHit, TArray<FHitResult>& OutHits, FLinearColor TraceColor, FLinearColor TraceHitColor, float DrawTime)
{
	if (DrawDebugType != EDrawDebugTrace::None)
	{
		bool bPersistent = DrawDebugType == EDrawDebugTrace::Persistent;
		float LifeTime = (DrawDebugType == EDrawDebugTrace::ForDuration) ? DrawTime : 0.f;

		if (bHit && OutHits.Last().bBlockingHit)
		{
			// Red up to the blocking hit, green thereafter
			FVector const BlockingHitPoint = OutHits.Last().Location;
			DrawDebugSweptSphere(World, Start, BlockingHitPoint, Radius, TraceColor.ToFColor(true), bPersistent, LifeTime);
			DrawDebugSweptSphere(World, BlockingHitPoint, End, Radius, TraceHitColor.ToFColor(true), bPersistent, LifeTime);
		}
		else
		{
			// no hit means all red
			DrawDebugSweptSphere(World, Start, End, Radius, TraceColor.ToFColor(true), bPersistent, LifeTime);
		}

		// draw hits
		for (int32 HitIdx = 0; HitIdx < OutHits.Num(); ++HitIdx)
		{
			FHitResult const& Hit = OutHits[HitIdx];
			::DrawDebugPoint(World, Hit.ImpactPoint, TRACE_DEBUG_IMPACTPOINT_SIZE, (Hit.bBlockingHit ? TraceColor.ToFColor(true) : TraceHitColor.ToFColor(true)), bPersistent, LifeTime);
		}
	}
}

//************************************
// Method:    DrawDebugCapsuleTraceSingle
// FullName:  UVertUtilities::DrawDebugCapsuleTraceSingle
// Access:    public static 
// Returns:   void
// Qualifier:
// Parameter: const UWorld * World
// Parameter: const FVector & Start
// Parameter: const FVector & End
// Parameter: float Radius
// Parameter: float HalfHeight
// Parameter: EDrawDebugTrace::Type DrawDebugType
// Parameter: bool bHit
// Parameter: FHitResult & OutHit
// Parameter: FLinearColor TraceColor
// Parameter: FLinearColor TraceHitColor
// Parameter: float DrawTime
//************************************
void UVertUtilities::DrawDebugCapsuleTraceSingle(const UWorld* World, const FVector& Start, const FVector& End, float Radius, float HalfHeight, EDrawDebugTrace::Type DrawDebugType, bool bHit, FHitResult& OutHit, FLinearColor TraceColor, FLinearColor TraceHitColor, float DrawTime)
{
	if (DrawDebugType != EDrawDebugTrace::None)
	{
		bool bPersistent = DrawDebugType == EDrawDebugTrace::Persistent;
		float LifeTime = (DrawDebugType == EDrawDebugTrace::ForDuration) ? DrawTime : 0.f;

		if (bHit && OutHit.bBlockingHit)
		{
			// Red up to the blocking hit, green thereafter
			::DrawDebugCapsule(World, Start, HalfHeight, Radius, FQuat::Identity, TraceColor.ToFColor(true), bPersistent, LifeTime);
			::DrawDebugCapsule(World, OutHit.Location, HalfHeight, Radius, FQuat::Identity, TraceColor.ToFColor(true), bPersistent, LifeTime);
			::DrawDebugLine(World, Start, OutHit.Location, TraceColor.ToFColor(true), bPersistent, LifeTime);
			::DrawDebugPoint(World, OutHit.ImpactPoint, TRACE_DEBUG_IMPACTPOINT_SIZE, TraceColor.ToFColor(true), bPersistent, LifeTime);

			::DrawDebugCapsule(World, End, HalfHeight, Radius, FQuat::Identity, TraceHitColor.ToFColor(true), bPersistent, LifeTime);
			::DrawDebugLine(World, OutHit.Location, End, TraceHitColor.ToFColor(true), bPersistent, LifeTime);
		}
		else
		{
			// no hit means all red
			::DrawDebugCapsule(World, Start, HalfHeight, Radius, FQuat::Identity, TraceColor.ToFColor(true), bPersistent, LifeTime);
			::DrawDebugCapsule(World, End, HalfHeight, Radius, FQuat::Identity, TraceColor.ToFColor(true), bPersistent, LifeTime);
			::DrawDebugLine(World, Start, End, TraceColor.ToFColor(true), bPersistent, LifeTime);
		}
	}
}

//************************************
// Method:    DrawDebugCapsuleTraceMulti
// FullName:  UVertUtilities::DrawDebugCapsuleTraceMulti
// Access:    public static 
// Returns:   void
// Qualifier:
// Parameter: const UWorld * World
// Parameter: const FVector & Start
// Parameter: const FVector & End
// Parameter: float Radius
// Parameter: float HalfHeight
// Parameter: EDrawDebugTrace::Type DrawDebugType
// Parameter: bool bHit
// Parameter: TArray<FHitResult> & OutHits
// Parameter: FLinearColor TraceColor
// Parameter: FLinearColor TraceHitColor
// Parameter: float DrawTime
//************************************
void UVertUtilities::DrawDebugCapsuleTraceMulti(const UWorld* World, const FVector& Start, const FVector& End, float Radius, float HalfHeight, EDrawDebugTrace::Type DrawDebugType, bool bHit, TArray<FHitResult>& OutHits, FLinearColor TraceColor, FLinearColor TraceHitColor, float DrawTime)
{
	if (DrawDebugType != EDrawDebugTrace::None)
	{
		bool bPersistent = DrawDebugType == EDrawDebugTrace::Persistent;
		float LifeTime = (DrawDebugType == EDrawDebugTrace::ForDuration) ? DrawTime : 0.f;

		if (bHit && OutHits.Last().bBlockingHit)
		{
			// Red up to the blocking hit, green thereafter
			FVector const BlockingHitPoint = OutHits.Last().Location;
			::DrawDebugCapsule(World, Start, HalfHeight, Radius, FQuat::Identity, TraceColor.ToFColor(true), bPersistent, LifeTime);
			::DrawDebugCapsule(World, BlockingHitPoint, HalfHeight, Radius, FQuat::Identity, TraceColor.ToFColor(true), bPersistent, LifeTime);
			::DrawDebugLine(World, Start, BlockingHitPoint, TraceColor.ToFColor(true), bPersistent, LifeTime);

			::DrawDebugCapsule(World, End, HalfHeight, Radius, FQuat::Identity, TraceHitColor.ToFColor(true), bPersistent, LifeTime);
			::DrawDebugLine(World, BlockingHitPoint, End, TraceHitColor.ToFColor(true), bPersistent, LifeTime);
		}
		else
		{
			// no hit means all red
			::DrawDebugCapsule(World, Start, HalfHeight, Radius, FQuat::Identity, TraceColor.ToFColor(true), bPersistent, LifeTime);
			::DrawDebugCapsule(World, End, HalfHeight, Radius, FQuat::Identity, TraceColor.ToFColor(true), bPersistent, LifeTime);
			::DrawDebugLine(World, Start, End, TraceColor.ToFColor(true), bPersistent, LifeTime);
		}

		// draw hits
		for (int32 HitIdx = 0; HitIdx < OutHits.Num(); ++HitIdx)
		{
			FHitResult const& Hit = OutHits[HitIdx];
			::DrawDebugPoint(World, Hit.ImpactPoint, TRACE_DEBUG_IMPACTPOINT_SIZE, (Hit.bBlockingHit ? TraceColor.ToFColor(true) : TraceHitColor.ToFColor(true)), bPersistent, LifeTime);
		}
	}
}
#endif

//************************************
// Method:    IsActorInFrustum
// FullName:  UVertUtilities::IsActorInFrustum
// Access:    public static 
// Returns:   bool
// Qualifier:
// Parameter: AActor * Actor
//************************************
bool UVertUtilities::IsActorInFrustum(const UObject* worldContextObject, AActor* actor)
{
	if (worldContextObject)
	{
		if (UWorld* world = worldContextObject->GetWorld())
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
	}	

	return false;
}

//************************************
// Method:    GetSquareDistanceFromFrustum
// FullName:  UVertUtilities::GetSquareDistanceFromFrustum
// Access:    public static 
// Returns:   float
// Qualifier:
// Parameter: const UWorld * world
// Parameter: AActor * actor
//************************************
float UVertUtilities::GetSquareDistanceFromFrustum(const UWorld* world, AActor* actor)
{
	float distance = 0;
	return distance;
}

//************************************
// Method:    GetAnimationTime
// FullName:  UVertUtilities::GetAnimationTime
// Access:    public static 
// Returns:   float
// Qualifier:
// Parameter: UAnimSequence * animation
//************************************
float UVertUtilities::GetAnimationTime(UAnimSequence* animSequence)
{
	if (animSequence && animSequence->RateScale != 0)
	{
		return animSequence->SequenceLength / animSequence->RateScale;
	}

	return 0;
}

//************************************
// Method:    VERT_TakePointDamage
// FullName:  UVertUtilities::VERT_TakePointDamage
// Access:    public static 
// Returns:   float
// Qualifier:
// Parameter: AActor * victim
// Parameter: float damage
// Parameter: const FVertPointDamageEvent & damageEvent
// Parameter: Controller * eventInstigator
// Parameter: AActor * damageCauser
//************************************
float UVertUtilities::VERT_TakePointDamage(AActor* victim, float damage, TSubclassOf<UDamageType> damageType, const FVector& shotDirection, const FHitResult& hitInfo, float Knockback, float KnockbackScaling, float stunTime, class AController* eventInstigator, AActor* damageCauser)
{
	if (victim)
	{
		FVertPointDamageEvent damageEvent;
		damageEvent.Damage = damage;
		damageEvent.DamageTypeClass = damageType;
		damageEvent.ShotDirection = shotDirection;
		damageEvent.HitInfo = hitInfo;
		damageEvent.Knockback = Knockback;
		damageEvent.KnockbackScaling = KnockbackScaling;
		damageEvent.StunTime = stunTime;
		return victim->TakeDamage(damage, damageEvent, eventInstigator, damageCauser);
	}

	return 0;
}

/** @RETURN True if weapon trace from Origin hits component VictimComp.  OutHitResult will contain properties of the hit. */
static bool ComponentIsDamageableFrom(UPrimitiveComponent* VictimComp, FVector const& Origin, AActor const* IgnoredActor, const TArray<AActor*>& IgnoreActors, ECollisionChannel TraceChannel, FHitResult& OutHitResult)
{
	static FName NAME_ComponentIsVisibleFrom = FName(TEXT("ComponentIsVisibleFrom"));
	FCollisionQueryParams LineParams(NAME_ComponentIsVisibleFrom, true, IgnoredActor);
	LineParams.AddIgnoredActors(IgnoreActors);

	// Do a trace from origin to middle of box
	UWorld* const World = VictimComp->GetWorld();
	check(World);

	FVector const TraceEnd = VictimComp->Bounds.Origin;
	FVector TraceStart = Origin;
	if (Origin == TraceEnd)
	{
		// tiny nudge so LineTraceSingle doesn't early out with no hits
		TraceStart.Z += 0.01f;
	}
	bool const bHadBlockingHit = World->LineTraceSingleByChannel(OutHitResult, TraceStart, TraceEnd, TraceChannel, LineParams);
	//::DrawDebugLine(World, TraceStart, TraceEnd, FLinearColor::Red, true);

	// If there was a blocking hit, it will be the last one
	if (bHadBlockingHit)
	{
		if (OutHitResult.Component == VictimComp)
		{
			// if blocking hit was the victim component, it is visible
			return true;
		}
		else
		{
			// if we hit something else blocking, it's not
			UE_LOG(LogDamage, Log, TEXT("Radial Damage to %s blocked by %s (%s)"), *GetNameSafe(VictimComp), *GetNameSafe(OutHitResult.GetActor()), *GetNameSafe(OutHitResult.Component.Get()));
			return false;
		}
	}

	// didn't hit anything, assume nothing blocking the damage and victim is consequently visible
	// but since we don't have a hit result to pass back, construct a simple one, modeling the damage as having hit a point at the component's center.
	FVector const FakeHitLoc = VictimComp->GetComponentLocation();
	FVector const FakeHitNorm = (Origin - FakeHitLoc).GetSafeNormal();		// normal points back toward the epicenter
	OutHitResult = FHitResult(VictimComp->GetOwner(), VictimComp, FakeHitLoc, FakeHitNorm);
	return true;
}

//************************************
// Method:    VERT_ApplyRadialDamage
// FullName:  UVertUtilities::VERT_ApplyRadialDamage
// Access:    public static 
// Returns:   bool
// Qualifier:
// Parameter: const UObject * WorldContextObject
// Parameter: float BaseDamage
// Parameter: const FVector & Origin
// Parameter: float DamageRadius
// Parameter: TSubclassOf<class UDamageType> DamageTypeClass
// Parameter: const TArray<AActor * > & IgnoreActors
// Parameter: AActor * DamageCauser
// Parameter: AController * InstigatedByController
// Parameter: bool bDoFullDamage
// Parameter: ECollisionChannel DamagePreventionChannel
//************************************
bool UVertUtilities::VERT_ApplyRadialDamage(const UObject* WorldContextObject, float BaseDamage, float BaseKnockback, float KnockbackScaling, float stunTime, const FVector& Origin, float DamageRadius, TSubclassOf<class UDamageType> DamageTypeClass, const TArray<AActor*>& IgnoreActors, AActor* DamageCauser/* = NULL*/, AController* InstigatedByController /*= NULL*/, bool bDoFullDamage /*= false*/, ECollisionChannel DamagePreventionChannel /*= ECC_Visibility*/)
{
	float DamageFalloff = bDoFullDamage ? 0.f : 1.f;
	return VERT_ApplyRadialDamageWithFalloff(WorldContextObject, BaseDamage, BaseKnockback, KnockbackScaling, stunTime, 0.f, Origin, 0.f, DamageRadius, DamageFalloff, DamageTypeClass, IgnoreActors, DamageCauser, InstigatedByController, DamagePreventionChannel);
}

//************************************
// Method:    VERT_ApplyRadialDamageWithFalloff
// FullName:  UVertUtilities::VERT_ApplyRadialDamageWithFalloff
// Access:    private 
// Returns:   bool
// Qualifier:
// Parameter: const UObject * WorldContextObject
// Parameter: float BaseDamage
// Parameter: float MinimumDamage
// Parameter: const FVector & Origin
// Parameter: float DamageInnerRadius
// Parameter: float DamageOuterRadius
// Parameter: float DamageFalloff
// Parameter: TSubclassOf<class UDamageType> DamageTypeClass
// Parameter: const TArray<AActor * > & IgnoreActors
// Parameter: AActor * DamageCauser
// Parameter: AController * InstigatedByController
// Parameter: ECollisionChannel DamagePreventionChannel
//************************************
bool UVertUtilities::VERT_ApplyRadialDamageWithFalloff(const UObject* WorldContextObject, float BaseDamage, float BaseKnockback, float KnockbackScaling, float StunTime, float MinimumDamage, const FVector& Origin, float DamageInnerRadius, float DamageOuterRadius, float DamageFalloff, TSubclassOf<class UDamageType> DamageTypeClass, const TArray<AActor*>& IgnoreActors, AActor* DamageCauser, AController* InstigatedByController, ECollisionChannel DamagePreventionChannel)
{
	static FName NAME_ApplyRadialDamage = FName(TEXT("VERT_ApplyRadialDamage"));
	FCollisionQueryParams SphereParams(NAME_ApplyRadialDamage, false, DamageCauser);

	SphereParams.AddIgnoredActors(IgnoreActors);

	// query scene to see what we hit
	TArray<FOverlapResult> Overlaps;
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject);
	World->OverlapMultiByObjectType(Overlaps, Origin, FQuat::Identity, FCollisionObjectQueryParams(FCollisionObjectQueryParams::InitType::AllDynamicObjects), FCollisionShape::MakeSphere(DamageOuterRadius), SphereParams);

	// collate into per-actor list of hit components
	TMap<AActor*, TArray<FHitResult> > OverlapComponentMap;
	for (int32 Idx = 0; Idx < Overlaps.Num(); ++Idx)
	{
		FOverlapResult const& Overlap = Overlaps[Idx];
		AActor* const OverlapActor = Overlap.GetActor();

		if (OverlapActor &&
			OverlapActor->bCanBeDamaged &&
			OverlapActor != DamageCauser &&
			Overlap.Component.IsValid())
		{
			FHitResult Hit;
			if (DamagePreventionChannel == ECC_MAX || ComponentIsDamageableFrom(Overlap.Component.Get(), Origin, DamageCauser, IgnoreActors, DamagePreventionChannel, Hit))
			{
				TArray<FHitResult>& HitList = OverlapComponentMap.FindOrAdd(OverlapActor);
				HitList.Add(Hit);
			}
		}
	}

	bool bAppliedDamage = false;

	if (OverlapComponentMap.Num() > 0)
	{
		// make sure we have a good damage type
		TSubclassOf<UDamageType> const ValidDamageTypeClass = DamageTypeClass ? DamageTypeClass : TSubclassOf<UDamageType>(UDamageType::StaticClass());

		FVertRadialDamageEvent DmgEvent;
		DmgEvent.DamageTypeClass = ValidDamageTypeClass;
		DmgEvent.Origin = Origin;
		DmgEvent.Params = FRadialDamageParams(BaseDamage, MinimumDamage, DamageInnerRadius, DamageOuterRadius, DamageFalloff);
		DmgEvent.Knockback = BaseKnockback;
		DmgEvent.KnockbackScaling = KnockbackScaling;
		DmgEvent.StunTime = StunTime;

		// call damage function on each affected actors
		for (TMap<AActor*, TArray<FHitResult> >::TIterator It(OverlapComponentMap); It; ++It)
		{
			AActor* const Victim = It.Key();
			TArray<FHitResult> const& ComponentHits = It.Value();
			DmgEvent.ComponentHits = ComponentHits;

			Victim->TakeDamage(BaseDamage, DmgEvent, InstigatedByController, DamageCauser);

			bAppliedDamage = true;
		}
	}

	return bAppliedDamage;
}

//************************************
// Method:    CallSinglePod
// FullName:  CallSinglePod
// Access:    public 
// Returns:   void
// Qualifier:
// Parameter: const UObject * worldContextObject
// Parameter: int32 targetIndex
// Parameter: const TArray<FVector> & spawnLocations
// Parameter: int32 numberOfPods
// Parameter: int32 itemsPerPod
// Parameter: float delayTime
// Parameter: UParticleSystem * warningFX
// Parameter: float warningTime
//************************************
void CallSinglePod(const UObject* worldContextObject, int32 targetIndex, const TArray<AActor*>& spawnLocations, int32 numberOfPods, int32 itemsPerPod, float delayTime, UParticleSystem* warningFX, FName warningBeamEndParam, float warningTime)
{
	if (targetIndex >= spawnLocations.Num())
		return;
	
	UWorld* world = worldContextObject->GetWorld();
	if (world == NULL)
		return;

	FVector podTarget = spawnLocations[targetIndex]->GetActorLocation();
	int32 randomSeed = FMath::Rand();
	FRandomStream PodRandomStream(randomSeed);
	FVector direction = PodRandomStream.VRandCone(FVector::UpVector, FMath::DegreesToRadians(30.f), FMath::DegreesToRadians(30.f));
	direction.Y = 0;
	FTransform spawnTransform(FQuat::Identity.Rotator(), podTarget + (direction * 5000.f));

	UParticleSystemComponent* particleSystem = UGameplayStatics::SpawnEmitterAtLocation(world, warningFX, spawnTransform);
	if (particleSystem)
	{
		particleSystem->SetVectorParameter(warningBeamEndParam, podTarget);
	}

	FTimerHandle warningTimer;
	world->GetTimerManager().SetTimer(warningTimer, [world, itemsPerPod, spawnTransform, direction](void) mutable {

		if (!world)
			return;

		AVertGameMode* gameMode = world->GetAuthGameMode<AVertGameMode>();
		if (gameMode)
		{
			TArray<TSubclassOf<AActor>> payload;
			for (int32 i = 0; i <= itemsPerPod; ++i)
			{
				payload.Add(gameMode->AvailableItemSpawns[FMath::RandRange(0, gameMode->AvailableItemSpawns.Num() - 1)]);
			}
			gameMode->PrepareDroppod(payload, spawnTransform, -direction);
		}
	}, warningTime, false);
}

//************************************
// Method:    InitiateDroppodsRecursive
// FullName:  InitiateDroppodsRecursive
// Access:    public 
// Returns:   void
// Qualifier:
// Parameter: const UObject * worldContextObject
// Parameter: int32 iteration
// Parameter: const TArray<FVector> & spawnLocations
// Parameter: int32 numberOfPods
// Parameter: int32 itemsPerPod
// Parameter: float delayTime
// Parameter: UParticleSystem * warningFX
// Parameter: FName warningBeamEndParam
// Parameter: float warningTime
//************************************
void InitiateDroppodsRecursive(const UObject* worldContextObject, int32 iteration, const TArray<AActor*>& spawnLocations, int32 numberOfPods, int32 itemsPerPod, float delayTime, UParticleSystem* warningFX, FName warningBeamEndParam, float warningTime)
{
	CallSinglePod(worldContextObject, iteration++, spawnLocations, numberOfPods, itemsPerPod, delayTime, warningFX, warningBeamEndParam, warningTime);

	if (iteration >= spawnLocations.Num())
		return;

	if (worldContextObject == NULL)
		return;

	UWorld* world = worldContextObject->GetWorld();
	if (world == NULL)
		return;

	FTimerHandle delayTimer;
	world->GetTimerManager().SetTimer(delayTimer, [worldContextObject, iteration, spawnLocations, numberOfPods, itemsPerPod, delayTime, warningFX, warningBeamEndParam, warningTime](void) {
		if (worldContextObject == NULL)
			return;

		InitiateDroppodsRecursive(worldContextObject, iteration, spawnLocations, numberOfPods, itemsPerPod, delayTime, warningFX, warningBeamEndParam, warningTime);
	}, delayTime, false);
}

//************************************
// Method:    VERT_MakeWeaponDrop
// FullName:  UVertUtilities::VERT_MakeWeaponDrop
// Access:    public static 
// Returns:   void
// Qualifier:
// Parameter: TArray<FVector> spawnLocations
// Parameter: int32 numberOfPods
// Parameter: int32 itemsPerPod
// Parameter: float delayTime
// Parameter: UParticleSystem * warningFX
// Parameter: float warningTime
//************************************
void UVertUtilities::VERT_MakeWeaponDrop(const UObject* worldContextObject, const TArray<AActor*>& spawnLocations, int32 numberOfPods, int32 itemsPerPod, float delayTime, UParticleSystem* warningFX /* = nullptr */, FName warningBeamEndParam /*= NAME_None*/, float warningTime /* = 2.f */)
{
	TArray<int32> chosenNumbers;
	TArray<AActor*> chosenLocations;

	for (int32 i = 0; i < FMath::Min(numberOfPods, spawnLocations.Num()); ++i)
	{
		int32 randomNumber = FMath::RandRange(0, spawnLocations.Num() - 1);
		while (chosenNumbers.Find(randomNumber) != INDEX_NONE) // ensure that each chosen pod location is unique
		{
			randomNumber = FMath::RandRange(0, spawnLocations.Num() - 1);
		}
		chosenNumbers.Add(randomNumber);
		chosenLocations.Add(spawnLocations[randomNumber]);
	}

	InitiateDroppodsRecursive(worldContextObject, 0, chosenLocations, numberOfPods, itemsPerPod, delayTime, warningFX, warningBeamEndParam, warningTime);
}

//************************************
// Method:    GetRatioDistanceFromCamera
// FullName:  UVertUtilities::GetRatioDistanceFromCamera
// Access:    public static 
// Returns:   void
// Qualifier:
// Parameter: const UObject * worldContextObject
// Parameter: const AActor * checkActor
// Parameter: float & outX
// Parameter: float & outZ
// Parameter: float & outHorizontalDistanceToEdge
// Parameter: float & outVerticalDistanceToEdge
//************************************
void UVertUtilities::GetRatioDistanceFromCamera(const UObject* worldContextObject, const AActor* checkActor, float& outX, float& outZ, float& outHorizontalDistanceToEdge, float& outVerticalDistanceToEdge)
{
	outX = outZ = outHorizontalDistanceToEdge = outVerticalDistanceToEdge -1.f;

	if (worldContextObject && checkActor)
	{
		if (AVertGameMode* gameMode = worldContextObject->GetWorld()->GetAuthGameMode<AVertGameMode>())
		{
			if (AVertPlayerCameraActor* camera = gameMode->GetActivePlayerCamera())
			{
				outVerticalDistanceToEdge = camera->GetCurrentArmLength() * 0.5f; // #MI_TODO: multiply this and the below line by the aspect ratio to ensure this code works for all screens.
				outHorizontalDistanceToEdge = camera->GetCurrentArmLength();
				float distanceFromCenterV = FMath::Abs(checkActor->GetActorLocation().Z - camera->GetActorLocation().Z);
				float distanceFromCenterH = FMath::Abs(checkActor->GetActorLocation().X - camera->GetActorLocation().X);
				outX = distanceFromCenterH / outHorizontalDistanceToEdge;
				outZ = distanceFromCenterV / outVerticalDistanceToEdge;
			}
		}

		UE_LOG(LogVertUtilities, Warning, TEXT("Invalid active game mode."));
	}
}

//************************************
// Method:    FadeFromBlack
// FullName:  UVertUtilities::FadeFromBlack
// Access:    public static 
// Returns:   void
// Qualifier:
// Parameter: const UObject * worldContextObject
// Parameter: float duration
//************************************
void UVertUtilities::FadeFromBlack(const UObject* worldContextObject, float duration)
{
	if (worldContextObject)
	{
		const UWorld* world = worldContextObject->GetWorld();
		if (world)
		{
			UVertViewportClient* viewportClient = Cast<UVertViewportClient>(world->GetGameViewport());
			if (viewportClient)
			{
				viewportClient->Fade(duration, false);
			}
		}
	}
}

//************************************
// Method:    FadeToBlack
// FullName:  UVertUtilities::FadeToBlack
// Access:    public static 
// Returns:   void
// Qualifier:
// Parameter: const UObject * worldContextObject
// Parameter: float duration
//************************************
void UVertUtilities::FadeToBlack(const UObject* worldContextObject, float duration)
{
	if (worldContextObject)
	{
		const UWorld* world = worldContextObject->GetWorld();
		if (world)
		{
			UVertViewportClient* viewportClient = Cast<UVertViewportClient>(world->GetGameViewport());
			if (viewportClient)
			{
				viewportClient->Fade(duration, true);
			}
		}
	}
}

//************************************
// Method:    LineToLineIntersection
// FullName:  UVertUtilities::LineToLineIntersection
// Access:    public static 
// Returns:   bool
// Qualifier:
// Parameter: const FVector & fromA
// Parameter: const FVector & fromB
// Parameter: const FVector & toA
// Parameter: const FVector & toB
// Parameter: FVector & outIntersection
//************************************
bool UVertUtilities::LineToLineIntersection(const FVector& fromA, const FVector& fromB, const FVector& toA, const FVector& toB, FVector& outIntersection)
{
	FVector da = fromB - fromA;
	FVector db = toB - toA;
	FVector dc = toA - fromA;

	if (FVector::DotProduct(dc, FVector::CrossProduct(da, db)) != 0.0) {
		return false;
	}

	FVector crossDaDb = FVector::CrossProduct(da, db);
	float prod = crossDaDb.X * crossDaDb.X + crossDaDb.Y * crossDaDb.Y + crossDaDb.Z * crossDaDb.Z;

	float res = FVector::DotProduct(FVector::CrossProduct(dc, db), FVector::CrossProduct(da, db) / prod);
	if (res >= 0.0f && res <= 1.0f) {
		outIntersection = fromA + da * FVector(res, res, res);
		return true;
	}

	return false;
}