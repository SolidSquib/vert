#pragma once

#include "DebugGroup.generated.h"

USTRUCT()
struct FDebugGroup
{
	GENERATED_BODY()

		UPROPERTY(EditAnywhere, Category = DebugGroup)
		bool Enabled;

	UPROPERTY(EditAnywhere, Category = DebugGroup)
		FColor MessageColour;

	FDebugGroup()
	{
		Enabled = false;
		MessageColour = FColor::Yellow;
	}
};
