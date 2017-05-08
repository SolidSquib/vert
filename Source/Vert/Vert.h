// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

// This file is the private precompiled header for your game.
// You must include it first in each .cpp file.
//
// Place any includes here that are needed by the majority of your .cpp files

#include "Engine.h"
#include "Net/UnrealNetwork.h"

// Engine
#include "Kismet/KismetMathLibrary.h"
#include "Engine/VertUtilities.h"
#include "Engine/VertGameInstance.h"
#include "Engine/VertCameraManager.h"
#include "Engine/VertPlayerController.h"
#include "Engine/VertGameState.h"
#include "Engine/VertPlayerState.h"

// Components
#include "CableComponent.h"
#include "PaperFlipbookComponent.h"

// Actors
#include "VertCharacter.h"
#include "VertSpectator.h"
#include "GameFramework/Pawn.h"
#include "VertGameMode.h"
#include "UserInterface/VertHUD.h"
#include "Weapons/BaseWeapon.h"

// Interfaces
#include "Interactives/Interactive.h"

// Define custom collision object types in code
#define ECC_Grappler ECC_GameTraceChannel1
#define ECC_Interactive ECC_GameTraceChannel2
#define ECC_SphereTracer ECC_GameTraceChannel3
#define ECC_LedgeTracer ECC_GameTraceChannel4
#define ECC_CharacterHitBox ECC_GameTraceChannel5
#define ECC_HeldWeapon ECC_GameTraceChannel6
#define ECC_WeaponTrace ECC_GameTraceChannel7