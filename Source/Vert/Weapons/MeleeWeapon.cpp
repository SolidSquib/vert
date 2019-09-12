// Copyright Inside Out Games Ltd. 2017

#include "MeleeWeapon.h"
#include "Engine/VertUtilities.h"

DECLARE_LOG_CATEGORY_CLASS(LogVertMeleeWeapon, Log, All);

AMeleeWeapon::AMeleeWeapon(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	bReplicates = true;
}

//************************************
// Method:    EndPlay
// FullName:  AMeleeWeapon::EndPlay
// Access:    virtual protected 
// Returns:   void
// Qualifier:
// Parameter: const EEndPlayReason::Type EndPlayReason
//************************************
void AMeleeWeapon::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (EndPlayReason == EEndPlayReason::Destroyed)
	{
		GetWorldTimerManager().ClearAllTimersForObject(this);
	}
}

//************************************
// Method:    Reset
// FullName:  AMeleeWeapon::Reset
// Access:    virtual protected 
// Returns:   void
// Qualifier:
//************************************
void AMeleeWeapon::Reset()
{
	ResetComboDepth();
	DisableTrace();

	Super::Reset();
}

//************************************
// Method:    StartAttacking
// FullName:  AMeleeWeapon::StartAttacking
// Access:    virtual protected 
// Returns:   void
// Qualifier:
//************************************
void AMeleeWeapon::StartAttacking()
{
	if (mIsAttackAnimationPlaying && ComboAttackAnims[mComboDepth].AllowAnimationClipping)
	{
		mClipAnimation = true;
		GetWorldTimerManager().SetTimer(mTimerHandle_AnimationClip, [this](void) {
			mClipAnimation = false;
		}, ComboAttackAnims[mComboDepth].ClipTime, false);
	}

	Super::StartAttacking();
}

//************************************
// Method:    ClientSimulateWeaponAttack_Implementation
// FullName:  AMeleeWeapon::ClientSimulateWeaponAttack_Implementation
// Access:    virtual protected 
// Returns:   void
// Qualifier:
//************************************
void AMeleeWeapon::ClientSimulateWeaponAttack_Implementation()
{
	Super::ClientSimulateWeaponAttack_Implementation();
}

//************************************
// Method:    ClientStopSimulateWeaponAttack_Implementation
// FullName:  AMeleeWeapon::ClientStopSimulateWeaponAttack_Implementation
// Access:    virtual protected 
// Returns:   void
// Qualifier:
//************************************
void AMeleeWeapon::ClientStopSimulateWeaponAttack_Implementation()
{
	Super::ClientStopSimulateWeaponAttack_Implementation();

	if (ArcPSC != NULL)
	{
		ArcPSC->DeactivateSystem();
		ArcPSC = NULL;
	}
}

//************************************
// Method:    AttackWithWeapon_Implementation
// FullName:  AMeleeWeapon::AttackWithWeapon_Implementation
// Access:    virtual protected 
// Returns:   bool
// Qualifier:
//************************************
bool AMeleeWeapon::AttackWithWeapon_Implementation()
{
	WeaponAttackFire();

	return true;
}

//************************************
// Method:    NotifyAttackAnimationActiveStarted_Implementation
// FullName:  AMeleeWeapon::NotifyAttackAnimationActiveStarted_Implementation
// Access:    virtual protected 
// Returns:   void
// Qualifier:
//************************************
void AMeleeWeapon::NotifyAttackAnimationActiveStarted_Implementation()
{
	EnableTrace();
}

//************************************
// Method:    NotifyAttackAnimationActiveEnded_Implementation
// FullName:  AMeleeWeapon::NotifyAttackAnimationActiveEnded_Implementation
// Access:    virtual protected 
// Returns:   void
// Qualifier:
//************************************
void AMeleeWeapon::NotifyAttackAnimationActiveEnded_Implementation()
{
	DisableTrace();
}

//************************************
// Method:    EnableTrace
// FullName:  AMeleeWeapon::EnableTrace
// Access:    protected 
// Returns:   void
// Qualifier:
//************************************
void AMeleeWeapon::EnableTrace()
{
	if (AttackStartFX)
	{
		UParticleSystemComponent* AttackStartPSC = UGameplayStatics::SpawnEmitterAtLocation(this, AttackStartFX, WeaponMesh->GetSocketLocation(FXSocketTop));
	}

	// Start tracing for hits!
	static constexpr float scTraceDelta = 1 / 60.f;
	GetWorldTimerManager().SetTimer(mTimerHandle_TraceTimer, [this](void) {
		BoxTraceForHits();
	}, scTraceDelta, true, 0.f);

	// Spawn arc particle FX
	if (ArcFX)
	{
		if (ArcPSC == NULL)
		{
			ArcPSC = UGameplayStatics::SpawnEmitterAttached(ArcFX, WeaponMesh, FXSocketTop);
		}
	}
}

//************************************
// Method:    DisableTrace
// FullName:  AMeleeWeapon::DisableTrace
// Access:    protected 
// Returns:   void
// Qualifier:
//************************************
void AMeleeWeapon::DisableTrace()
{
	if (AttackEndFX)
	{
		UParticleSystemComponent* AttackEndPSC = UGameplayStatics::SpawnEmitterAtLocation(this, AttackEndFX, WeaponMesh->GetSocketLocation(FXSocketTop));
	}

	if (ArcPSC != NULL)
	{
		ArcPSC->DeactivateSystem();
		ArcPSC = NULL;
	}

	GetWorldTimerManager().ClearTimer(mTimerHandle_TraceTimer);
}

//************************************
// Method:    NotifyAttackAnimationEnded_Implementation
// FullName:  AMeleeWeapon::NotifyAttackAnimationEnded_Implementation
// Access:    virtual protected 
// Returns:   void
// Qualifier:
//************************************
void AMeleeWeapon::NotifyAttackAnimationEnded_Implementation()
{
	Super::NotifyAttackAnimationEnded_Implementation();

	GetWorldTimerManager().SetTimer(mTimerHandle_ComboDeathTime, [this](void) {
		mComboInProgress = false;
		mComboDepth = 0;
		mLastAttackUsed = -1;
	}, TimeForComboToDie, false);

	if (mClipAnimation)
	{
		WeaponAttackFire();
		mClipAnimation = false;
	}

	mIsAttackAnimationPlaying = false;
}

//************************************
// Method:    GetWeaponType_Implementation
// FullName:  AMeleeWeapon::GetWeaponType_Implementation
// Access:    virtual protected 
// Returns:   UClass*
// Qualifier: const
//************************************
UClass* AMeleeWeapon::GetWeaponType_Implementation() const
{
	return AMeleeWeapon::StaticClass();
}

//************************************
// Method:    BoxTraceForHit
// FullName:  AMeleeWeapon::BoxTraceForHit
// Access:    protected 
// Returns:   FHitResult
// Qualifier: const
//************************************
TArray<FHitResult> AMeleeWeapon::BoxTraceForHits()
{
	FVector start = FVector::ZeroVector;
	FQuat rotation = FQuat::Identity;
	WeaponMesh->GetSocketWorldLocationAndRotation(BoxTrace.TraceStartSocket, start, rotation);
	FVector direction = rotation.Vector().GetSafeNormal();
	
	static FName WeaponFireTag = FName(TEXT("WeaponTrace"));

	// Perform trace to retrieve hit info
	FCollisionQueryParams TraceParams(WeaponFireTag, true, Instigator);
	TraceParams.bTraceAsyncScene = true;
	TraceParams.bReturnPhysicalMaterial = true;
	FVector end = start + (direction*BoxTrace.TraceLength);

	TArray<FHitResult> hits;
	const bool didHit = GetWorld()->SweepMultiByChannel(hits, start, end, (end - start).Rotation().Quaternion(), ECC_WeaponTracePenetrate, FCollisionShape::MakeBox(BoxTrace.BoxTraceHalfExtent), TraceParams);

	for (auto hit : hits)
	{
		if (hit.Actor.IsValid() && mCurrentHitActors.Find(hit.Actor) == INDEX_NONE)
		{
			if (!mDidHit && Cast<AVertCharacter>(hit.Actor.Get()))
			{
				mDidHit = true;
			}

			mCurrentHitActors.Add(hit.Actor);

			FVertPointDamageEvent PointDmg;
			PointDmg.DamageTypeClass = WeaponConfig.DamageType;
			PointDmg.HitInfo = hit;
			PointDmg.ShotDirection = (hit.TraceEnd - hit.TraceStart).GetSafeNormal();
			PointDmg.Damage = WeaponConfig.BaseDamage + GetBonusDamage();
			PointDmg.Knockback = WeaponConfig.BaseKnockback + GetBonusKnockback();
			PointDmg.KnockbackScaling = WeaponConfig.KnockbackScaling;
			PointDmg.StunTime = WeaponConfig.StunTime;
			hit.Actor->TakeDamage(
				WeaponConfig.BaseDamage + GetBonusDamage(),
				PointDmg,
				Instigator ? Instigator->GetController() : nullptr,
				Instigator ? Instigator : nullptr
			);
		}
	}

#if ENABLE_DRAW_DEBUG
	if (ShowDebug)
	{
		// Red up to the blocking hit, green thereafter
		UVertUtilities::DrawDebugBoxTraceMulti(GetWorld(), start, end, BoxTrace.BoxTraceHalfExtent, (start - end).Rotation(), EDrawDebugTrace::ForDuration, didHit, hits, FLinearColor::Green, FLinearColor::Red, 3);
	}
#endif

	return hits;
}

//************************************
// Method:    WeaponAttackFire
// FullName:  AMeleeWeapon::WeaponAttackFire
// Access:    virtual protected 
// Returns:   void
// Qualifier:
// Parameter: float ratio
//************************************
void AMeleeWeapon::WeaponAttackFire(float ratio /*= 0.f*/)
{
	mCurrentHitActors.Empty();

	if (!UseComboAnimations)
	{
		Super::WeaponAttackFire(ratio);
	}
	else
	{
		if (!ManualComboManagement)
		{
			if (mDidHit)
			{
				if (mLastAttackUsed == (ComboAttackAnims.Num() - 1))
				{
					mComboDepth = 0;
				}
				else
				{
					mComboDepth = mLastAttackUsed + 1;
				}				
			}
			else
			{
				if (mLastAttackUsed == AnimLoopRange)
				{
					mComboDepth = 0;
				}
				else
				{
					mComboDepth = mLastAttackUsed + 1;
				}
			}

			IsWaitingForAttackAnimEnd = ComboAttackAnims[mComboDepth].AttackAnim.HoldForAnimationEnd;
			Delegate_OnWeaponAttackFire.Broadcast(ComboAttackAnims[mComboDepth].AttackAnim, ratio);
			mLastAttackUsed = mComboDepth;
		}
		else
		{
			mComboDepth = FMath::Clamp(mComboDepth, 0, ComboAttackAnims.Num() - 1);
			IsWaitingForAttackAnimEnd = ComboAttackAnims[mComboDepth].AttackAnim.HoldForAnimationEnd;
			Delegate_OnWeaponAttackFire.Broadcast(ComboAttackAnims[mComboDepth].AttackAnim, ratio);
		}
	}

	mIsAttackAnimationPlaying = true;
	mDidHit = false;
}

//************************************
// Method:    IncrementComboDepth
// FullName:  AMeleeWeapon::IncrementComboDepth
// Access:    protected 
// Returns:   void
// Qualifier:
//************************************
void AMeleeWeapon::IncrementComboDepth()
{
	mComboDepth = FMath::Max(mComboDepth + 1, ComboAttackAnims.Num() - 1);
}

//************************************
// Method:    DecrementComboDepth
// FullName:  AMeleeWeapon::DecrementComboDepth
// Access:    protected 
// Returns:   void
// Qualifier:
//************************************
void AMeleeWeapon::DecrementComboDepth()
{
	mComboDepth = FMath::Min(0, mComboDepth-1);
}

//************************************
// Method:    ResetComboDepth
// FullName:  AMeleeWeapon::ResetComboDepth
// Access:    protected 
// Returns:   void
// Qualifier:
//************************************
void AMeleeWeapon::ResetComboDepth()
{
	mComboDepth = 0;
}