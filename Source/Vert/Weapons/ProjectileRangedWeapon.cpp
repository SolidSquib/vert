// Copyright Inside Out Games Ltd. 2017

#include "Vert.h"
#include "RangedWeapon.h"
#include "ProjectileRangedWeapon.h"

AProjectileRangedWeapon::AProjectileRangedWeapon(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
}

//////////////////////////////////////////////////////////////////////////
// Weapon usage

void AProjectileRangedWeapon::FireWeapon()
{
	const int32 RandomSeed = FMath::Rand();
	FRandomStream WeaponRandomStream(RandomSeed);
	const float CurrentSpread = GetCurrentSpread();
	const float ConeHalfAngle = FMath::DegreesToRadians(CurrentSpread * 0.5f);

	FVector AimDir = GetAdjustedAim();
	FVector Origin = GetMuzzleLocation();
	const FVector ShootDir = WeaponRandomStream.VRandCone(AimDir, ConeHalfAngle, ConeHalfAngle);
	const FVector ShootDir2D = (FVector(ShootDir.X, 0.f, ShootDir.Z) * 100).GetSafeNormal();

	ServerFireProjectile(Origin, ShootDir2D);
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
