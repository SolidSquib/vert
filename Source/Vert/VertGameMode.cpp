// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "Vert.h"
#include "VertGameMode.h"
#include "VertCharacter.h"

AVertGameMode::AVertGameMode()
{
	// set default pawn class to our character
	DefaultPawnClass = AVertCharacter::StaticClass();	
}
