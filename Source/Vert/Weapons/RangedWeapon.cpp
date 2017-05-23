#include "Vert.h"
#include "RangedWeapon.h"

ARangedWeapon::ARangedWeapon(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	mCurrentFiringSpread = 0.0f;
}

//////////////////////////////////////////////////////////////////////////
// Weapon usage helpers

float ARangedWeapon::GetCurrentSpread() const
{
	float finalSpread = SpreadConfig.WeaponSpread + mCurrentFiringSpread;
	if (MyPawn && MyPawn->IsMoving())
	{
		finalSpread *= SpreadConfig.MovingSpreadMod;
	}

	return finalSpread;
}

void ARangedWeapon::OnBurstFinished()
{
	Super::OnBurstFinished();

	mCurrentFiringSpread = 0.0f;
}

FVector ARangedWeapon::GetShootDirectionAfterSpread(const FVector& aimDirection, int32& outRandomSeed, float& outCurrentSpread)
{
	outRandomSeed = FMath::Rand();
	FRandomStream WeaponRandomStream(outRandomSeed);
	outCurrentSpread = GetCurrentSpread();
	const float ConeHalfAngle = FMath::DegreesToRadians(outCurrentSpread * 0.5f);

#if 1
	FVector newDirection = WeaponRandomStream.VRandCone(aimDirection, ConeHalfAngle, ConeHalfAngle);
	newDirection.Y = 0.f;

	return newDirection;
#else
	return WeaponRandomStream.VRandCone(aimDirection, ConeHalfAngle, ConeHalfAngle);
#endif
}
