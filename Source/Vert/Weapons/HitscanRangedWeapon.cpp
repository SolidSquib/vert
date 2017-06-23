// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "Weapons/HitscanRangedWeapon.h"
#include "Particles/ParticleSystemComponent.h"
#include "Effects/VertImpactEffect.h"

DECLARE_LOG_CATEGORY_CLASS(LogHitscanRangedWeapon, Log, All);

AHitscanRangedWeapon::AHitscanRangedWeapon(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	
}

//////////////////////////////////////////////////////////////////////////
// Weapon usage

//************************************
// Method:    AttackWithWeapon_Implementation
// FullName:  AHitscanRangedWeapon::AttackWithWeapon_Implementation
// Access:    virtual protected 
// Returns:   bool
// Qualifier:
//************************************
bool AHitscanRangedWeapon::AttackWithWeapon_Implementation()
{
	int32 randomSeed;
	float currentSpread;

	FVector AimDir = GetAdjustedAim();
	const FVector StartTrace = GetMuzzleLocation();
	const FVector ShootDir = GetShootDirectionAfterSpread(AimDir, randomSeed, currentSpread);
	const FVector EndTrace = StartTrace + (ShootDir * InstantConfig.WeaponRange);

	const FHitResult Impact = WeaponTrace(StartTrace, EndTrace);
	ProcessInstantHit(Impact, StartTrace, ShootDir, randomSeed, currentSpread);

	mCurrentFiringSpread = FMath::Min(SpreadConfig.FiringSpreadMax, mCurrentFiringSpread + SpreadConfig.FiringSpreadIncrement);

	Delegate_OnWeaponFiredWithRecoil.Broadcast(SpreadConfig.RecoilAmount);

	return true;
}

//************************************
// Method:    ServerNotifyHit_Validate
// FullName:  AHitscanRangedWeapon::ServerNotifyHit_Validate
// Access:    public 
// Returns:   bool
// Qualifier:
// Parameter: const FHitResult & Impact
// Parameter: FVector_NetQuantizeNormal ShootDir
// Parameter: int32 RandomSeed
// Parameter: float ReticleSpread
//************************************
bool AHitscanRangedWeapon::ServerNotifyHit_Validate(const FHitResult& Impact, FVector_NetQuantizeNormal ShootDir, int32 RandomSeed, float ReticleSpread)
{
	return true;
}

//************************************
// Method:    ServerNotifyHit_Implementation
// FullName:  AHitscanRangedWeapon::ServerNotifyHit_Implementation
// Access:    public 
// Returns:   void
// Qualifier:
// Parameter: const FHitResult & Impact
// Parameter: FVector_NetQuantizeNormal ShootDir
// Parameter: int32 RandomSeed
// Parameter: float ReticleSpread
//************************************
void AHitscanRangedWeapon::ServerNotifyHit_Implementation(const FHitResult& Impact, FVector_NetQuantizeNormal ShootDir, int32 RandomSeed, float ReticleSpread)
{
	const float WeaponAngleDot = FMath::Abs(FMath::Sin(ReticleSpread * PI / 180.f));

	// if we have an instigator, calculate dot between the view and the shot
	if (Instigator && (Impact.GetActor() || Impact.bBlockingHit))
	{
		const FVector Origin = GetMuzzleLocation();
		const FVector ViewDir = (Impact.Location - Origin).GetSafeNormal();

		// is the angle between the hit and the view within allowed limits (limit + weapon max angle)
		const float ViewDotHitDir = FVector::DotProduct(Instigator->GetViewRotation().Vector(), ViewDir);
		if (ViewDotHitDir > InstantConfig.AllowedViewDotHitDir - WeaponAngleDot)
		{
			if (mCurrentState != EWeaponState::CombatIdle && mCurrentState != EWeaponState::PassiveIdle)
			{
				if (Impact.GetActor() == NULL)
				{
					if (Impact.bBlockingHit)
					{
						ProcessInstantHit_Confirmed(Impact, Origin, ShootDir, RandomSeed, ReticleSpread);
					}
				}
				// assume it told the truth about static things because the don't move and the hit 
				// usually doesn't have significant gameplay implications
				else if (Impact.GetActor()->IsRootComponentStatic() || Impact.GetActor()->IsRootComponentStationary())
				{
					ProcessInstantHit_Confirmed(Impact, Origin, ShootDir, RandomSeed, ReticleSpread);
				}
				else
				{
					// Get the component bounding box
					const FBox HitBox = Impact.GetActor()->GetComponentsBoundingBox();

					// calculate the box extent, and increase by a leeway
					FVector BoxExtent = 0.5 * (HitBox.Max - HitBox.Min);
					BoxExtent *= InstantConfig.ClientSideHitLeeway;

					// avoid precision errors with really thin objects
					BoxExtent.X = FMath::Max(20.0f, BoxExtent.X);
					BoxExtent.Y = FMath::Max(20.0f, BoxExtent.Y);
					BoxExtent.Z = FMath::Max(20.0f, BoxExtent.Z);

					// Get the box center
					const FVector BoxCenter = (HitBox.Min + HitBox.Max) * 0.5;

					// if we are within client tolerance
					if (FMath::Abs(Impact.Location.Z - BoxCenter.Z) < BoxExtent.Z &&
						FMath::Abs(Impact.Location.X - BoxCenter.X) < BoxExtent.X &&
						FMath::Abs(Impact.Location.Y - BoxCenter.Y) < BoxExtent.Y)
					{
						ProcessInstantHit_Confirmed(Impact, Origin, ShootDir, RandomSeed, ReticleSpread);
					}
					else
					{
						UE_LOG(LogHitscanRangedWeapon, Log, TEXT("%s Rejected client side hit of %s (outside bounding box tolerance)"), *GetNameSafe(this), *GetNameSafe(Impact.GetActor()));
					}
				}
			}
		}
		else if (ViewDotHitDir <= InstantConfig.AllowedViewDotHitDir)
		{
			UE_LOG(LogHitscanRangedWeapon, Log, TEXT("%s Rejected client side hit of %s (facing too far from the hit direction)"), *GetNameSafe(this), *GetNameSafe(Impact.GetActor()));
		}
		else
		{
			UE_LOG(LogHitscanRangedWeapon, Log, TEXT("%s Rejected client side hit of %s"), *GetNameSafe(this), *GetNameSafe(Impact.GetActor()));
		}
	}
}

//************************************
// Method:    ServerNotifyMiss_Validate
// FullName:  AHitscanRangedWeapon::ServerNotifyMiss_Validate
// Access:    public 
// Returns:   bool
// Qualifier:
// Parameter: FVector_NetQuantizeNormal ShootDir
// Parameter: int32 RandomSeed
// Parameter: float ReticleSpread
//************************************
bool AHitscanRangedWeapon::ServerNotifyMiss_Validate(FVector_NetQuantizeNormal ShootDir, int32 RandomSeed, float ReticleSpread)
{
	return true;
}

//************************************
// Method:    ServerNotifyMiss_Implementation
// FullName:  AHitscanRangedWeapon::ServerNotifyMiss_Implementation
// Access:    public 
// Returns:   void
// Qualifier:
// Parameter: FVector_NetQuantizeNormal ShootDir
// Parameter: int32 RandomSeed
// Parameter: float ReticleSpread
//************************************
void AHitscanRangedWeapon::ServerNotifyMiss_Implementation(FVector_NetQuantizeNormal ShootDir, int32 RandomSeed, float ReticleSpread)
{
	const FVector Origin = GetMuzzleLocation();

	// play FX on remote clients
	HitNotify.Origin = Origin;
	HitNotify.RandomSeed = RandomSeed;
	HitNotify.ReticleSpread = ReticleSpread;

	// play FX locally
	if (GetNetMode() != NM_DedicatedServer)
	{
		const FVector EndTrace = Origin + ShootDir * InstantConfig.WeaponRange;
		SpawnTrailEffect(EndTrace);
	}
}

//************************************
// Method:    ProcessInstantHit
// FullName:  AHitscanRangedWeapon::ProcessInstantHit
// Access:    protected 
// Returns:   void
// Qualifier:
// Parameter: const FHitResult & Impact
// Parameter: const FVector & Origin
// Parameter: const FVector & ShootDir
// Parameter: int32 RandomSeed
// Parameter: float ReticleSpread
//************************************
void AHitscanRangedWeapon::ProcessInstantHit(const FHitResult& Impact, const FVector& Origin, const FVector& ShootDir, int32 RandomSeed, float ReticleSpread)
{
	if (MyPawn && MyPawn->IsLocallyControlled() && GetNetMode() == NM_Client)
	{
		// if we're a client and we've hit something that is being controlled by the server
		if (Impact.GetActor() && Impact.GetActor()->GetRemoteRole() == ROLE_Authority)
		{
			// notify the server of the hit
			ServerNotifyHit(Impact, ShootDir, RandomSeed, ReticleSpread);
		}
		else if (Impact.GetActor() == NULL)
		{
			if (Impact.bBlockingHit)
			{
				// notify the server of the hit
				ServerNotifyHit(Impact, ShootDir, RandomSeed, ReticleSpread);
			}
			else
			{
				// notify server of the miss
				ServerNotifyMiss(ShootDir, RandomSeed, ReticleSpread);
			}
		}
	}

	// process a confirmed hit
	ProcessInstantHit_Confirmed(Impact, Origin, ShootDir, RandomSeed, ReticleSpread);
}

//************************************
// Method:    ProcessInstantHit_Confirmed
// FullName:  AHitscanRangedWeapon::ProcessInstantHit_Confirmed
// Access:    protected 
// Returns:   void
// Qualifier:
// Parameter: const FHitResult & Impact
// Parameter: const FVector & Origin
// Parameter: const FVector & ShootDir
// Parameter: int32 RandomSeed
// Parameter: float ReticleSpread
//************************************
void AHitscanRangedWeapon::ProcessInstantHit_Confirmed(const FHitResult& Impact, const FVector& Origin, const FVector& ShootDir, int32 RandomSeed, float ReticleSpread)
{
	// handle damage
	if (ShouldDealDamage(Impact.GetActor()))
	{
		DealDamage(Impact, ShootDir);
	}

	// play FX on remote clients
	if (Role == ROLE_Authority)
	{
		HitNotify.Origin = Origin;
		HitNotify.RandomSeed = RandomSeed;
		HitNotify.ReticleSpread = ReticleSpread;
	}

	// play FX locally
	if (GetNetMode() != NM_DedicatedServer)
	{
		const FVector EndTrace = Origin + ShootDir * InstantConfig.WeaponRange;
		const FVector EndPoint = Impact.GetActor() ? Impact.ImpactPoint : EndTrace;

		SpawnTrailEffect(EndPoint);
		SpawnImpactEffects(Impact);
	}
}

//************************************
// Method:    ShouldDealDamage
// FullName:  AHitscanRangedWeapon::ShouldDealDamage
// Access:    protected 
// Returns:   bool
// Qualifier: const
// Parameter: AActor * TestActor
//************************************
bool AHitscanRangedWeapon::ShouldDealDamage(AActor* TestActor) const
{
	// if we're an actor on the server, or the actor's role is authoritative, we should register damage
	if (TestActor)
	{
		if (GetNetMode() != NM_Client ||
			TestActor->Role == ROLE_Authority ||
			TestActor->bTearOff)
		{
			return true;
		}
	}

	return false;
}

//************************************
// Method:    DealDamage
// FullName:  AHitscanRangedWeapon::DealDamage
// Access:    protected 
// Returns:   void
// Qualifier:
// Parameter: const FHitResult & Impact
// Parameter: const FVector & ShootDir
//************************************
void AHitscanRangedWeapon::DealDamage(const FHitResult& Impact, const FVector& ShootDir)
{
	FPointDamageEvent PointDmg;
	PointDmg.DamageTypeClass = WeaponConfig.DamageType;
	PointDmg.HitInfo = Impact;
	PointDmg.ShotDirection = ShootDir;
	PointDmg.Damage = WeaponConfig.BaseDamage;

	Impact.GetActor()->TakeDamage(PointDmg.Damage, PointDmg, MyPawn->Controller, this);
}

//////////////////////////////////////////////////////////////////////////
// Replication & effects

//************************************
// Method:    OnRep_HitNotify
// FullName:  AHitscanRangedWeapon::OnRep_HitNotify
// Access:    protected 
// Returns:   void
// Qualifier:
//************************************
void AHitscanRangedWeapon::OnRep_HitNotify()
{
	SimulateInstantHit(HitNotify.Origin, HitNotify.RandomSeed, HitNotify.ReticleSpread);
}

//************************************
// Method:    SimulateInstantHit
// FullName:  AHitscanRangedWeapon::SimulateInstantHit
// Access:    protected 
// Returns:   void
// Qualifier:
// Parameter: const FVector & ShotOrigin
// Parameter: int32 RandomSeed
// Parameter: float ReticleSpread
//************************************
void AHitscanRangedWeapon::SimulateInstantHit(const FVector& ShotOrigin, int32 RandomSeed, float ReticleSpread)
{
	FRandomStream WeaponRandomStream(RandomSeed);
	const float ConeHalfAngle = FMath::DegreesToRadians(ReticleSpread * 0.5f);

	const FVector StartTrace = ShotOrigin;
	const FVector AimDir = GetMuzzleDirection();
	const FVector ShootDir = WeaponRandomStream.VRandCone(AimDir, ConeHalfAngle, ConeHalfAngle);
	const FVector EndTrace = StartTrace + ShootDir * InstantConfig.WeaponRange;

	FHitResult Impact = WeaponTrace(StartTrace, EndTrace);
	if (Impact.bBlockingHit)
	{
		SpawnImpactEffects(Impact);
		SpawnTrailEffect(Impact.ImpactPoint);
	}
	else
	{
		SpawnTrailEffect(EndTrace);
	}
}

//************************************
// Method:    SpawnImpactEffects
// FullName:  AHitscanRangedWeapon::SpawnImpactEffects
// Access:    protected 
// Returns:   void
// Qualifier:
// Parameter: const FHitResult & Impact
//************************************
void AHitscanRangedWeapon::SpawnImpactEffects(const FHitResult& Impact)
{
	if (ImpactTemplate && Impact.bBlockingHit)
	{
		FHitResult UseImpact = Impact;

		// trace again to find component lost during replication
		if (!Impact.Component.IsValid())
		{
			const FVector StartTrace = Impact.ImpactPoint + Impact.ImpactNormal * 10.0f;
			const FVector EndTrace = Impact.ImpactPoint - Impact.ImpactNormal * 10.0f;
			FHitResult Hit = WeaponTrace(StartTrace, EndTrace);
			UseImpact = Hit;
		}

		FTransform const SpawnTransform(Impact.ImpactNormal.Rotation(), Impact.ImpactPoint);
		AVertImpactEffect* EffectActor = GetWorld()->SpawnActorDeferred<AVertImpactEffect>(ImpactTemplate, SpawnTransform);
		if (EffectActor)
		{
			EffectActor->SurfaceHit = UseImpact;
			UGameplayStatics::FinishSpawningActor(EffectActor, SpawnTransform);
		}
	}
}

//************************************
// Method:    SpawnTrailEffect
// FullName:  AHitscanRangedWeapon::SpawnTrailEffect
// Access:    protected 
// Returns:   void
// Qualifier:
// Parameter: const FVector & EndPoint
//************************************
void AHitscanRangedWeapon::SpawnTrailEffect(const FVector& EndPoint)
{
	if (TrailFX)
	{
		const FVector Origin = GetMuzzleLocation();

		UParticleSystemComponent* TrailPSC = UGameplayStatics::SpawnEmitterAtLocation(this, TrailFX, Origin);
		if (TrailPSC)
		{
			TrailPSC->SetVectorParameter(TrailTargetParam, EndPoint);
		}
	}
}

//************************************
// Method:    GetLifetimeReplicatedProps
// FullName:  AHitscanRangedWeapon::GetLifetimeReplicatedProps
// Access:    public 
// Returns:   void
// Qualifier: const
// Parameter: TArray< FLifetimeProperty > & OutLifetimeProps
//************************************
void AHitscanRangedWeapon::GetLifetimeReplicatedProps(TArray< FLifetimeProperty > & OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(AHitscanRangedWeapon, HitNotify, COND_SkipOwner);
}

//************************************
// Method:    GetWeaponType_Implementation
// FullName:  AHitscanRangedWeapon::GetWeaponType_Implementation
// Access:    virtual protected 
// Returns:   UClass*
// Qualifier: const
//************************************
UClass* AHitscanRangedWeapon::GetWeaponType_Implementation() const
{
	return AHitscanRangedWeapon::StaticClass();
}