// Copyright Inside Out Games Ltd. 2017

#include "Vert.h"
#include "WeaponProjectile.h"
#include "Particles/ParticleSystemComponent.h"
#include "Effects/VertExplosionEffect.h"

AWeaponProjectile::AWeaponProjectile(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	CollisionComp = ObjectInitializer.CreateDefaultSubobject<USphereComponent>(this, TEXT("SphereComp"));
	CollisionComp->InitSphereRadius(5.0f);
	CollisionComp->AlwaysLoadOnClient = true;
	CollisionComp->AlwaysLoadOnServer = true;
	CollisionComp->bTraceComplexOnMove = true;
	CollisionComp->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CollisionComp->SetCollisionObjectType(ECC_WeaponProjectile);
	CollisionComp->SetCollisionResponseToAllChannels(ECR_Ignore);
	CollisionComp->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
	CollisionComp->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
	CollisionComp->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
	RootComponent = CollisionComp;

	ParticleComp = ObjectInitializer.CreateDefaultSubobject<UParticleSystemComponent>(this, TEXT("ParticleComp"));
	ParticleComp->bAutoActivate = false;
	ParticleComp->bAutoDestroy = false;
	ParticleComp->SetupAttachment(RootComponent);

	MovementComp = ObjectInitializer.CreateDefaultSubobject<UProjectileMovementComponent>(this, TEXT("ProjectileComp"));
	MovementComp->UpdatedComponent = CollisionComp;
	MovementComp->InitialSpeed = 2000.0f;
	MovementComp->MaxSpeed = 2000.0f;
	MovementComp->bRotationFollowsVelocity = true;
	MovementComp->ProjectileGravityScale = 0.f;
	MovementComp->bInitialVelocityInLocalSpace = false;

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	MeshComponent->SetupAttachment(RootComponent);

	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PrePhysics;
	SetRemoteRoleForBackwardsCompat(ROLE_SimulatedProxy);
	bReplicates = true;
	bReplicateMovement = true;
}

void AWeaponProjectile::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	MovementComp->OnProjectileStop.AddDynamic(this, &AWeaponProjectile::OnImpact);
	CollisionComp->MoveIgnoreActors.Add(Instigator);

	AProjectileRangedWeapon* OwnerWeapon = Cast<AProjectileRangedWeapon>(GetOwner());
	if (OwnerWeapon)
	{
		OwnerWeapon->ApplyWeaponConfig(mWeaponConfig);
	}

	SetLifeSpan(mWeaponConfig.ProjectileLife);
	mController = GetInstigatorController();
}

void AWeaponProjectile::InitVelocity(FVector& ShootDirection)
{
	if (MovementComp)
	{		
		MovementComp->Velocity = ShootDirection * MovementComp->InitialSpeed;
	}
}

void AWeaponProjectile::OnImpact_Implementation(const FHitResult& HitResult)
{
	if (Role == ROLE_Authority && !bExploded)
	{
		if (mWeaponConfig.IsExplosive)
			Explode(HitResult);
		else
			ApplyPointDamage(HitResult);

		DisableAndDestroy();
	}	
}

void AWeaponProjectile::Explode(const FHitResult& Impact)
{
	if (ParticleComp)
	{
		ParticleComp->Deactivate();
	}

	// effects and damage origin shouldn't be placed inside mesh at impact point
	const FVector NudgedImpactLocation = Impact.ImpactPoint + Impact.ImpactNormal * 10.0f;

	if (BaseDamage > 0 && mWeaponConfig.DamageType && mWeaponConfig.ExplosionRadius > 0)
	{
		UGameplayStatics::ApplyRadialDamage(this, BaseDamage, NudgedImpactLocation, mWeaponConfig.ExplosionRadius, mWeaponConfig.DamageType, TArray<AActor*>(), this, mController.Get());
	}

	if (ExplosionTemplate)
	{
		FTransform const SpawnTransform(Impact.ImpactNormal.Rotation(), NudgedImpactLocation);
		AVertExplosionEffect* const EffectActor = GetWorld()->SpawnActorDeferred<AVertExplosionEffect>(ExplosionTemplate, SpawnTransform);
		if (EffectActor)
		{
			EffectActor->SurfaceHit = Impact;
			UGameplayStatics::FinishSpawningActor(EffectActor, SpawnTransform);
		}
	}

	bExploded = true;
}

void AWeaponProjectile::ApplyPointDamage(const FHitResult& impact)
{
	ABaseWeapon* firedFrom = Cast<ABaseWeapon>(GetOwner());

	FPointDamageEvent PointDmg;
	PointDmg.DamageTypeClass = mWeaponConfig.DamageType;
	PointDmg.HitInfo = impact;
	PointDmg.ShotDirection = MovementComp->Velocity.GetSafeNormal();
	PointDmg.Damage = firedFrom ? firedFrom->GetBaseDamage() : 0;

	ACharacter* instigatingCharacter = Cast<ACharacter>(Instigator);
	impact.GetActor()->TakeDamage(PointDmg.Damage, PointDmg, instigatingCharacter ? instigatingCharacter->Controller : nullptr, this);
}

void AWeaponProjectile::DisableAndDestroy()
{
	UAudioComponent* ProjAudioComp = FindComponentByClass<UAudioComponent>();
	if (ProjAudioComp && ProjAudioComp->IsPlaying())
	{
		ProjAudioComp->FadeOut(0.1f, 0.f);
	}

	MovementComp->StopMovementImmediately();

	// give clients some time to show explosion
	SetLifeSpan(2.0f);
}

///CODE_SNIPPET_START: AActor::GetActorLocation AActor::GetActorRotation
void AWeaponProjectile::OnRep_Exploded()
{
	FVector ProjDirection = GetActorForwardVector();

	const FVector StartTrace = GetActorLocation() - ProjDirection * 200;
	const FVector EndTrace = GetActorLocation() + ProjDirection * 150;
	FHitResult Impact;

	if (!GetWorld()->LineTraceSingleByChannel(Impact, StartTrace, EndTrace, ECC_WeaponProjectile, FCollisionQueryParams(TEXT("ProjClient"), true, Instigator)))
	{
		// failsafe
		Impact.ImpactPoint = GetActorLocation();
		Impact.ImpactNormal = -ProjDirection;
	}

	Explode(Impact);
}
///CODE_SNIPPET_END

void AWeaponProjectile::PostNetReceiveVelocity(const FVector& NewVelocity)
{
	if (MovementComp)
	{
		MovementComp->Velocity = NewVelocity;
	}
}

void AWeaponProjectile::GetLifetimeReplicatedProps(TArray< FLifetimeProperty > & OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AWeaponProjectile, bExploded);
}