// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "Weapons/ProjectileRangedWeapon.h"
#include "Vert.h"
#include "Weapons/WeaponProjectile.h"

DECLARE_LOG_CATEGORY_CLASS(LogVertProjectileRangedWeapon, Log, All);

AProjectileRangedWeapon::AProjectileRangedWeapon(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer),
	ProjectileClass(AWeaponProjectile::StaticClass())
{
}

//////////////////////////////////////////////////////////////////////////
// Weapon usage

//************************************
// Method:    AttackWithWeapon_Implementation
// FullName:  AProjectileRangedWeapon::AttackWithWeapon_Implementation
// Access:    virtual protected 
// Returns:   bool
// Qualifier:
//************************************
bool AProjectileRangedWeapon::AttackWithWeapon_Implementation()
{
	int32 randomSeed;
	float currentSpread;

	FVector AimDir = GetAdjustedAim();
	const FVector ShootDir = GetShootDirectionAfterSpread(AimDir, randomSeed, currentSpread);

	ServerFireProjectile(GetMuzzleLocation(), ShootDir);

	mCurrentFiringSpread = FMath::Min(SpreadConfig.FiringSpreadMax, mCurrentFiringSpread + SpreadConfig.FiringSpreadIncrement);

	WeaponAttackFire(SpreadConfig.RecoilAmount);

	return true;
}

//************************************
// Method:    GetProjectileClass
// FullName:  AProjectileRangedWeapon::GetProjectileClass
// Access:    public 
// Returns:   TSubclassOf<AWeaponProjectile>
// Qualifier: const
//************************************
TSubclassOf<AWeaponProjectile> AProjectileRangedWeapon::GetProjectileClass() const
{
	return ProjectileClass;
}

//************************************
// Method:    ServerFireProjectile_Validate
// FullName:  AProjectileRangedWeapon::ServerFireProjectile_Validate
// Access:    public 
// Returns:   bool
// Qualifier:
// Parameter: FVector Origin
// Parameter: FVector_NetQuantizeNormal ShootDir
//************************************
bool AProjectileRangedWeapon::ServerFireProjectile_Validate(FVector Origin, FVector_NetQuantizeNormal ShootDir)
{
	return true;
}

//************************************
// Method:    ServerFireProjectile_Implementation
// FullName:  AProjectileRangedWeapon::ServerFireProjectile_Implementation
// Access:    public 
// Returns:   void
// Qualifier:
// Parameter: FVector Origin
// Parameter: FVector_NetQuantizeNormal ShootDir
//************************************
void AProjectileRangedWeapon::ServerFireProjectile_Implementation(FVector Origin, FVector_NetQuantizeNormal ShootDir)
{
	if (ProjectileClass == nullptr)
	{
		UE_LOG(LogVertProjectileRangedWeapon, Warning, TEXT("Projectile class not set in weapon %s."), *GetClass()->GetName());
		return;
	}

	FTransform SpawnTM(ShootDir.Rotation(), Origin);

	if (AVertGameMode* gm = GetWorld()->GetAuthGameMode<AVertGameMode>())
	{
		if (AWeaponProjectile* Projectile = gm->RequestProjectileSpawn(ProjectileClass, SpawnTM, this))
		{
			Projectile->SetOwner(this);
			Projectile->Instigator = Instigator;
			Projectile->InitProjectile(ProjectileConfig, WeaponConfig.BaseDamage, WeaponConfig.BaseKnockback, WeaponConfig.KnockbackScaling, WeaponConfig.StunTime);
			Projectile->InitVelocity(ShootDir);
		}
		else
		{
			UE_LOG(LogVertProjectileRangedWeapon, Error, TEXT("Projectile pool for class %s does not exist in game mode."), *ProjectileClass->GetName())
		}
	}
}
