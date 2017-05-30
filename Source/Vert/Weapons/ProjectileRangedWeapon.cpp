// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "Vert.h"
#include "Weapons/ProjectileRangedWeapon.h"
#include "Weapons/WeaponProjectile.h"

AProjectileRangedWeapon::AProjectileRangedWeapon(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
}

//////////////////////////////////////////////////////////////////////////
// Weapon usage

bool AProjectileRangedWeapon::FireWeapon_Implementation()
{
	int32 randomSeed;
	float currentSpread;

	FVector AimDir = GetAdjustedAim();
	const FVector ShootDir = GetShootDirectionAfterSpread(AimDir, randomSeed, currentSpread);

	ServerFireProjectile(GetMuzzleLocation(), ShootDir);

	mCurrentFiringSpread = FMath::Min(SpreadConfig.FiringSpreadMax, mCurrentFiringSpread + SpreadConfig.FiringSpreadIncrement);

	if (MyPawn)
	{
		MyPawn->OnWeaponFiredWithRecoil.Broadcast(SpreadConfig.RecoilAmount);
	}

	return true;
}

bool AProjectileRangedWeapon::ServerFireProjectile_Validate(FVector Origin, FVector_NetQuantizeNormal ShootDir)
{
	return true;
}

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

void AProjectileRangedWeapon::ApplyWeaponConfig(FProjectileWeaponData& Data)
{
	Data = ProjectileConfig;
}
