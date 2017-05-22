// Copyright Inside Out Games Ltd. 2017

#pragma once

#include "GameFramework/Actor.h"
#include "Interactives/Interactive.h"
#include "PaperFlipbook.h"
#include "RangedWeapon.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogRangedWeapon, Log, All);

USTRUCT()
struct FRangedWeaponSpreadConfig
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

	/** defaults */
	FRangedWeaponSpreadConfig()
	{
		WeaponSpread = 0.25f;
		MovingSpreadMod = 5.f;
		FiringSpreadIncrement = 1.0f;
		FiringSpreadMax = 10.0f;
	}
};

UCLASS(Abstract, Blueprintable)
class ARangedWeapon : public ABaseWeapon
{
	GENERATED_UCLASS_BODY()

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = Config)
	FRangedWeaponSpreadConfig SpreadConfig;

public:
	float GetCurrentSpread() const;

protected:	
	virtual FVector GetShootDirectionAfterSpread(const FVector& aimDirection, int32& outRandomSeed, float& outCurrentSpread);
	virtual void OnBurstFinished() override; /** [local + server] update spread on firing */

protected:
	float mCurrentFiringSpread;
};

