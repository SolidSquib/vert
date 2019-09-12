/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////
///			Copyright 2017 (C) Bruno Xavier B. Leite
//////////////////////////////////////////////////////////////
/*
	BY EXECUTING, READING, EDITING, COPYING OR KEEPING FILES FROM THIS SOFTWARE SOURCE CODE,
	YOU AGREE TO THE FOLLOWING TERMS IN ADDITION TO EPIC GAMES MARKETPLACE EULA:
	- YOU HAVE READ AND AGREE TO EPIC GAMES TERMS: https://publish.unrealengine.com/faq
	- YOU AGREE DEVELOPER RESERVES ALL RIGHTS TO THE SOFTWARE PROVIDED, GRANTED BY LAW.
	- YOU AGREE YOU'LL NOT CREATE OR PUBLISH DERIVATIVE SOFTWARE TO THE MARKETPLACE.
	- YOU AGREE DEVELOPER WILL NOT PROVIDE SOFTWARE OUTSIDE MARKETPLACE ENVIRONMENT.
	- YOU AGREE DEVELOPER WILL NOT PROVIDE PAID OR EXCLUSIVE SUPPORT SERVICES.
	- YOU AGREE DEVELOPER PROVIDED SUPPORT CHANNELS, ARE UNDER HIS SOLE DISCRETION.
*/
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "Runtime/Engine/Classes/Engine/Engine.h"
#include "PoolSpawnOptions.generated.h"

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

UENUM(Category = "Object Pool", BlueprintType)
enum class EPoolCollisionType : uint8 {
	NoCollision					UMETA(DisplayName="No Collision"), 
	QueryOnly						UMETA(DisplayName="Query Only (No Physics Collision)"),
	PhysicsOnly					UMETA(DisplayName="Physics Only (No Query Collision)"),
	QueryAndPhysics			UMETA(DisplayName="Collision Enabled (Query and Physics)") 
};

USTRUCT(Category = "Object Pool", BlueprintType, meta = (DisplayName = "Pool Spawn Options"))
struct OBJPOOL_API FPoolSpawnOptions {
	GENERATED_USTRUCT_BODY()
	//
	UPROPERTY(Category = STRING_NONE, EditAnywhere, SaveGame, BlueprintReadWrite)
	EPoolCollisionType CollisionType;
	//
	UPROPERTY(Category = STRING_NONE, EditAnywhere, SaveGame, BlueprintReadWrite)
	bool EnableCollision;
	//
	UPROPERTY(Category = STRING_NONE, EditAnywhere, SaveGame, BlueprintReadWrite)
	bool SimulatePhysics;
	//
	UPROPERTY(Category = STRING_NONE, EditAnywhere, SaveGame, BlueprintReadWrite)
	bool ActorTickEnabled;
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////