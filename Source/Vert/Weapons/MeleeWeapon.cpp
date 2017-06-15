// Copyright Inside Out Games Ltd. 2017

#include "MeleeWeapon.h"
#include "Vert.h"

DECLARE_LOG_CATEGORY_CLASS(LogVertMeleeWeapon, Log, All);

AMeleeWeapon::AMeleeWeapon(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	bReplicates = true;
}

// Called when the game starts or when spawned
void AMeleeWeapon::BeginPlay()
{
	Super::BeginPlay();
}

void AMeleeWeapon::ClientSimulateWeaponAttack_Implementation()
{
	Super::ClientSimulateWeaponAttack_Implementation();
}

void AMeleeWeapon::ClientStopSimulateWeaponAttack_Implementation()
{
	Super::ClientStopSimulateWeaponAttack_Implementation();

	if (ArcPSC != NULL)
	{
		ArcPSC->DeactivateSystem();
		ArcPSC = NULL;
	}
}

bool AMeleeWeapon::AttackWithWeapon_Implementation()
{
	if (mTraceHit)
	{
		for (FName socket : ScanSockets)
		{
			FVector start;
			FQuat rotation;
			WeaponMesh->GetSocketWorldLocationAndRotation(socket, start, rotation);
			FVector direction = rotation.Vector().GetSafeNormal();

			FHitResult hit = WeaponTrace(start, direction*MeleeTraceRange);
			if (hit.bBlockingHit)
				mComboDepth += 1;
		}

		MeleeAttackWithWeapon();
	}

	return mAttackDone;
}

void AMeleeWeapon::MeleeAttackWithWeapon_Implementation()
{
	OnMeleeAttack.Broadcast(mComboDepth);
}

void AMeleeWeapon::NotifyAttackAnimationActiveStarted_Implementation()
{
	mTraceHit = true;

	if (AttackStartFX)
	{
		UParticleSystemComponent* AttackStartPSC = UGameplayStatics::SpawnEmitterAtLocation(this, AttackStartFX, WeaponMesh->GetSocketLocation(FXSocketTop));
	}

	if (ArcFX)
	{
		if (ArcPSC == NULL)
		{
			ArcPSC = UGameplayStatics::SpawnEmitterAttached(ArcFX, WeaponMesh, FXSocketTop);
		}
	}
}

void AMeleeWeapon::NotifyAttackAnimationActiveEnded_Implementation()
{
	mTraceHit = false;

	if (AttackEndFX)
	{
		UParticleSystemComponent* AttackEndPSC = UGameplayStatics::SpawnEmitterAtLocation(this, AttackEndFX, WeaponMesh->GetSocketLocation(FXSocketTop));
	}

	if (ArcPSC != NULL)
	{
		ArcPSC->DeactivateSystem();
		ArcPSC = NULL;
	}

	mAttackDone = true;
}