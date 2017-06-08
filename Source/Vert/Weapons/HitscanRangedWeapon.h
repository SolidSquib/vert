// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Vert.h"
#include "HitscanRangedWeapon.generated.h"

class AVertImpactEffect;

USTRUCT()
struct FInstantHitInfo
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	FVector Origin;

	UPROPERTY()
	float ReticleSpread;

	UPROPERTY()
	int32 RandomSeed;
};

USTRUCT()
struct FInstantWeaponData
{
	GENERATED_USTRUCT_BODY()

	/** weapon range */
	UPROPERTY(EditDefaultsOnly, Category = WeaponStat)
	float WeaponRange;

	/** hit verification: scale for bounding box of hit actor */
	UPROPERTY(EditDefaultsOnly, Category = HitVerification)
	float ClientSideHitLeeway;

	/** hit verification: threshold for dot product between view direction and hit direction */
	UPROPERTY(EditDefaultsOnly, Category = HitVerification)
	float AllowedViewDotHitDir;

	/** defaults */
	FInstantWeaponData()
	{
		WeaponRange = 10000.0f;
		ClientSideHitLeeway = 200.0f;
		AllowedViewDotHitDir = 0.8f;
	}
};

// A weapon where the damage impact occurs instantly upon firing
UCLASS(Abstract, Blueprintable)
class AHitscanRangedWeapon : public ARangedWeapon
{
	GENERATED_UCLASS_BODY()

protected:
	/** weapon config */
	UPROPERTY(EditDefaultsOnly, Category = Config)
	FInstantWeaponData InstantConfig;

	/** impact effects */
	UPROPERTY(EditDefaultsOnly, Category = Effects)
	TSubclassOf<AVertImpactEffect> ImpactTemplate;

	/** smoke trail */
	UPROPERTY(EditDefaultsOnly, Category = Effects)
	UParticleSystem* TrailFX;

	/** param name for beam target in smoke trail */
	UPROPERTY(EditDefaultsOnly, Category = Effects)
	FName TrailTargetParam;

	/** instant hit notify for replication */
	UPROPERTY(Transient, ReplicatedUsing = OnRep_HitNotify)
	FInstantHitInfo HitNotify;


protected:
	void ProcessInstantHit_Confirmed(const FHitResult& Impact, const FVector& Origin, const FVector& ShootDir, int32 RandomSeed, float ReticleSpread); /** continue processing the instant hit, as if it has been confirmed by the server */
	bool ShouldDealDamage(AActor* TestActor) const; /** check if weapon should deal damage to actor */
	void DealDamage(const FHitResult& Impact, const FVector& ShootDir);	/** handle damage */
	void SimulateInstantHit(const FVector& Origin, int32 RandomSeed, float ReticleSpread); /** called in network play to do the cosmetic fx  */
	void SpawnImpactEffects(const FHitResult& Impact); /** spawn effects for impact */
	void SpawnTrailEffect(const FVector& EndPoint); /** spawn trail effect */
	virtual bool FireWeapon_Implementation() override; /** [local] weapon specific fire implementation */

	UFUNCTION()
	void OnRep_HitNotify();

	UFUNCTION(BlueprintCallable, Category = "Attack")
	void ProcessInstantHit(const FHitResult& Impact, const FVector& Origin, const FVector& ShootDir, int32 RandomSeed, float ReticleSpread); /** process the instant hit and notify the server if necessary */

	/** server notified of hit from client to verify */
	UFUNCTION(reliable, server, WithValidation)
	void ServerNotifyHit(const FHitResult& Impact, FVector_NetQuantizeNormal ShootDir, int32 RandomSeed, float ReticleSpread);

	/** server notified of miss to show trail FX */
	UFUNCTION(unreliable, server, WithValidation)
	void ServerNotifyMiss(FVector_NetQuantizeNormal ShootDir, int32 RandomSeed, float ReticleSpread);
};
