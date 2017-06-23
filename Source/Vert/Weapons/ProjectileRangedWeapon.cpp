// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "Weapons/ProjectileRangedWeapon.h"
#include "Vert.h"
#include "Weapons/WeaponProjectile.h"

AProjectileRangedWeapon::AProjectileRangedWeapon(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
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

	Delegate_OnWeaponFiredWithRecoil.Broadcast(SpreadConfig.RecoilAmount);

	return true;
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
	FTransform SpawnTM(ShootDir.Rotation(), Origin);
	AWeaponProjectile* Projectile = Cast<AWeaponProjectile>(UGameplayStatics::BeginDeferredActorSpawnFromClass(this, ProjectileConfig.ProjectileClass, SpawnTM));
	if (Projectile)
	{
		Projectile->Instigator = Instigator;
		Projectile->SetOwner(this);
		Projectile->InitVelocity(ShootDir);

		UGameplayStatics::FinishSpawningActor(Projectile, SpawnTM);
	}
}

//************************************
// Method:    ApplyWeaponConfig
// FullName:  AProjectileRangedWeapon::ApplyWeaponConfig
// Access:    public 
// Returns:   void
// Qualifier:
// Parameter: FProjectileWeaponData & Data
//************************************
void AProjectileRangedWeapon::ApplyWeaponConfig(FProjectileWeaponData& Data)
{
	Data = ProjectileConfig;
}

//************************************
// Method:    GetWeaponType_Implementation
// FullName:  AProjectileRangedWeapon::GetWeaponType_Implementation
// Access:    virtual protected 
// Returns:   UClass*
// Qualifier: const
//************************************
UClass* AProjectileRangedWeapon::GetWeaponType_Implementation() const
{
	return AProjectileRangedWeapon::StaticClass();
}