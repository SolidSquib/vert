// Copyright Inside Out Games Ltd. 2017

#include "Vert.h"
#include "MeleeWeapon.h"

DECLARE_LOG_CATEGORY_CLASS(LogVertMeleeWeapon, Log, All);

// Sets default values
AMeleeWeapon::AMeleeWeapon()
{
 	
}

// Called when the game starts or when spawned
void AMeleeWeapon::BeginPlay()
{
	Super::BeginPlay();
}

// Called every frame
void AMeleeWeapon::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AMeleeWeapon::Attack()
{
	Super::Attack();
}

void AMeleeWeapon::Interact(TWeakObjectPtr<UCharacterInteractionComponent> instigator)
{
	Super::Interact(instigator);
}

void AMeleeWeapon::Throw()
{
	Super::Throw();
}