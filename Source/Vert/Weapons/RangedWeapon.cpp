// Copyright Inside Out Games Ltd. 2017

#include "Vert.h"
#include "RangedWeapon.h"

// Called when the game starts or when spawned
void ARangedWeapon::BeginPlay()
{
	Super::BeginPlay();
}

float ARangedWeapon::GetCurrentSpread() const
{
	float FinalSpread = WeaponConfig.WeaponSpread + mCurrentFiringSpread;
	float velocitySqr = mCharacterInteractionOwner->GetCharacterOwner()->GetVelocity().SizeSquared();
	if (velocitySqr > KINDA_SMALL_NUMBER)
	{
		FinalSpread *= WeaponConfig.MovingSpreadMod;
	}

	return FinalSpread;
}

FVector ARangedWeapon::GetMuzzleLocation() const
{
	if (Sprite)
	{
		return Sprite->GetSocketTransform(MuzzleSocketName).GetLocation();
	}

	return FVector::ZeroVector;
}

void  ARangedWeapon::FireRangedWeapon(const FVector& muzzleLocation, const FVector& shootDirection)
{

}

FHitResult ARangedWeapon::WeaponTrace(const FVector& TraceFrom, const FVector& TraceTo) const
{
	static FName WeaponFireTag = FName(TEXT("WeaponTrace"));

	// Perform trace to retrieve hit info
	FCollisionQueryParams TraceParams(WeaponFireTag, true, Instigator);
	TraceParams.bTraceAsyncScene = true;
	TraceParams.bReturnPhysicalMaterial = true;

	FHitResult Hit(ForceInit);
	GetWorld()->LineTraceSingleByChannel(Hit, TraceFrom, TraceTo, ECC_WeaponTrace, TraceParams);

	return Hit;
}

void ARangedWeapon::ExecuteAttack_Implementation()
{
	
}