#include "RangedWeapon.h"
#include "Vert.h"

ARangedWeapon::ARangedWeapon(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	mCurrentFiringSpread = 0.0f;
	LoopedMuzzleFX = false;
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

void ARangedWeapon::ClientSimulateWeaponAttack_Implementation()
{
	Super::ClientSimulateWeaponAttack_Implementation();

	if (MuzzleFX)
	{
		if (!LoopedMuzzleFX || MuzzlePSC == NULL)
		{
			MuzzlePSC = UGameplayStatics::SpawnEmitterAttached(MuzzleFX, WeaponMesh, MuzzleAttachPoint);
		}
	}
}

void ARangedWeapon::ClientStopSimulateWeaponAttack_Implementation()
{
	Super::ClientStopSimulateWeaponAttack_Implementation();

	if (LoopedMuzzleFX)
	{
		if (MuzzlePSC != NULL)
		{
			MuzzlePSC->DeactivateSystem();
			MuzzlePSC = NULL;
		}
		if (MuzzlePSCSecondary != NULL)
		{
			MuzzlePSCSecondary->DeactivateSystem();
			MuzzlePSCSecondary = NULL;
		}
	}
}

FVector ARangedWeapon::GetMuzzleLocation() const
{
	return WeaponMesh->GetSocketLocation(MuzzleAttachPoint);
}

FVector ARangedWeapon::GetMuzzleDirection() const
{
	return WeaponMesh->GetSocketRotation(MuzzleAttachPoint).Vector();
}

FVector ARangedWeapon::GetAdjustedAim() const
{
	if (UseControllerAim)
	{
		if (AVertCharacter* character = Cast<AVertCharacter>(Instigator))
		{
			return character->GetActorForwardVector().GetSafeNormal();
		}

		UE_LOG(LogVertBaseWeapon, Warning, TEXT("Unable to get owning character of weapon %s"), *GetName());
	}

	return GetMuzzleDirection();
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
