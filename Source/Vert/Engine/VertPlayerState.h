// Copyright Inside Out Games Ltd. 2017

#pragma once

#include "GameFramework/PlayerState.h"
#include "VertPlayerState.generated.h"

/**
 * 
 */
UCLASS()
class VERT_API AVertPlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	UPROPERTY(Transient, Replicated)
	int32 ActualDamageTaken = 0;

	UPROPERTY(Transient, Replicated)
	int32 ShownDamageTaken = 0;

public:
	void SetDamageTaken(int32 newDamage, int32 newShownDamage);
	int32 GetActualDamageTaken() const;
	int32 GetShownDamageTaken() const;
};
