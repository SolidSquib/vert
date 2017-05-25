// Copyright Inside Out Games Ltd. 2017

#pragma once

#include "Debuggable.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UDebuggable : public UInterface
{
	GENERATED_UINTERFACE_BODY()
};

/**
 * 
 */
class VERT_API IDebuggable
{
	GENERATED_IINTERFACE_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	virtual void EnableDebug(bool enable);

protected:
	virtual void DrawDebugInfo() = 0;

protected:
	bool mShowDebug = false;
};
