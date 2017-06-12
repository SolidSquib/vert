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
#include "Engine/VertGlobals.h"
#include "Engine/VertLevelScriptActor.h"

// Components
#include "CableComponent.h"

// Actors
#include "VertCharacter.h"
#include "VertSpectator.h"
#include "GameFramework/Pawn.h"
#include "VertGameMode.h"
#include "UserInterface/VertHUD.h"
#include "Weapons/BaseWeapon.h"
#include "Weapons/WeaponProjectile.h"
#include "Components/SplineComponent.h"

// UObjects
#include "Weapons/VertDamageType.h"

// Interfaces
#include "Interactives/Interactive.h"