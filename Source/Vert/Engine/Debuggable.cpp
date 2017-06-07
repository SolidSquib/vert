// Copyright Inside Out Games Ltd. 2017

#include "Debuggable.h"


// This function does not need to be modified.
UDebuggable::UDebuggable(const class FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
}

void IDebuggable::EnableDebug(bool enable)
{
	mShowDebug = enable;
}