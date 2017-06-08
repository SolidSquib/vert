// Copyright Inside Out Games Ltd. 2017

#include "MeleeWeapon.h"
#include "Vert.h"

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

void AMeleeWeapon::ClientSimulateWeaponFire_Implementation()
{
	Super::ClientSimulateWeaponFire_Implementation();
}

void AMeleeWeapon::ClientStopSimulateWeaponFire_Implementation()
{
	Super::ClientStopSimulateWeaponFire_Implementation();

	if (ArcPSC != NULL)
	{
		ArcPSC->DeactivateSystem();
		ArcPSC = NULL;
	}
}

bool AMeleeWeapon::FireWeapon_Implementation()
{
	// Handled in animgraph
	OnMeleeAttack.Broadcast(mDidHit ? mComboDepth++ : (mComboDepth = 0));
	mDidHit = false;

	return true;
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

	if (otherActor)
	{
		FPointDamageEvent PointDmg;
		PointDmg.DamageTypeClass = WeaponConfig.DamageType;
		PointDmg.HitInfo = sweepResult;
		PointDmg.ShotDirection = otherActor->GetActorLocation() - GetActorLocation();
		PointDmg.Damage = WeaponConfig.BaseDamage;

		otherActor->TakeDamage(PointDmg.Damage, PointDmg, MyPawn->Controller, this);
	}
}

void AMeleeWeapon::OnWeaponEndOverlap_Implementation(UPrimitiveComponent* overlappedComp, AActor* otherActor, UPrimitiveComponent* otherComp, int32 otherBodyIndex)
{
	
}