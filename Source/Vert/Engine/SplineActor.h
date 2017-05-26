// Copyright Inside Out Games Ltd. 2017

#pragma once

#include "GameFramework/Actor.h"
#include "SplineActor.generated.h"

/**
 * 
 */
UCLASS(Blueprintable, BlueprintType, ClassGroup = (Splines))
class VERT_API ASplineActor : public AActor
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Spline)
	class USplineComponent* Spline;

public:
	FVector GetLocationAtTime(float time, ESplineCoordinateSpace::Type coords, bool constantVelocity);
	void GetLocationAndTangentAtSplinePoint(int32 point, FVector& location, FVector& tangent, ESplineCoordinateSpace::Type coords);
	FVector GetRightVectorAtTime(float time, ESplineCoordinateSpace::Type coords, bool constantVelocity);
	FVector GetDirectionAtTime(float time, ESplineCoordinateSpace::Type coords, bool constantVelocity);
	int32 GetNumberOfSplinePoints() const;

public:
	ASplineActor();
};
