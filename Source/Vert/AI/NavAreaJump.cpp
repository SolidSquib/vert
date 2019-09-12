// Copyright Inside Out Games Ltd. 2017

#include "NavAreaJump.h"

UNavAreaJump::UNavAreaJump(const class FObjectInitializer& PCIP)
	:Super(PCIP)
{
	FNavAreaHelper::Set(AreaFlags, ENavAreaFlag::Jump);
	DefaultCost = 1.1f;
}