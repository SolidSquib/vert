// Copyright Inside Out Games Limited 2017

#pragma once

#include "CoreMinimal.h"
#include "Components/WidgetComponent.h"
#include "AimArrowWidgetComponent.generated.h"

/**
 * 
 */
UCLASS()
class VERT_API UAimArrowWidgetComponent : public UWidgetComponent
{
	GENERATED_BODY()
	
public:
	UAimArrowWidgetComponent();

	void SetColour(const FLinearColor& newColour);
	void SetAimDirection(const FVector& newAimDirection);
};
