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
#include "Engine/VertGameInstance.h"
#include "Engine/VertCameraManager.h"
#include "Engine/VertPlayerController.h"

// Components
#include "CableComponent.h"
#include "PaperFlipbookComponent.h"

// Actors
#include "VertCharacter.h"
#include "VertSpectator.h"
#include "Environment/GrapplePoint.h"
#include "GameFramework/Pawn.h"
#include "VertGameMode.h"

// Define custom collision object types in code
#define ECC_Grappler ECC_GameTraceChannel1
#define ECC_Interactive ECC_GameTraceChannel2