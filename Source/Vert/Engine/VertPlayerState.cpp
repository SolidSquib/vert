// Copyright Inside Out Games Ltd. 2017

#include "Vert.h"
#include "VertPlayerState.h"

static FColor sTeamColours[] = {
	FColor::Blue,
	FColor::Emerald,
	FColor::Red,
	FColor::Yellow
};

void AVertPlayerState::SetDamageTaken(int32 newDamage, int32 newShownDamage)
{
	ActualDamageTaken = newDamage;
	ShownDamageTaken = newShownDamage;
}

int32 AVertPlayerState::GetActualDamageTaken() const 
{
	return ActualDamageTaken;
}

int32 AVertPlayerState::GetShownDamageTaken() const 
{
	return ShownDamageTaken;
}

void AVertPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AVertPlayerState, ActualDamageTaken);
	DOREPLIFETIME(AVertPlayerState, ShownDamageTaken);
}