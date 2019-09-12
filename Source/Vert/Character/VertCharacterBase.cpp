// Copyright Inside Out Games Ltd. 2017

#include "VertCharacterBase.h"
#include "HealthComponent.h"
#include "Engine/VertLevelScriptActor.h"
#include "Engine/VertPlayerController.h"

// Sets default values
AVertCharacterBase::AVertCharacterBase(const FObjectInitializer& ObjectInitializer) 
	: Super(ObjectInitializer),
	HealthComponent(CreateDefaultSubobject<UHealthComponent>(TEXT("HealthComponent")))
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	// Enable replication on the Sprite component so animations show up when networked
	GetMesh()->SetIsReplicated(true);
	bReplicates = true;
}

// Called when the game starts or when spawned
void AVertCharacterBase::BeginPlay()
{
	Super::BeginPlay();
	
	if (AVertLevelScriptActor* level = Cast<AVertLevelScriptActor>(GetWorld()->GetLevelScriptActor()))
	{
		GetCharacterMovement()->SetPlaneConstraintNormal(FVector::RightVector);
		GetCharacterMovement()->SetPlaneConstraintOrigin(level->GetPlaneConstraintOrigin());
		GetCharacterMovement()->SetPlaneConstraintEnabled(true);
		GetCharacterMovement()->SnapUpdatedComponentToPlane();
	}

	if (HealthComponent)
	{
		FScriptDelegate onDeath;
		onDeath.BindUFunction(this, TEXT("OnDeath"));
		HealthComponent->OnDeath.Add(onDeath);
	}

	if (CameraShouldFollow)
	{
		if (AGameModeBase* gameMode = GetWorld()->GetAuthGameMode())
		{
			if (AVertGameMode* vertGameMode = Cast<AVertGameMode>(gameMode))
			{
				vertGameMode->RegisterPlayerPawn(this);
			}
		}
	}
}

void AVertCharacterBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (EndPlayReason == EEndPlayReason::Destroyed)
	{
		if (UWorld* world = GetWorld())
		{
			if (AVertGameMode* gameMode = world->GetAuthGameMode<AVertGameMode>())
			{
				gameMode->UnregisterPlayerPawn(this);
			}
		}
	}

	GetWorld()->GetTimerManager().ClearAllTimersForObject(this);

	Super::EndPlay(EndPlayReason);
}

//************************************
// Method:    TakeDamage
// FullName:  AVertCharacterBase::TakeDamage
// Access:    virtual public 
// Returns:   float
// Qualifier:
// Parameter: float Damage
// Parameter: const FDamageEvent & DamageEvent
// Parameter: class AController * EventInstigator
// Parameter: class AActor * DamageCauser
//************************************
float AVertCharacterBase::TakeDamage(float Damage, const FDamageEvent& DamageEvent, class AController* EventInstigator, class AActor* damageCauser)
{
	if (AVertPlayerController* pc = Cast<AVertPlayerController>(Controller))
	{
		if (pc->HasGodMode() || IgnoredDamageTypes.Contains(DamageEvent.DamageTypeClass))
		{
			return 0.f;
		}
	}

	UE_LOG(LogVertCharacter, Log, TEXT("%s recieved damage from %s"), *GetName(), damageCauser ? *damageCauser->GetName() : TEXT("NULL"));

	float actualDamageTaken = Super::TakeDamage(Damage, DamageEvent, EventInstigator, damageCauser);
	actualDamageTaken = FMath::Max(1.f, actualDamageTaken - (actualDamageTaken * GlobalDamageResistance));

	OnDamageTaken.Broadcast(actualDamageTaken, DamageEvent.DamageTypeClass);

	APawn* pawnInstigator = EventInstigator ? EventInstigator->GetPawn() : nullptr;

	HealthComponent->DealDamage(actualDamageTaken, DamageEvent, pawnInstigator, damageCauser);
	return actualDamageTaken;
}

//************************************
// Method:    ApplyDamageMomentum
// FullName:  AVertCharacterBase::ApplyDamageMomentum
// Access:    virtual public 
// Returns:   void
// Qualifier:
// Parameter: float damageTaken
// Parameter: const FDamageEvent & damageEvent
// Parameter: APawn * pawnInstigator
// Parameter: AActor * damageCauser
//************************************
void AVertCharacterBase::ApplyDamageMomentum(float damageTaken, const FDamageEvent& damageEvent, APawn* pawnInstigator, AActor* damageCauser)
{
	if (!damageEvent.DamageTypeClass)
		UE_LOG(LogVertCharacter, Error, TEXT("Damage type not set on causer: %s"), damageCauser ? *damageCauser->GetName() : TEXT("NULL"));

	const UDamageType* const dmgTypeCDO = (damageEvent.DamageTypeClass) ? damageEvent.DamageTypeClass->GetDefaultObject<UDamageType>() : UDamageType::StaticClass()->GetDefaultObject<UDamageType>();
	AVertGameMode* gameMode = GetWorld()->GetAuthGameMode<AVertGameMode>();

	float knockbackAmount = 0.f;
	float hitstunTime = 0.f;
	float damageEventKnockback = 0.f;
	float damageEventKnockbackScaling = 0.f;
	float damageEventStunTime = 0.f;

	FVertPointDamageEvent* pointDamage = nullptr;
	FVertRadialDamageEvent* radialDamage = nullptr;
	if (damageEvent.IsOfType(FVertPointDamageEvent::ClassID))
	{
		pointDamage = (FVertPointDamageEvent*)&damageEvent;
		damageEventKnockback = pointDamage->Knockback;
		damageEventKnockbackScaling = pointDamage->KnockbackScaling;
		damageEventStunTime = pointDamage->StunTime;
	}
	else if (damageEvent.IsOfType(FVertRadialDamageEvent::ClassID))
	{
		radialDamage = (FVertRadialDamageEvent*)&damageEvent;
		damageEventKnockback = radialDamage->Knockback;
		damageEventKnockbackScaling = radialDamage->KnockbackScaling;
		damageEventStunTime = radialDamage->StunTime;
	}

	if (damageEventKnockback > 0.0001f || damageEventKnockbackScaling > 0.0001f)
	{
		static constexpr float scMass_Modifier = 1.4f;

		if(!ImmuneToKnockback)
			knockbackAmount = ((((((HealthComponent->GetCurrentDamageModifier() / 10) + ((HealthComponent->GetCurrentDamageModifier() * damageTaken) / 10)) * (200 / (GetCharacterMovement()->Mass + 100)) * scMass_Modifier) + 18) * damageEventKnockbackScaling) + damageEventKnockback) * gameMode->GetDamageRatio();
		
		if(!ImmuneToStun)
			ApplyDamageHitstun(FMath::FloorToInt(hitstunTime));
	}

	if (!ImmuneToKnockback)
	{
		float const impulseScale = dmgTypeCDO->DamageImpulse * knockbackAmount;
		UE_LOG(LogTemp, Warning, TEXT("damage impulse = %f | knockback amount = %f"), dmgTypeCDO->DamageImpulse, knockbackAmount);

		if ((impulseScale > 3.f) && (GetCharacterMovement() != NULL))
		{
			FHitResult hitInfo;
			FVector impulseDir;
			damageEvent.GetBestHitInfo(this, pawnInstigator, hitInfo, impulseDir);

			// Add a slight upwards rotation to the impulse direction if the character is on the floor
			// This allows the character to get launched without friction meddling (could try temporarily altering friction instead like in DashingComponent)
			if (IsGrounded())
			{
				impulseDir = gameMode->GetAmmendedLaunchAngle(impulseDir, knockbackAmount);
			}

			FVector impulse = impulseDir * impulseScale;
			const bool massIndependentImpulse = !dmgTypeCDO->bScaleMomentumByMass;

			// Limit Z momentum added if already going up faster than jump (to avoid blowing character way up into the sky)
			{
				FVector massScaledImpulse = impulse;
				if (!massIndependentImpulse && GetCharacterMovement()->Mass > SMALL_NUMBER)
				{
					massScaledImpulse = massScaledImpulse / GetCharacterMovement()->Mass;
				}
			}

			UE_LOG(LogVertCharacter, Log, TEXT("Applying knockback of %f magnitude."), impulseScale);
			UE_LOG(LogVertCharacter, Log, TEXT("impulseDirection = %f %f %f"), impulseDir.X, impulseDir.Y, impulseDir.Z);
			GetCharacterMovement()->AddImpulse(impulse, massIndependentImpulse);

			if (KnockbackTrailFX && knockbackAmount > KnockbackTrailTriggerMagnitude && (!KnockbackTrailPSC || KnockbackTrailPSC->IsPendingKill()))
			{
				KnockbackTrailPSC = UGameplayStatics::SpawnEmitterAttached(KnockbackTrailFX, GetRootComponent());
				if (KnockbackTrailPSC)
				{
					GetWorldTimerManager().SetTimer(mTimerHandle_TrailCheck, this, &AVertCharacterBase::CheckForEndKnockbackTrail, 0.5f, true);
				}
			}
		}
	}
}

//************************************
// Method:    CheckForEndKnockbackTrail
// FullName:  AVertCharacterBase::CheckForEndKnockbackTrail
// Access:    private 
// Returns:   void
// Qualifier:
//************************************
void AVertCharacterBase::CheckForEndKnockbackTrail()
{
	float velocityMagnitudeSqr = GetVelocity().SizeSquared();
	if (velocityMagnitudeSqr <= FMath::Square(KnockbackTrailDestroySpeed))
	{
		if (KnockbackTrailPSC && !KnockbackTrailPSC->IsPendingKill())
		{
			KnockbackTrailPSC->DeactivateSystem();
			KnockbackTrailPSC = NULL;
		}

		GetWorldTimerManager().ClearTimer(mTimerHandle_TrailCheck);
	}
}

//************************************
// Method:    InternalTakeRadialKnockback
// FullName:  AVertCharacter::InternalTakeRadialKnockback
// Access:    private 
// Returns:   void
// Qualifier:
// Parameter: FVertRadialDamageEvent & damageEvent
//************************************
void AVertCharacterBase::InternalTakeRadialKnockback(float Knockback, float StunTime, FVertRadialDamageEvent& RadialDamageEvent, class AController* EventInstigator, class AActor* KnockbackCauser)
{
	float ActualKnockback = Knockback;
	float ActualStunTime = StunTime;

	FVector ClosestHitLoc(0);

	// find closest component
	// @todo, something more accurate here to account for size of components, e.g. closest point on the component bbox?
	// @todo, sum up damage contribution to each component?
	float ClosestHitDistSq = MAX_FLT;
	for (int32 HitIdx = 0; HitIdx < RadialDamageEvent.ComponentHits.Num(); ++HitIdx)
	{
		FHitResult const& Hit = RadialDamageEvent.ComponentHits[HitIdx];
		float const DistSq = (Hit.ImpactPoint - RadialDamageEvent.Origin).SizeSquared();
		if (DistSq < ClosestHitDistSq)
		{
			ClosestHitDistSq = DistSq;
			ClosestHitLoc = Hit.ImpactPoint;
		}
	}

	const float RadialDamageScale = RadialDamageEvent.Params.GetDamageScale(FMath::Sqrt(ClosestHitDistSq));

	ActualKnockback = FMath::Lerp(RadialDamageEvent.Params.MinimumDamage, ActualKnockback, FMath::Max(0.f, RadialDamageScale));
	ActualStunTime = FMath::Lerp(RadialDamageEvent.Params.MinimumDamage, ActualStunTime, FMath::Max(0.f, RadialDamageScale));

	RadialDamageEvent.Knockback = ActualKnockback;
	RadialDamageEvent.StunTime = ActualStunTime;
}

//************************************
// Method:    FellOutOfWorld
// FullName:  AVertCharacterBase::FellOutOfWorld
// Access:    virtual public 
// Returns:   void
// Qualifier:
// Parameter: const class UDamageType & dmgType
//************************************
void AVertCharacterBase::FellOutOfWorld(const class UDamageType& dmgType)
{
	Die();
}

//************************************
// Method:    ApplyDamageHitstun
// FullName:  AVertCharacterBase::ApplyDamageHitstun
// Access:    virtual protected 
// Returns:   void
// Qualifier:
// Parameter: int32 hitstunFrames
//************************************
void AVertCharacterBase::ApplyDamageHitstun(float hitstunTime)
{
	FTimerManager& timerMan = GetWorld()->GetTimerManager();
	if (timerMan.IsTimerActive(mHitStunTimer))
	{
		timerMan.ClearTimer(mHitStunTimer);
	}

	// Always prioritize the latest hit
	timerMan.SetTimer(mHitStunTimer, hitstunTime, false);
}

//************************************
// Method:    IsInHitstun
// FullName:  AVertCharacterBase::IsInHitstun
// Access:    public 
// Returns:   bool
// Qualifier: const
//************************************
bool AVertCharacterBase::IsInHitstun() const
{
	return GetWorldTimerManager().IsTimerActive(mHitStunTimer);
}

//************************************
// Method:    Die
// FullName:  AVertCharacterBase::Die
// Access:    virtual public 
// Returns:   bool
// Qualifier:
// Parameter: float KillingDamage
// Parameter: const FDamageEvent & DamageEvent
// Parameter: class AController * Killer
// Parameter: class AActor * DamageCauser
//************************************
bool AVertCharacterBase::DieInternal(float KillingDamage, const FDamageEvent& DamageEvent, class AController* Killer, class AActor* DamageCauser)
{
	// if this is an environmental death then refer to the previous killer so that they receive credit (knocked into lava pits, etc)
	UDamageType const* const DamageType = DamageEvent.DamageTypeClass ? DamageEvent.DamageTypeClass->GetDefaultObject<UDamageType>() : GetDefault<UDamageType>();
	Killer = GetDamageInstigator(Killer, *DamageType);

	AController* const KilledPlayer = (Controller != NULL) ? Controller : Cast<AController>(GetOwner());
	GetWorld()->GetAuthGameMode<AVertGameMode>()->Killed(Killer, KilledPlayer, this, DamageType);

	NetUpdateFrequency = GetDefault<AVertGameMode>()->NetUpdateFrequency;
	GetCharacterMovement()->ForceReplicationUpdate();

	OnDeath(KillingDamage, DamageEvent, Killer ? Killer->GetPawn() : NULL, DamageCauser);
	return true;
}

//************************************
// Method:    CanDie
// FullName:  AVertCharacterBase::CanDie
// Access:    virtual public 
// Returns:   bool
// Qualifier: const
// Parameter: float KillingDamage
// Parameter: const FDamageEvent & DamageEvent
// Parameter: AController * Killer
// Parameter: AActor * DamageCauser
//************************************
bool AVertCharacterBase::CanDie() const
{
	if (mIsDying										// already dying
		|| IsPendingKill()								// already destroyed
		|| Role != ROLE_Authority						// not authority
		|| GetWorld()->GetAuthGameMode<AVertGameMode>() == NULL
		|| GetWorld()->GetAuthGameMode<AVertGameMode>()->GetMatchState() == MatchState::LeavingMap)	// level transition occurring
	{
		return false;
	}

	return true;
}

//************************************
// Method:    OnDeath
// FullName:  AVertCharacterBase::OnDeath
// Access:    virtual public 
// Returns:   void
// Qualifier:
// Parameter: float killingDamage
// Parameter: const FDamageEvent & damageEvent
// Parameter: APawn * pawnInstigator
// Parameter: AActor * damageCauser
//************************************
void AVertCharacterBase::OnDeath(float killingDamage, const FDamageEvent& damageEvent, APawn* pawnInstigator, AActor* damageCauser)
{
	if (mIsDying)
	{
		return;
	}

	bReplicateMovement = false;
	bTearOff = true;
	mIsDying = true;

	if (Role == ROLE_Authority)
	{
		HealthComponent->ReplicateHit(killingDamage, damageEvent, pawnInstigator, damageCauser, true);

		// play the force feedback effect on the client player controller
		APlayerController* PC = Cast<APlayerController>(Controller);
		if (PC && damageEvent.DamageTypeClass)
		{
			UVertDamageType *damageType = Cast<UVertDamageType>(damageEvent.DamageTypeClass->GetDefaultObject());
			if (damageType && damageType->KilledForceFeedback)
			{
				PC->ClientPlayForceFeedback(damageType->KilledForceFeedback, false, "Damage");
			}
		}
	}

	DetachFromControllerPendingDestroy();
	
	if (AVertGameMode* gameMode = GetWorld()->GetAuthGameMode<AVertGameMode>())
	{
		gameMode->UnregisterPlayerPawn(this);
	}

	UE_LOG(LogVertCharacter, Log, TEXT("Character [%s] killed by [%s]"), *GetName(), (pawnInstigator) ? *pawnInstigator->GetName() : TEXT("World"));

	DisableComponentsSimulatePhysics();
	SetActorHiddenInGame(true);
	SetActorEnableCollision(false);
	Destroy();
}

//************************************
// Method:    Suicide
// FullName:  AVertCharacterBase::Suicide
// Access:    virtual public 
// Returns:   void
// Qualifier:
//************************************
bool AVertCharacterBase::Suicide()
{
	if (!CanDie())
		return false;

	return KilledBy(this);
}

//************************************
// Method:    KilledBy
// FullName:  AVertCharacterBase::KilledBy
// Access:    virtual public 
// Returns:   void
// Qualifier:
// Parameter: class APawn * EventInstigator
//************************************
bool AVertCharacterBase::KilledBy(class APawn* EventInstigator)
{
	if (Role == ROLE_Authority && !mIsDying)
	{
		AController* Killer = NULL;
		if (EventInstigator != NULL)
		{
			Killer = EventInstigator->Controller;
		}

		return DieInternal(
			HealthComponent->GetCurrentDamageModifier(),
			HealthComponent->LastTakeHitInfo.GetDamageEvent(),
			Killer,
			(HealthComponent->LastTakeHitInfo.DamageCauser.IsValid()) ? HealthComponent->LastTakeHitInfo.DamageCauser.Get() : nullptr);
	}

	return false;
}

//************************************
// Method:    Die
// FullName:  AVertCharacterBase::Die
// Access:    public 
// Returns:   bool
// Qualifier:
//************************************
bool AVertCharacterBase::Die()
{
	if (!CanDie())
		return false;

	AController* killer = (HealthComponent->LastTakeHitInfo.PawnInstigator.IsValid()) ? HealthComponent->LastTakeHitInfo.PawnInstigator->GetController() : nullptr;
	if (!killer)
	{
		return Suicide();
	}

	float damage = HealthComponent->GetCurrentDamageModifier();
	const FDamageEvent& damageEvent = HealthComponent->LastTakeHitInfo.GetDamageEvent();
	AActor* damageCauser = (HealthComponent->LastTakeHitInfo.DamageCauser.IsValid()) ? HealthComponent->LastTakeHitInfo.DamageCauser.Get() : nullptr;

	return DieInternal(damage, damageEvent, killer, damageCauser);
}

//************************************
// Method:    SetRagdollPhysics
// FullName:  AVertCharacterBase::SetRagdollPhysics
// Access:    private 
// Returns:   void
// Qualifier:
//************************************
void AVertCharacterBase::SetRagdollPhysics()
{
	bool bInRagdoll = false;

	if (IsPendingKill())
	{
		bInRagdoll = false;
	}
	else if (!GetMesh() || !GetMesh()->GetPhysicsAsset())
	{
		bInRagdoll = false;
	}
	else
	{
		// initialize physics/etc
		GetMesh()->SetSimulatePhysics(true);
		GetMesh()->WakeAllRigidBodies();
		GetMesh()->bBlendPhysics = true;

		bInRagdoll = true;
	}

	GetCharacterMovement()->StopMovementImmediately();
	GetCharacterMovement()->DisableMovement();
	GetCharacterMovement()->SetComponentTickEnabled(false);

	if (!bInRagdoll)
	{
		// hide and set short lifespan
		TurnOff();
		SetActorHiddenInGame(true);
		SetLifeSpan(1.0f);
	}
	else
	{
		SetLifeSpan(10.0f);
	}

	mIsRagdolling = bInRagdoll;
}

//************************************
// Method:    IsMoving
// FullName:  AVertCharacterBase::IsMoving
// Access:    public 
// Returns:   bool
// Qualifier:
//************************************
bool AVertCharacterBase::IsMoving()
{
	FVector velocity = GetVelocity();
	return velocity.SizeSquared() > KINDA_SMALL_NUMBER;
}

//************************************
// Method:    CanComponentRecharge
// FullName:  AVertCharacterBase::CanComponentRecharge
// Access:    public 
// Returns:   bool
// Qualifier:
// Parameter: ERechargeRule rule
//************************************
bool AVertCharacterBase::CanComponentRecharge(ERechargeRule rule)
{
	return IsGrounded()
		|| rule == ERechargeRule::OnRechargeTimer
		|| rule == ERechargeRule::OnContactGroundOrLatchedAnywhere;

	return false;
}

//************************************
// Method:    GetFloorActor
// FullName:  AVertCharacterBase::GetFloorActor
// Access:    public 
// Returns:   AActor*
// Qualifier:
//************************************
AActor* AVertCharacterBase::GetFloorActor()
{
	if (IsGrounded())
	{
		static const FName scFloorTraceName(TEXT("CheckForFloorTrace"));

		FVector start = GetActorLocation();
		FVector end = start + ((-FVector::UpVector) * (GetCapsuleComponent()->GetScaledCapsuleHalfHeight()));
		FCollisionQueryParams params(scFloorTraceName, false, this);
		FHitResult hit;

		const bool didHit = GetWorld()->LineTraceSingleByChannel(hit, start, end, ECC_Visibility, params);

		if (didHit && hit.Actor.IsValid())
		{
			return hit.Actor.Get();
		}
	}

	return nullptr;
}