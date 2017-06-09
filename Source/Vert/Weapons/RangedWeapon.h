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

	UPROPERTY(EditDefaultsOnly, Category = Recoil)
	float RecoilAmount;

	/** defaults */
	FRangedWeaponSpreadConfig()
	{
		WeaponSpread = 0.25f;
		MovingSpreadMod = 5.f;
		FiringSpreadIncrement = 1.0f;
		FiringSpreadMax = 10.0f;
		RecoilAmount = 0.f;
	}
};

UCLASS(Abstract, Blueprintable)
class ARangedWeapon : public ABaseWeapon
{
	GENERATED_UCLASS_BODY()

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = Config)
	FRangedWeaponSpreadConfig SpreadConfig;

	/** name of bone/socket for muzzle in weapon mesh */
	UPROPERTY(EditDefaultsOnly, Category = Effects)
	FName MuzzleAttachPoint;

	/** FX for muzzle flash */
	UPROPERTY(EditDefaultsOnly, Category = Effects)
	UParticleSystem* MuzzleFX;

	/** spawned component for muzzle FX */
	UPROPERTY(Transient)
	UParticleSystemComponent* MuzzlePSC;

	/** spawned component for second muzzle FX (Needed for split screen) */
	UPROPERTY(Transient)
	UParticleSystemComponent* MuzzlePSCSecondary;

	/** is muzzle FX looped? */
	UPROPERTY(EditDefaultsOnly, Category = Effects)
	bool LoopedMuzzleFX;

public:
	float GetCurrentSpread() const;

	/** get the muzzle location of the weapon */
	UFUNCTION(BlueprintCallable, Category = "Aim")
	FVector GetMuzzleLocation() const;

	/** get direction of weapon's muzzle */
	UFUNCTION(BlueprintCallable, Category = "Aim")
	FVector GetMuzzleDirection() const;

	/** Get the aim of the weapon, allowing for adjustments to be made by the weapon */
	UFUNCTION(BlueprintCallable, Category = "Aim")
	virtual FVector GetAdjustedAim() const;

protected:	
	virtual FVector GetShootDirectionAfterSpread(const FVector& aimDirection, int32& outRandomSeed, float& outCurrentSpread);
	virtual void OnBurstFinished() override; /** [local + server] update spread on firing */
	virtual void ClientSimulateWeaponAttack_Implementation() override;
	virtual void ClientStopSimulateWeaponAttack_Implementation() override;

protected:
	float mCurrentFiringSpread;
};

