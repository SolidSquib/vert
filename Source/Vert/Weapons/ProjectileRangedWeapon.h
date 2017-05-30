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
	
	UPROPERTY(EditDefaultsOnly, Category = WeaponStat)
	bool IsExplosive;

	/** radius of damage */
	UPROPERTY(EditDefaultsOnly, Category = WeaponStat, meta = (EditCondition = "IsExplosive"))
	float ExplosionRadius;

	/** type of damage */
	UPROPERTY(EditDefaultsOnly, Category = WeaponStat)
	TSubclassOf<UDamageType> DamageType;

	/** defaults */
	FProjectileWeaponData()
	{
		ProjectileClass = NULL;
		ProjectileLife = 10.0f;
		IsExplosive = true;
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
	virtual bool FireWeapon_Implementation() override; /** [local] weapon specific fire implementation */

	/** spawn projectile on server */
	UFUNCTION(BlueprintCallable, reliable, server, WithValidation)
	void ServerFireProjectile(FVector Origin, FVector_NetQuantizeNormal ShootDir);
};