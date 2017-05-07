// Copyright Inside Out Games Ltd. 2017

#include "Vert.h"
#include "HealthComponent.h"

// Sets default values for this component's properties
UHealthComponent::UHealthComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	DamageModifier = 1.f;
}

void UHealthComponent::BeginPlay()
{
	Super::BeginPlay();

	AActor* owner = GetOwner();
	if (ACharacter* character = Cast<ACharacter>(owner))
	{
		mCharacterOwner = character;
	}

	SetDamageTaken(mDamageTaken);
	// #MI_TODO: setup damage types (resistences etc)
}

int32 UHealthComponent::DealDamage(int32 magnitude, TSubclassOf<UDamageType> type = UDamageType::StaticClass(), const FVector& knockbackImpulse)
{
	SetDamageTaken(mDamageTaken + magnitude);
	OnHit.Broadcast();

	return mDamageTaken;
}

int32 UHealthComponent::HealDamage(int32 magnitude)
{
	SetDamageTaken(mDamageTaken - magnitude);
	OnHeal.Broadcast();

	return mDamageTaken;
}

void UHealthComponent::SetDamageTaken(int32 totalDamage)
{
	mDamageTaken = totalDamage;

	if (mCharacterOwner.IsValid() && mCharacterOwner->PlayerState)
	{
		AVertPlayerState* playerState = Cast<AVertPlayerState>(mCharacterOwner->PlayerState);
		if (playerState)
			playerState->SetDamageTaken(mDamageTaken);
	}
}