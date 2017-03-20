#pragma once

#include "AxisPositions.generated.h"

USTRUCT()
struct FAxisPositions
{
	GENERATED_BODY()

	float RightX, RightY, LeftX, LeftY;
	FVector MouseDirection;

	FORCEINLINE FVector GetPlayerLeftThumbstick() const { return FVector(LeftX, 0.f, LeftY); }
	FORCEINLINE FVector GetPlayerRightThumbstick() const { return FVector(RightX, 0.f, RightY); }
	FORCEINLINE FVector2D GetPlayerLeftThumbstick2D() const { return FVector2D(LeftX, LeftY); }
	FORCEINLINE FVector2D GetPlayerRightThumbstick2D() const { return FVector2D(RightX, RightY); }
	FORCEINLINE FVector GetPlayerLeftThumbstickDirection() const { return (FVector(LeftX, 0.f, LeftY) * 100).GetSafeNormal(); }
	FORCEINLINE FVector GetPlayerRightThumbstickDirection() const { return (FVector(RightX, 0.f, RightY) * 100).GetSafeNormal(); }
	FORCEINLINE FVector2D GetPlayerLeftThumbstickDirection2D() const { return (FVector2D(LeftX, LeftY) * 100).GetSafeNormal(); }
	FORCEINLINE FVector2D GetPlayerRightThumbstickDirection2D() const { return (FVector2D(RightX, RightY) * 100).GetSafeNormal(); }
	FORCEINLINE FVector GetPlayerMouseDirection() const { return MouseDirection; }

	FAxisPositions()
	{
		RightX = 0;
		RightY = 0;
		LeftX = 0;
		LeftY = 0;
		MouseDirection = FVector::ZeroVector;
	}
};