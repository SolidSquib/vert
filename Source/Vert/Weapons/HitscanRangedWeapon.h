// Copyright Inside Out Games Ltd. 2017

#pragma once

#include "GameFramework/Actor.h"
#include "RangedWeapon.h"
#include "Interactives/Interactive.h"
#include "HitscanRangedWeapon.generated.h"

UCLASS()
class VERT_API AHitscanRangedWeapon : public ARangedWeapon
{
	GENERATED_BODY()

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|FX")
	UParticleSystem* TrailFX;

protected:
	void ProcessInstantHit(const FHitResult& Impact, const FVector& Origin, const FVector& ShootDir, int32 RandomSeed, float ReticleSpread)

	virtual void BeginPlay() override;
	virtual void ExecuteAttack_Implementation() override;
};
