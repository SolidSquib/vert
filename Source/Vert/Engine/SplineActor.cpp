// Copyright Inside Out Games Ltd. 2017

#include "SplineActor.h"
#include "Vert.h"

ASplineActor::ASplineActor(const FObjectInitializer& objectInitializer) : Super(objectInitializer)
{
	Spline = CreateDefaultSubobject<USplineComponent>(TEXT("SplineComponent"));
}

FVector ASplineActor::GetLocationAtTime(float time, ESplineCoordinateSpace::Type coords, bool constantVelocity)
{
	return Spline->GetLocationAtTime(time, coords, constantVelocity);
}

void ASplineActor::GetLocationAndTangentAtSplinePoint(int32 point, FVector& location, FVector& tangent, ESplineCoordinateSpace::Type coords)
{
	Spline->GetLocationAndTangentAtSplinePoint(point, location, tangent, coords);
}

FVector ASplineActor::GetRightVectorAtTime(float time, ESplineCoordinateSpace::Type coords, bool constantVelocity)
{
	return Spline->GetRightVectorAtTime(time, coords, constantVelocity);
}

FVector ASplineActor::GetDirectionAtTime(float time, ESplineCoordinateSpace::Type coords, bool constantVelocity)
{
	return Spline->GetDirectionAtTime(time, coords, constantVelocity);
}

int32 ASplineActor::GetNumberOfSplinePoints() const
{
	return Spline->GetNumberOfSplinePoints();
}

float ASplineActor::GetSplineLength()
{
	return Spline->GetSplineLength();
}