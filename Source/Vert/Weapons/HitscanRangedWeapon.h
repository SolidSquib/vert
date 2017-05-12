// Copyright Inside Out Games Ltd. 2017

#pragma once

#include "GameFramework/Actor.h"
#include "RangedWeapon.h"
#include "Interactives/Interactive.h"
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

	/** base weapon spread (degrees) */
	UPROPERTY(EditDefaultsOnly, Category = Accuracy)
	float WeaponSpread;

	/** targeting spread modifier */
	UPROPERTY(EditDefaultsOnly, Category = Accuracy)
	float MovingSpreadMod;

	/** continuous firing: spread increment */
	UPROPERTY(EditDefaultsOnly, Category = Accuracy)
	float FiringSpreadIncrement;

	/** continuous firing: max increment */
	UPROPERTY(EditDefaultsOnly, Category = Accuracy)
	float FiringSpreadMax;

	/** weapon range */
	UPROPERTY(EditDefaultsOnly, Category = WeaponStat)
	float WeaponRange;

	/** damage amount */
	UPROPERTY(EditDefaultsOnly, Category = WeaponStat)
	int32 HitDamage;

	/** type of damage */
	UPROPERTY(EditDefaultsOnly, Category = WeaponStat)
	TSubclassOf<UDamageType> DamageType;

	/** hit verification: scale for bounding box of hit actor */
	UPROPERTY(EditDefaultsOnly, Category = HitVerification)
	float ClientSideHitLeeway;

	/** hit verification: threshold for dot product between view direction and hit direction */
	UPROPERTY(EditDefaultsOnly, Category = HitVerification)
	float AllowedViewDotHitDir;

	/** defaults */
	FInstantWeaponData()
	{
		WeaponSpread = 5.0f;
		MovingSpreadMod = 0.25f;
		FiringSpreadIncrement = 1.0f;
		FiringSpreadMax = 10.0f;
		WeaponRange = 10000.0f;
		HitDamage = 10;
		DamageType = UDamageType::StaticClass();
		ClientSideHitLeeway = 200.0f;
		AllowedViewDotHitDir = 0.8f;
	}
};

// A weapon where the damage impact occurs instantly upon firing
UCLASS(Abstract)
class AHitscanRangedWeapon : public ARangedWeapon
{
	GENERATED_UCLASS_BODY()

protected:
	UPROPERTY(EditDefaultsOnly, Category = Config) /** weapon config */
	FInstantWeaponData InstantConfig;

	UPROPERTY(EditDefaultsOnly, Category = Effects) /** impact effects */
	TSubclassOf<AVertImpactEffect> ImpactTemplate;

	UPROPERTY(EditDefaultsOnly, Category = Effects) /** smoke trail */
	UParticleSystem* TrailFX;

	UPROPERTY(EditDefaultsOnly, Category = Effects) /** param name for beam target in smoke trail */
	FName TrailTargetParam;

	UPROPERTY(Transient, ReplicatedUsing = OnRep_HitNotify) /** instant hit notify for replication */
	FInstantHitInfo HitNotify;

public:
	float GetCurrentSpread() const; /** get current spread */

protected:	
	void SimulateInstantHit(const FVector& Origin, int32 RandomSeed, float ReticleSpread); /** called in network play to do the cosmetic fx  */	
	void SpawnImpactEffects(const FHitResult& Impact); /** spawn effects for impact */	
	void SpawnTrailEffect(const FVector& EndPoint); /** spawn trail effect */	
	void ProcessInstantHit(const FHitResult& Impact, const FVector& Origin, const FVector& ShootDir, int32 RandomSeed, float ReticleSpread); /** process the instant hit and notify the server if necessary */	
	void ProcessInstantHit_Confirmed(const FHitResult& Impact, const FVector& Origin, const FVector& ShootDir, int32 RandomSeed, float ReticleSpread); /** continue processing the instant hit, as if it has been confirmed by the server */	
	void DealDamage(const FHitResult& Impact, const FVector& ShootDir); /** handle damage */	
	bool ShouldDealDamage(AActor* TestActor) const; /** check if weapon should deal damage to actor */

	virtual void FireWeapon() override; /** [local] weapon specific fire implementation */	
	virtual void OnBurstFinished() override; /** [local + server] update spread on firing */
	virtual EAmmoType GetAmmoType() const override
	{
		return EAmmoType::EBullet;
	}
	
	UFUNCTION(reliable, server, WithValidation) /** server notified of hit from client to verify */
	void ServerNotifyHit(const FHitResult& Impact, FVector_NetQuantizeNormal ShootDir, int32 RandomSeed, float ReticleSpread);

	UFUNCTION(unreliable, server, WithValidation) /** server notified of miss to show trail FX */
	void ServerNotifyMiss(FVector_NetQuantizeNormal ShootDir, int32 RandomSeed, float ReticleSpread);

	UFUNCTION()
	void OnRep_HitNotify();	

protected:
	float mCurrentFiringSpread; /** current spread from continuous firing */
};
