// Copyright Inside Out Games Ltd. 2017

#pragma once

#include "GameFramework/Actor.h"
#include "ProjectileRangedWeapon.h"
#include "WeaponProjectile.generated.h"

// 
UCLASS(Abstract, Blueprintable)
class AWeaponProjectile : public AActor
{
	GENERATED_UCLASS_BODY()

protected:
	/** did it explode? */
	UPROPERTY(Transient, ReplicatedUsing = OnRep_Exploded)
	bool bExploded;

	/** effects for explosion */
	UPROPERTY(EditDefaultsOnly, Category = Effects)
	TSubclassOf<class AVertExplosionEffect> ExplosionTemplate;

	UPROPERTY(EditDefaultsOnly, Category = Damage)
	float BaseDamage = 10.f;

private:
	/** movement component */
	UPROPERTY(VisibleDefaultsOnly, Category = Projectile)
	UProjectileMovementComponent* MovementComp;

	/** collisions */
	UPROPERTY(VisibleDefaultsOnly, Category = Projectile)
	USphereComponent* CollisionComp;

	UPROPERTY(VisibleDefaultsOnly, Category = Projectile)
	UParticleSystemComponent* ParticleComp;

public:
	/** setup velocity */
	void InitVelocity(FVector& ShootDirection);

	virtual void PostInitializeComponents() override;
	
	UFUNCTION(BlueprintNativeEvent) /** handle hit */
	void OnImpact(const FHitResult& HitResult);

protected:
	/** trigger explosion */
	void Explode(const FHitResult& Impact);
	void ApplyPointDamage(const FHitResult& impact);

	/** shutdown projectile and prepare for destruction */
	void DisableAndDestroy();

	/** update velocity on client */
	virtual void PostNetReceiveVelocity(const FVector& NewVelocity) override;

	FORCEINLINE UProjectileMovementComponent* GetMovementComp() const { return MovementComp; }
	FORCEINLINE USphereComponent* GetCollisionComp() const { return CollisionComp; }
	FORCEINLINE UParticleSystemComponent* GetParticleComp() const { return ParticleComp; }

	/** [client] explosion happened */
	UFUNCTION()
	void OnRep_Exploded();

protected:
	/** controller that fired me (cache for damage calculations) */
	TWeakObjectPtr<AController> mController;

	/** projectile data */
	struct FProjectileWeaponData mWeaponConfig;
};
