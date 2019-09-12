// Copyright Inside Out Games Limited 2017

#include "AimArrowWidgetComponent.h"

UAimArrowWidgetComponent::UAimArrowWidgetComponent()
{
	Space = EWidgetSpace::Screen;
}

void UAimArrowWidgetComponent::SetAimDirection(const FVector& newAimDirection)
{
	if (Widget)
	{
		float pitch = newAimDirection.Rotation().Pitch;
		float angle = FMath::Clamp(pitch -90.f, -180.f, 180.f);
		if (newAimDirection.X > 0)
			angle = -angle;
		
		Widget->SetRenderAngle(angle);
	}
}

void UAimArrowWidgetComponent::SetColour(const FLinearColor& newColour)
{
	if (Widget)
	{
		Widget->SetColorAndOpacity(newColour);
	}
}