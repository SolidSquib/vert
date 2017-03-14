// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

// This file is the private precompiled header for your game.
// You must include it first in each .cpp file.
//
// Place any includes here that are needed by the majority of your .cpp files

#if 1
#include "Engine.h"
#else
#include "EngineMinimal.h"
#endif

// Engine
#include "Kismet/KismetMathLibrary.h"
#include "Engine/VertUtilities.h"

// Components
#include "CableComponent.h"
#include "PaperFlipbookComponent.h"

// Actors
#include "VertCharacter.h"
#include "Environment/GrapplePoint.h"

//General Log
DECLARE_LOG_CATEGORY_EXTERN(CharacterLog, Log, All);

//Logging during game startup
DECLARE_LOG_CATEGORY_EXTERN(Startup, Log, All);

//Logging for your AI system
DECLARE_LOG_CATEGORY_EXTERN(VertAI, Log, All);

//Logging for Critical Errors that must always be addressed
DECLARE_LOG_CATEGORY_EXTERN(VertCritical, Log, All);