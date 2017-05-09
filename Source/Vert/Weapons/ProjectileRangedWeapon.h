// Copyright Inside Out Games Ltd. 2017

#pragma once

#include "RangedWeapon.h"
#include "ProjectileRangedWeapon.generated.h"

USTRUCT()
struct FProjectileWeaponData
{
	GENERATED_USTRUCT_BODY()

	/** projectile class */
	UPROPERTY(EditDefaultsOnly, Category = Projectile)
	TSubclassOf<class AWeaponProjectile> ProjectileClass;

	/** life time */
	UPROPERTY(EditDefaultsOnly, Category = Projectile)
	float ProjectileLife;

	/** damage at impact point */
	UPROPERTY(EditDefaultsOnly, Category = WeaponStat)
	int32 ExplosionDamage;

	/** radius of damage */
	UPROPERTY(EditDefaultsOnly, Category = WeaponStat)
	float ExplosionRadius;

	/** type of damage */
	UPROPERTY(EditDefaultsOnly, Category = WeaponStat)
	TSubclassOf<UDamageType> DamageType;

	/** defaults */
	FProjectileWeaponData()
	{
		ProjectileClass = NULL;
		ProjectileLife = 10.0f;
		ExplosionDamage = 100;
		ExplosionRadius = 300.0f;
		DamageType = UDamageType::StaticClass();
	}
};

// A weapon that fires a visible projectile
UCLASS(Abstract)
class AProjectileRangedWeapon : public ARangedWeapon
{
	GENERATED_UCLASS_BODY()

protected:
	/** weapon config */
	UPROPERTY(EditDefaultsOnly, Category = Config)
	FProjectileWeaponData ProjectileConfig;

public:
	void ApplyWeaponConfig(FProjectileWeaponData& Data); /** apply config on projectile */

protected:
	virtual EAmmoType GetAmmoType() const override
	{
		return EAmmoType::ERocket;
	}

	virtual void FireWeapon() override; /** [local] weapon specific fire implementation */

	UFUNCTION(reliable, server, WithValidation) /** spawn projectile on server */
	void ServerFireProjectile(FVector Origin, FVector_NetQuantizeNormal ShootDir);
};
