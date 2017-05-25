// Copyright Inside Out Games Ltd. 2017

#include "Vert.h"
#include "SplineActor.h"

ASplineActor::ASplineActor(const FObjectInitializer& objectInitializer) : Super(objectInitializer)
{
	Spline = CreateDefaultSubobject<USplineComponent>(TEXT("SplineComponent"));
}