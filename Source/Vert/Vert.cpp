// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "Vert.h"

IMPLEMENT_PRIMARY_GAME_MODULE( FDefaultGameModuleImpl, Vert, "Vert" );

//General Log
DEFINE_LOG_CATEGORY(CharacterLog);

//Logging during game startup
DEFINE_LOG_CATEGORY(Startup);

//Logging for your AI system
DEFINE_LOG_CATEGORY(VertAI);

//Logging for Critical Errors that must always be addressed
DEFINE_LOG_CATEGORY(VertCritical);