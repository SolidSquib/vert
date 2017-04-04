// Copyright Inside Out Games Ltd. 2017

#pragma once

#include "GameFramework/Actor.h"
#include "BaseWeapon.h"
#include "MeleeWeapon.generated.h"

UCLASS()
class VERT_API AMeleeWeapon : public ABaseWeapon
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AMeleeWeapon();

	// Called every frame
	virtual void Tick(float DeltaTime) override;
	virtual void Attack() override;
	virtual void Interact(TWeakObjectPtr<class UCharacterInteractionComponent> instigator) override;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	virtual void Throw() override;
};
