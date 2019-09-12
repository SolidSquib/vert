#include "RangedWeapon.h"
#include "Vert.h"

ARangedWeapon::ARangedWeapon(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	mCurrentFiringSpread = 0.0f;
	LoopedMuzzleFX = false;
}

//////////////////////////////////////////////////////////////////////////
// Weapon usage helpers

//************************************
// Method:    GetCurrentSpread
// FullName:  ARangedWeapon::GetCurrentSpread
// Access:    public 
// Returns:   float
// Qualifier: const
//************************************
float ARangedWeapon::GetCurrentSpread() const
{
	float finalSpread = SpreadConfig.WeaponSpread + mCurrentFiringSpread;
	if (MyPawn && MyPawn->IsMoving())
	{
		finalSpread *= SpreadConfig.MovingSpreadMod;
	}

	return finalSpread;
}

//************************************
// Method:    OnBurstFinished
// FullName:  ARangedWeapon::OnBurstFinished
// Access:    virtual protected 
// Returns:   void
// Qualifier:
//************************************
void ARangedWeapon::OnBurstFinished()
{
	Super::OnBurstFinished();

	mCurrentFiringSpread = 0.0f;
}

//************************************
// Method:    ClientSimulateWeaponAttack_Implementation
// FullName:  ARangedWeapon::ClientSimulateWeaponAttack_Implementation
// Access:    virtual protected 
// Returns:   void
// Qualifier:
//************************************
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

//************************************
// Method:    ClientStopSimulateWeaponAttack_Implementation
// FullName:  ARangedWeapon::ClientStopSimulateWeaponAttack_Implementation
// Access:    virtual protected 
// Returns:   void
// Qualifier:
//************************************
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

//************************************
// Method:    GetMuzzleLocation
// FullName:  ARangedWeapon::GetMuzzleLocation
// Access:    public 
// Returns:   FVector
// Qualifier: const
//************************************
FVector ARangedWeapon::GetMuzzleLocation() const
{
	return WeaponMesh->GetSocketLocation(MuzzleAttachPoint);
}

//************************************
// Method:    GetMuzzleDirection
// FullName:  ARangedWeapon::GetMuzzleDirection
// Access:    public 
// Returns:   FVector
// Qualifier: const
//************************************
FVector ARangedWeapon::GetMuzzleDirection() const
{
	FVector muzzleDirection = WeaponMesh->GetSocketRotation(MuzzleAttachPoint).Vector();
	muzzleDirection.Y = 0;
	return muzzleDirection.GetSafeNormal();
}

//************************************
// Method:    GetAdjustedAim
// FullName:  ARangedWeapon::GetAdjustedAim
// Access:    virtual public 
// Returns:   FVector
// Qualifier: const
//************************************
FVector ARangedWeapon::GetAdjustedAim() const
{
	if (UseControllerAim)
	{
		if (AVertCharacter* character = Cast<AVertCharacter>(Instigator))
		{
			FVector direction = character->GetActorForwardVector().GetSafeNormal();
			direction.Y = 0;
			return direction;
		}

		UE_LOG(LogVertBaseWeapon, Warning, TEXT("Unable to get owning character of weapon %s"), *GetName());
	}

	return GetMuzzleDirection();
}

void ARangedWeapon::StartReload(bool bFromReplication /*= false*/)
{
	Super::StartReload(bFromReplication);

	if (ReloadSound)
	{
		UAkGameplayStatics::PostEvent(ReloadSound, GetPawnOwner(), false);
	}
}

//************************************
// Method:    GetShootDirectionAfterSpread
// FullName:  ARangedWeapon::GetShootDirectionAfterSpread
// Access:    virtual protected 
// Returns:   FVector
// Qualifier:
// Parameter: const FVector & aimDirection
// Parameter: int32 & outRandomSeed
// Parameter: float & outCurrentSpread
//************************************
FVector ARangedWeapon::GetShootDirectionAfterSpread(const FVector& aimDirection, int32& outRandomSeed, float& outCurrentSpread)
{
	outRandomSeed = FMath::Rand();
	FRandomStream WeaponRandomStream(outRandomSeed);
	outCurrentSpread = GetCurrentSpread();
	const float ConeHalfAngle = FMath::DegreesToRadians(outCurrentSpread * 0.5f);

	FVector newDirection = WeaponRandomStream.VRandCone(aimDirection, ConeHalfAngle, ConeHalfAngle);
	newDirection.Y = 0.f;

	return newDirection;
}

//************************************
// Method:    GetWeaponType_Implementation
// FullName:  ARangedWeapon::GetWeaponType_Implementation
// Access:    virtual protected 
// Returns:   UClass*
// Qualifier: const
//************************************
UClass* ARangedWeapon::GetWeaponType_Implementation() const
{
	return ARangedWeapon::StaticClass();
}