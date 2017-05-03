// Copyright Inside Out Games Ltd. 2017

#pragma once

#include "GameFramework/Actor.h"
#include "BaseWeapon.h"
#include "MeleeWeapon.generated.h"

UCLASS()
class VERT_API AMeleeWeapon : public ABaseWeapon
{
	GENERATED_BODY()

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
};
