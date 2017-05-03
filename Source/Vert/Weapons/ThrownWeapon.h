// Copyright Inside Out Games Ltd. 2017

#pragma once

#include "GameFramework/Actor.h"
#include "RangedWeapon.h"
#include "Interactives/Interactive.h"
#include "ThrownWeapon.generated.h"

UCLASS()
class VERT_API AThrownWeapon : public ARangedWeapon
{
	GENERATED_BODY()
		
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
};
