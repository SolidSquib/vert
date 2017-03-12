// Fill out your copyright notice in the Description page of Project Settings.

#include "Vert.h"
#include "VertUtilities.h"

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

FVector2D UVertUtilities::LimitAimTrajectory2D(EAimFreedom mode, const FVector2D& vector)
{
	FVector2D fixedVector;

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

FVector2D UVertUtilities::SnapVector2DToAngle(const FVector2D& vector, float degrees)
{
	FVector vector3D(vector.X, 0.f, vector.Y);
	vector3D = SnapVectorToAngle(vector3D, degrees);
	FVector2D newVector(vector3D.X, vector3D.Z);

	return (newVector * 100).GetSafeNormal();
}


