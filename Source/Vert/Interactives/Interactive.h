// Copyright Inside Out Games Ltd. 2017

#pragma once

#include "Interactive.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UInteractive : public UInterface
{
	GENERATED_UINTERFACE_BODY()
};

/**
 * 
 */
class VERT_API IInteractive
{
	GENERATED_IINTERFACE_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	virtual void Interact(TWeakObjectPtr<class UCharacterInteractionComponent> instigator) = 0;
};
