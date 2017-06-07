// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "VertDamageType.generated.h"

// DamageType class that specifies an icon to display
UCLASS(const, Blueprintable, BlueprintType)
class UVertDamageType : public UDamageType
{
	GENERATED_UCLASS_BODY()

	/** force feedback effect to play on a player hit by this damage type */
	UPROPERTY(EditDefaultsOnly, Category = Effects)
	UForceFeedbackEffect *HitForceFeedback;

	/** force feedback effect to play on a player killed by this damage type */
	UPROPERTY(EditDefaultsOnly, Category = Effects)
	UForceFeedbackEffect *KilledForceFeedback;
};