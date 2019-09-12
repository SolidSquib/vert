// Copyright Inside Out Games Ltd. 2017

#pragma once

#include "PooledActor.h"
#include "PooledProjectile.h"
#include "ProjectileRangedWeapon.h"
#include "WeaponProjectile.generated.h"

// 
UCLASS(Abstract, Blueprintable)
class AWeaponProjectile : public APooledActor
{
	GENERATED_UCLASS_BODY()

protected:
	/** did it explode? */
	UPROPERTY(Transient, ReplicatedUsing = OnRep_Exploded)
	bool bExploded;

	/** effects for explosion */
	UPROPERTY(EditDefaultsOnly, Category = Effects)
	TSubclassOf<class AVertExplosionEffect> ExplosionTemplate;

private:
	/** movement component */
	UPROPERTY(VisibleDefaultsOnly, Category = Projectile)
	UProjectileMovementComponent* MovementComp;

	/** collisions */
	UPROPERTY(VisibleDefaultsOnly, Category = Projectile)
	USphereComponent* CollisionComp;

public:
	/** setup velocity */
	void InitVelocity(const FVector& ShootDirection);
	void InitProjectile(const FProjectileWeaponData& weaponData, int32 baseDamage, float baseKnockback, float knockbackScaling, float stunTime);

	virtual void PostInitializeComponents() override;
	
	UFUNCTION(BlueprintNativeEvent) /** handle hit */
	void OnImpact(const FHitResult& HitResult);

	UFUNCTION(BlueprintNativeEvent)
	void OnCollisionHit(UPrimitiveComponent* hitComponent, AActor* otherActor, UPrimitiveComponent* otherComp, FVector normalImpulse, const FHitResult& hit);

protected:
	/** trigger explosion */
	void Explode(const FHitResult& Impact);
	void ApplyPointDamage(const FHitResult& impact);

	/** shutdown projectile and prepare for destruction */
	void DisableAndDestroy();

	/** update velocity on client */
	virtual void PostNetReceiveVelocity(const FVector& NewVelocity) override;
	virtual void LifeSpanExpired() override;

	FORCEINLINE UProjectileMovementComponent* GetMovementComp() const { return MovementComp; }
	FORCEINLINE USphereComponent* GetCollisionComp() const { return CollisionComp; }

	/** [client] explosion happened */
	UFUNCTION()
	void OnRep_Exploded();

	UFUNCTION()
	void PoolBeginPlay();

	UFUNCTION()
	void PoolEndPlay();

protected:
	/** controller that fired me (cache for damage calculations) */
	TWeakObjectPtr<AController> mController;

	/** projectile data */
	struct FProjectileWeaponData mWeaponConfig;
};
