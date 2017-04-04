// Copyright Inside Out Games Ltd. 2017

#include "Vert.h"
#include "VertPlayerState.h"

static FColor sTeamColours[] = {
	FColor::Blue,
	FColor::Emerald,
	FColor::Red,
	FColor::Yellow
};

void AVertPlayerState::SetDamageTaken(int32 newDamage)
{
	DamageTaken = newDamage;
}

int32 AVertPlayerState::GetDamageTaken() const 
{
	return DamageTaken;
}

void AVertPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AVertPlayerState, DamageTaken);
}