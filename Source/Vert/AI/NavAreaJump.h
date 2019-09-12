// Copyright Inside Out Games Ltd. 2017

#pragma once

#include "AI/Navigation/RecastNavMesh.h"
#include "CoreMinimal.h"
#include "AI/Navigation/NavAreas/NavArea.h"
#include "NavAreaJump.generated.h"

/**
 * 
 */
UCLASS()
class VERT_API UNavAreaJump : public UNavArea
{
	GENERATED_BODY()
	
		UNavAreaJump(const FObjectInitializer & PCIP);
	
	
};

UENUM()
namespace ENavAreaFlag
{
	// Up to 15 values can be added here
	enum Type
	{
		Deafault,
		Jump,
		Crouch
		//etc
	};
}

namespace FNavAreaHelper
{
	FORCEINLINE bool IsSet(uint16 Flags, ENavAreaFlag::Type Bit) { return (Flags & (1 << Bit)) != 0; }
	FORCEINLINE void Set(uint16& Flags, ENavAreaFlag::Type Bit) { Flags |= (1 << Bit); }

	FORCEINLINE bool IsNavLink(const FNavPathPoint& PathVert) { return (FNavMeshNodeFlags(PathVert.Flags).PathFlags & RECAST_STRAIGHTPATH_OFFMESH_CONNECTION) != 0; }
	FORCEINLINE bool HasJumpFlag(const FNavPathPoint& PathVert) { return IsSet(FNavMeshNodeFlags(PathVert.Flags).AreaFlags, ENavAreaFlag::Jump); }
	FORCEINLINE bool HasCrouchFlag(const FNavPathPoint& PathVert) { return IsSet(FNavMeshNodeFlags(PathVert.Flags).AreaFlags, ENavAreaFlag::Crouch); }
}