#pragma once

#include "GameFramework/Actor.h"
#include "VertTimer.h"
#include "VertUtilities.h"
#include "VertGlobals.generated.h"

UENUM()
enum class ERechargeRule : uint8
{
	OnRechargeTimer UMETA(DisplayName = "Always recharge"),
	OnContactGround UMETA(DisplayName = "Recharge on ground"),
	OnContactGroundOrLatchedAnywhere UMETA(DisplayName = "Recharge on ground / Latched to any surface")
};
