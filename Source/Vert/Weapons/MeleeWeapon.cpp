// Copyright Inside Out Games Ltd. 2017

#include "Vert.h"
#include "MeleeWeapon.h"

DECLARE_LOG_CATEGORY_CLASS(LogVertMeleeWeapon, Log, All);

AMeleeWeapon::AMeleeWeapon(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	WeaponMesh->SetCollisionObjectType(ECC_WorldDynamic);
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WeaponMesh->SetCollisionResponseToAllChannels(ECR_Ignore);

	bReplicates = true;
}

// Called when the game starts or when spawned
void AMeleeWeapon::BeginPlay()
{
	Super::BeginPlay();

	WeaponMesh->OnComponentBeginOverlap.AddDynamic(this, &AMeleeWeapon::OnWeaponBeginOverlap);
	WeaponMesh->OnComponentEndOverlap.AddDynamic(this, &AMeleeWeapon::OnWeaponEndOverlap);
}

void AMeleeWeapon::SimulateWeaponFire()
{
	Super::SimulateWeaponFire();
}

void AMeleeWeapon::StopSimulatingWeaponFire()
{
	Super::StopSimulatingWeaponFire();

	if (ArcPSC != NULL)
	{
		ArcPSC->DeactivateSystem();
		ArcPSC = NULL;
	}
}

void AMeleeWeapon::FireWeapon_Implementation()
{
	// Handled in animgraph
	OnMeleeAttack.Broadcast(mDidHit ? mComboDepth++ : (mComboDepth = 0));
}

void AMeleeWeapon::NotifyAttackBegin()
{
	WeaponMesh->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Overlap);
	WeaponMesh->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Overlap);
	WeaponMesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

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

void AMeleeWeapon::NotifyAttackEnd()
{
	WeaponMesh->SetCollisionResponseToAllChannels(ECR_Ignore);

	if (AttackEndFX)
	{
		UParticleSystemComponent* AttackEndPSC = UGameplayStatics::SpawnEmitterAtLocation(this, AttackEndFX, WeaponMesh->GetSocketLocation(FXSocketTop));
	}

	if (ArcPSC != NULL)
	{
		ArcPSC->DeactivateSystem();
		ArcPSC = NULL;
	}
}

void AMeleeWeapon::OnWeaponBeginOverlap_Implementation(UPrimitiveComponent* overlappedComp, AActor* otherActor, UPrimitiveComponent* otherComp, int32 otherBodyIndex, bool fromSweep, const FHitResult& sweepResult)
{
	mDidHit = true;
}

void AMeleeWeapon::OnWeaponEndOverlap_Implementation(UPrimitiveComponent* overlappedComp, AActor* otherActor, UPrimitiveComponent* otherComp, int32 otherBodyIndex)
{
	
}