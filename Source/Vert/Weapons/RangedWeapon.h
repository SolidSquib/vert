// Copyright Inside Out Games Ltd. 2017

#pragma once

#include "GameFramework/Actor.h"
#include "WeaponPickup.h"
#include "Interactives/Interactive.h"
#include "RangedWeapon.generated.h"

UCLASS()
class VERT_API ARangedWeapon : public ABaseWeapon
{
	GENERATED_BODY()
	
protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Ranged")
	float MaximumEffectiveRange = 500.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Weapon|Ranged")
	float BulletSpread = 0;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
};
