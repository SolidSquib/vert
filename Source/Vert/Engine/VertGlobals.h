#pragma once

#include "GameFramework/Actor.h"
#include "VertTimer.h"
#include "VertUtilities.h"
#include "VertGlobals.generated.h"

UENUM(BlueprintType)
enum class ECustomMovementMode : uint8
{
	MOVE_Climbing UMETA(DisplayName = "Climbing"),
	MOVE_GrappleWalk UMETA(DisplayName = "Grappling - Ground"),
	MOVE_GrappleFall UMETA(DisplayName = "Grappling - Air")
};

UENUM()
enum class ERechargeRule : uint8
{
	OnRechargeTimer UMETA(DisplayName = "Always recharge"),
	OnContactGround UMETA(DisplayName = "Recharge on ground"),
	OnContactGroundOrLatchedAnywhere UMETA(DisplayName = "Recharge on ground / Latched to any surface")
};

UENUM(BlueprintType)
enum class EScoreTrack : uint8
{
	SCORE_BountyHunter,
	SCORE_HeavyHitter,
	SCORE_KillingSpree,
	SCORE_GlassCannon,
	SCORE_MonsterHunter,
	SCORE_SpeedRacer,
	SCORE_MasterOfArms,
	SCORE_Survivor,
	SCORE_RubberBand
};

namespace EVertMatchState
{
	enum Type
	{
		Warmup,
		Playing,
		Won,
		Lost,
	};
}

/** keep in sync with VertImpactEffect */
UENUM()
namespace EVertPhysMaterialType
{
	enum Type
	{
		Unknown,
		Concrete,
		Dirt,
		Water,
		Metal,
		Wood,
		Grass,
		Glass,
		Flesh,
	};
}

namespace EVertDialogType
{
	enum Type
	{
		None,
		Generic,
		ControllerDisconnected
	};
}

#define VERT_SURFACE_Default	SurfaceType_Default
#define VERT_SURFACE_Concrete	SurfaceType1
#define VERT_SURFACE_Dirt		SurfaceType2
#define VERT_SURFACE_Water		SurfaceType3
#define VERT_SURFACE_Metal		SurfaceType4
#define VERT_SURFACE_Wood		SurfaceType5
#define VERT_SURFACE_Grass		SurfaceType6
#define VERT_SURFACE_Glass		SurfaceType7
#define VERT_SURFACE_Flesh		SurfaceType8

// Define custom collision object types in code
#define ECC_Grappler ECC_GameTraceChannel1
#define ECC_Interactive ECC_GameTraceChannel2
#define ECC_SphereTracer ECC_GameTraceChannel3
#define ECC_LedgeTracer ECC_GameTraceChannel4
#define ECC_IKFootTrace ECC_GameTraceChannel5
#define ECC_CameraPlaceholder ECC_GameTraceChannel6
#define ECC_WeaponTrace ECC_GameTraceChannel7
#define ECC_WeaponProjectile ECC_GameTraceChannel8
#define ECC_InteractionTrace ECC_GameTraceChannel9
#define ECC_WeaponMelee ECC_GameTraceChannel10
#define ECC_StreamingBounds ECC_GameTraceChannel11
#define ECC_GrappleTrace ECC_GameTraceChannel12
#define ECC_WeaponTracePenetrate ECC_GameTraceChannel13

#define UI_MOVE_GrappleWalk static_cast<uint8>(ECustomMovementMode::MOVE_GrappleWalk)
#define UI_MOVE_GrappleFall static_cast<uint8>(ECustomMovementMode::MOVE_GrappleFall)

USTRUCT(BlueprintType)
struct FPlayerColours
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Colours")
	FLinearColor PrimaryColour;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Colours")
	FLinearColor SecondaryColour;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Colours")
	FLinearColor EmissiveColour;

	FPlayerColours()
	{
		PrimaryColour = FLinearColor::White;
		SecondaryColour = FLinearColor::Gray;
		EmissiveColour = FLinearColor::Black;
	}

	FPlayerColours(const FLinearColor& armour, const FLinearColor& detail, const FLinearColor& visor)
	{
		PrimaryColour = armour;
		SecondaryColour = detail;
		EmissiveColour = visor;
	}
};

USTRUCT()
struct FDecalData
{
	GENERATED_USTRUCT_BODY()

	/** material */
	UPROPERTY(EditDefaultsOnly, Category = Decal)
	UMaterial* DecalMaterial;

	/** quad size (width & height) */
	UPROPERTY(EditDefaultsOnly, Category = Decal)
	float DecalSize;

	/** lifespan */
	UPROPERTY(EditDefaultsOnly, Category = Decal)
	float LifeSpan;

	/** defaults */
	FDecalData()
		: DecalSize(256.f),
		LifeSpan(10.f)
	{
	}
};

/** replicated information on a hit we've taken */
USTRUCT()
struct FTakeHitInfo
{
	GENERATED_USTRUCT_BODY()

	/** The amount of damage actually applied */
	UPROPERTY()
	float ActualDamage;

	/** The damage type we were hit with. */
	UPROPERTY()
	UClass* DamageTypeClass;

	/** Who hit us */
	UPROPERTY()
	TWeakObjectPtr<class AVertCharacterBase> PawnInstigator;

	/** Who actually caused the damage */
	UPROPERTY()
	TWeakObjectPtr<class AActor> DamageCauser;

	/** Specifies which DamageEvent below describes the damage received. */
	UPROPERTY()
	int32 DamageEventClassID;

	/** Rather this was a kill */
	UPROPERTY()
	uint32 bKilled : 1;

private:
	/** A rolling counter used to ensure the struct is dirty and will replicate. */
	UPROPERTY()
	uint8 EnsureReplicationByte;

	/** Describes general damage. */
	UPROPERTY()
	FDamageEvent GeneralDamageEvent;

	/** Describes point damage, if that is what was received. */
	UPROPERTY()
	FPointDamageEvent PointDamageEvent;

	/** Describes radial damage, if that is what was received. */
	UPROPERTY()
	FRadialDamageEvent RadialDamageEvent;

public:
	FTakeHitInfo();

	FDamageEvent& GetDamageEvent();
	void SetDamageEvent(const FDamageEvent& DamageEvent);
	void EnsureReplication();
};