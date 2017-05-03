// Copyright Inside Out Games Ltd. 2017

#pragma once

#include "WeaponPickup.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UWeaponPickup : public UInterface
{
	GENERATED_UINTERFACE_BODY()

//public:
//	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Test")
//	int32 testproperty = 0;
};

/**
 * 
 */
class VERT_API IWeaponPickup
{
	GENERATED_IINTERFACE_BODY()

public:
	virtual void Attack() = 0;
	virtual void StopAttacking() = 0;

	UFUNCTION(BlueprintImplementableEvent, Category = "Weapon|Throw")
	void OnCatch(class APawn* newOwner);

	UFUNCTION(BlueprintImplementableEvent, Category = "Weapon|Throw")
	void OnThrow(class APawn* owner);

	UFUNCTION(BlueprintImplementableEvent, Category = "Weapon|Throw")
	void OnImpact(class APawn* owner, class AActor* hitActor);

protected:
	bool mIsAttacking = false;
	int32 mChargeLevel = 0;
	TWeakObjectPtr<class UCharacterInteractionComponent> mCharacterInteractionOwner = nullptr;
};
