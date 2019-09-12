// Copyright Inside Out Games Ltd. 2017

#include "WeaponProjectile.h"
#include "Vert.h"
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
	CollisionComp->bGenerateOverlapEvents = true;
	CollisionComp->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
	CollisionComp->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
	CollisionComp->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
	CollisionComp->SetCanEverAffectNavigation(false);
	RootComponent = CollisionComp;
	
	MovementComp = ObjectInitializer.CreateDefaultSubobject<UProjectileMovementComponent>(this, TEXT("ProjectileComp"));
	MovementComp->UpdatedComponent = CollisionComp;
	MovementComp->InitialSpeed = 2000.0f;
	MovementComp->MaxSpeed = 2000.0f;
	MovementComp->bRotationFollowsVelocity = true;
	MovementComp->ProjectileGravityScale = 0.f;
	MovementComp->bInitialVelocityInLocalSpace = false;
	MovementComp->SetCanEverAffectNavigation(false);
	
	LifeSpanPool = 3.f;

	SetRemoteRoleForBackwardsCompat(ROLE_SimulatedProxy);
	bReplicates = true;
	bReplicateMovement = true;
}

void AWeaponProjectile::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	FScriptDelegate pooledBeginPlay;
	pooledBeginPlay.BindUFunction(this, TEXT("PoolBeginPlay"));
	OnPoolBeginPlay.Add(pooledBeginPlay);

	FScriptDelegate pooledEndPlay;
	pooledEndPlay.BindUFunction(this, TEXT("PoolEndPlay"));
	OnPoolEndPlay.Add(pooledEndPlay);

	MovementComp->OnProjectileStop.AddDynamic(this, &AWeaponProjectile::OnImpact);
	CollisionComp->OnComponentHit.AddDynamic(this, &AWeaponProjectile::OnCollisionHit);
}

//************************************
// Method:    PoolBeginPlay
// FullName:  AWeaponProjectile::PoolBeginPlay
// Access:    protected 
// Returns:   void
// Qualifier:
//************************************
void AWeaponProjectile::PoolBeginPlay()
{
	if (AVertLevelScriptActor* level = Cast<AVertLevelScriptActor>(GetWorld()->GetLevelScriptActor()))
	{
		MovementComp->SetPlaneConstraintNormal(FVector::RightVector);
		MovementComp->SetPlaneConstraintOrigin(level->GetPlaneConstraintOrigin());
		MovementComp->SetPlaneConstraintEnabled(true);
		MovementComp->SnapUpdatedComponentToPlane();
	}

	MovementComp->Activate();
	MovementComp->SetUpdatedComponent(CollisionComp);
}

//************************************
// Method:    PoolEndPlay
// FullName:  AWeaponProjectile::PoolEndPlay
// Access:    protected 
// Returns:   void
// Qualifier:
//************************************
void AWeaponProjectile::PoolEndPlay()
{
	CollisionComp->MoveIgnoreActors.Empty();

	MovementComp->StopMovementImmediately();
	MovementComp->Deactivate();

	Instigator = nullptr;
	SetOwner(nullptr);

	mController = nullptr;
}

//************************************
// Method:    LifeSpanExpired
// FullName:  AWeaponProjectile::LifeSpanExpired
// Access:    virtual protected 
// Returns:   void
// Qualifier:
//************************************
void AWeaponProjectile::LifeSpanExpired()
{
	ReturnToPool(); // Return to object pool instead of destroying this object
}

//************************************
// Method:    InitVelocity
// FullName:  AWeaponProjectile::InitVelocity
// Access:    public 
// Returns:   void
// Qualifier:
// Parameter: FVector & ShootDirection
//************************************
void AWeaponProjectile::InitVelocity(const FVector& ShootDirection)
{
	if (MovementComp)
	{		
		MovementComp->Velocity = ShootDirection * MovementComp->InitialSpeed;
	}
}

//************************************
// Method:    InitProjectileConfig
// FullName:  AWeaponProjectile::InitProjectileConfig
// Access:    public 
// Returns:   void
// Qualifier:
// Parameter: const FProjectileWeaponData & weaponData
// Parameter: int32 baseDamage
// Parameter: float baseKnockback
// Parameter: float knockbackScaling
//************************************
void AWeaponProjectile::InitProjectile(const FProjectileWeaponData& weaponData, int32 baseDamage, float baseKnockback, float knockbackScaling, float stunTime)
{
	mWeaponConfig = weaponData;
	mWeaponConfig.BaseDamage = baseDamage;
	mWeaponConfig.BaseKnockback = baseKnockback;
	mWeaponConfig.KnockbackScaling = knockbackScaling;
	mWeaponConfig.StunTime = stunTime;

	SetLifeSpan(mWeaponConfig.ProjectileLife);
	mController = GetInstigatorController();

	CollisionComp->MoveIgnoreActors.Add(Instigator);
}

//************************************
// Method:    OnImpact_Implementation
// FullName:  AWeaponProjectile::OnImpact_Implementation
// Access:    public 
// Returns:   void
// Qualifier:
// Parameter: const FHitResult & HitResult
//************************************
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

//************************************
// Method:    OnCollisionHit
// FullName:  AWeaponProjectile::OnCollisionHit
// Access:    public 
// Returns:   void
// Qualifier:
// Parameter: UPrimitiveComponent * hitComponent
// Parameter: AActor * otherActor
// Parameter: UPrimitiveComponent * otherComp
// Parameter: FVector normalImpulse
// Parameter: const FHitResult & hit
//************************************
void AWeaponProjectile::OnCollisionHit_Implementation(UPrimitiveComponent* hitComponent, AActor* otherActor, UPrimitiveComponent* otherComp, FVector normalImpulse, const FHitResult& hit)
{
	if (!MovementComp->bShouldBounce)
	{
		OnImpact(hit);
	}
}

//************************************
// Method:    Explode
// FullName:  AWeaponProjectile::Explode
// Access:    protected 
// Returns:   void
// Qualifier:
// Parameter: const FHitResult & Impact
//************************************
void AWeaponProjectile::Explode(const FHitResult& Impact)
{
	// effects and damage origin shouldn't be placed inside mesh at impact point
	const FVector NudgedImpactLocation = Impact.ImpactPoint + Impact.ImpactNormal * 10.0f;

	if (mWeaponConfig.BaseDamage > 0 && mWeaponConfig.DamageType && mWeaponConfig.ExplosionRadius > 0)
	{
		UVertUtilities::VERT_ApplyRadialDamage(this, mWeaponConfig.BaseDamage, mWeaponConfig.BaseKnockback, mWeaponConfig.KnockbackScaling, mWeaponConfig.StunTime, NudgedImpactLocation, mWeaponConfig.ExplosionRadius, mWeaponConfig.DamageType, TArray<AActor*>(), this, mController.Get());
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

//************************************
// Method:    ApplyPointDamage
// FullName:  AWeaponProjectile::ApplyPointDamage
// Access:    protected 
// Returns:   void
// Qualifier:
// Parameter: const FHitResult & impact
//************************************
void AWeaponProjectile::ApplyPointDamage(const FHitResult& impact)
{
	if (impact.Actor.IsValid())
	{
		ABaseWeapon* firedFrom = Cast<ABaseWeapon>(GetOwner());

		FVertPointDamageEvent PointDmg;
		PointDmg.DamageTypeClass = mWeaponConfig.DamageType;
		PointDmg.HitInfo = impact;
		PointDmg.ShotDirection = -impact.ImpactNormal;
		PointDmg.Damage = mWeaponConfig.BaseDamage + (firedFrom ? firedFrom->GetBonusDamage() : 0);
		PointDmg.Knockback = mWeaponConfig.BaseKnockback + (firedFrom ? firedFrom->GetBonusKnockback() : 0);
		PointDmg.KnockbackScaling = mWeaponConfig.KnockbackScaling;
		PointDmg.StunTime = mWeaponConfig.StunTime;

		ACharacter* instigatingCharacter = Cast<ACharacter>(Instigator);
		impact.GetActor()->TakeDamage(
			PointDmg.Damage,
			PointDmg,
			instigatingCharacter ? instigatingCharacter->Controller : nullptr,
			this
		);
	}	
}

//************************************
// Method:    DisableAndDestroy
// FullName:  AWeaponProjectile::DisableAndDestroy
// Access:    protected 
// Returns:   void
// Qualifier:
//************************************
void AWeaponProjectile::DisableAndDestroy()
{
	UAudioComponent* ProjAudioComp = FindComponentByClass<UAudioComponent>();
	if (ProjAudioComp && ProjAudioComp->IsPlaying())
	{
		ProjAudioComp->FadeOut(0.1f, 0.f);
	}

	MovementComp->StopMovementImmediately();

	// give clients some time to show explosion
	if (mWeaponConfig.IsExplosive)
		SetLifeSpan(2.0f);
	else
		ReturnToPool();
}

///CODE_SNIPPET_START: AActor::GetActorLocation AActor::GetActorRotation
//************************************
// Method:    OnRep_Exploded
// FullName:  AWeaponProjectile::OnRep_Exploded
// Access:    protected 
// Returns:   void
// Qualifier:
//************************************
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

//************************************
// Method:    PostNetReceiveVelocity
// FullName:  AWeaponProjectile::PostNetReceiveVelocity
// Access:    virtual protected 
// Returns:   void
// Qualifier:
// Parameter: const FVector & NewVelocity
//************************************
void AWeaponProjectile::PostNetReceiveVelocity(const FVector& NewVelocity)
{
	if (MovementComp)
	{
		MovementComp->Velocity = NewVelocity;
	}
}

//************************************
// Method:    GetLifetimeReplicatedProps
// FullName:  AWeaponProjectile::GetLifetimeReplicatedProps
// Access:    public 
// Returns:   void
// Qualifier: const
// Parameter: TArray< FLifetimeProperty > & OutLifetimeProps
//************************************
void AWeaponProjectile::GetLifetimeReplicatedProps(TArray< FLifetimeProperty > & OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AWeaponProjectile, bExploded);
}