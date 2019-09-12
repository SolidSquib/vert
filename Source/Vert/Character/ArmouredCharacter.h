// Copyright Inside Out Games Limited 2017

#pragma once

#include "CoreMinimal.h"
#include "Character/VertCharacterBase.h"
#include "ArmouredCharacter.generated.h"

/**
 * 
 */
UCLASS()
class VERT_API AArmouredCharacter : public AVertCharacterBase
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintCallable, Category = "Health")
	virtual float TakeDamage(float Damage, const FDamageEvent& DamageEvent, class AController* EventInstigator, class AActor* DamageCauser) override;

protected:
	UFUNCTION(BlueprintImplementableEvent)
	void TakeDamageSucceeded(float actualDamageTaken);

	UFUNCTION(BlueprintImplementableEvent)
	void TakeDamageFailed();
};
