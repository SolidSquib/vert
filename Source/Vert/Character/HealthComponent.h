// Copyright Inside Out Games Ltd. 2017

#pragma once

#include "Components/ActorComponent.h"
#include "GameFramework/DamageType.h"
#include "HealthComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnHitDelegate);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnHealDelegate);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class VERT_API UHealthComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable, Category = "Character|Health")
	FOnHitDelegate OnHit;

	UPROPERTY(BlueprintAssignable, Category = "Character|Health")
	FOnHealDelegate OnHeal;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character|Health")
	float DamageModifier;

public:	
	// Sets default values for this component's properties
	UHealthComponent();

	UFUNCTION(BlueprintCallable, Category = "Character|Health")
	int32 DealDamage(int32 magnitude, TSubclassOf<UDamageType> type, const FVector& knockbackImpulse = FVector::ZeroVector);

	UFUNCTION(BlueprintCallable, Category = "Character|Health")
	int32 HealDamage(int32 magnitude);

	UFUNCTION(BlueprintCallable, Category = "Character")
	FORCEINLINE ACharacter* GetCharacterOwner() const { if (mCharacterOwner.IsValid()) { return mCharacterOwner.Get(); } return nullptr; }

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

private:
	void SetDamageTaken(int32 totalDamage);

private:
	int32 mDamageTaken = 0;
	TWeakObjectPtr<ACharacter> mCharacterOwner = nullptr;
};
