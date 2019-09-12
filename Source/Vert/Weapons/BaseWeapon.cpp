// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "BaseWeapon.h"
#include "Particles/ParticleSystemComponent.h"
#include "Engine/VertPlayerState.h"

DEFINE_LOG_CATEGORY(LogVertBaseWeapon);

ABaseWeapon::ABaseWeapon(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	WeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh"));
	WeaponMesh->MeshComponentUpdateFlag = EMeshComponentUpdateFlag::OnlyTickPoseWhenRendered;
	WeaponMesh->bReceivesDecals = false;
	WeaponMesh->CastShadow = true;
	WeaponMesh->SetCollisionObjectType(ECC_WorldDynamic);
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	WeaponMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
	WeaponMesh->SetMobility(EComponentMobility::Movable);
	WeaponMesh->CanCharacterStepUpOn = ECanBeCharacterBase::ECB_No;
	WeaponMesh->bGenerateOverlapEvents = true;
	WeaponMesh->SetCanEverAffectNavigation(false);
	RootComponent = WeaponMesh;

	InteractionSphere->SetupAttachment(RootComponent);
	InteractionSphere->SetCanEverAffectNavigation(false);

	IsEquipped = false;
	WantsToFire = false;
	PendingReload = false;
	PendingEquip = false;
	mAttackSpent = false;
	mCombatIdle = false;
	OverrideAnimCompleteNotify = false;
	mCurrentState = EWeaponState::PassiveIdle;

	CurrentAmmo = 0;
	CurrentAmmoInClip = 0;
	BurstCounter = 0;
	mLastFireTime = 0.0f;

	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PrePhysics;
	SetRemoteRoleForBackwardsCompat(ROLE_SimulatedProxy);
	bReplicates = true;
	bNetUseOwnerRelevancy = true;
}

//************************************
// Method:    PostInitializeComponents
// FullName:  ABaseWeapon::PostInitializeComponents
// Access:    virtual public 
// Returns:   void
// Qualifier:
//************************************
void ABaseWeapon::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	OnPoolBeginPlay.AddDynamic(this, &ABaseWeapon::PoolBeginPlay);
	OnPoolEndPlay.AddDynamic(this, &ABaseWeapon::PoolEndPlay);
	
	WeaponMesh->OnComponentBeginOverlap.AddDynamic(this, &ABaseWeapon::OnBeginOverlap);
	WeaponMesh->OnComponentHit.AddDynamic(this, &ABaseWeapon::OnComponentHit);
}

//************************************
// Method:    BeginPlay
// FullName:  ABaseWeapon::BeginPlay
// Access:    virtual protected 
// Returns:   void
// Qualifier:
//************************************
void ABaseWeapon::BeginPlay()
{
	Super::BeginPlay();

	if (WeaponConfig.InitialClips > 0)
	{
		CurrentAmmoInClip = WeaponConfig.AmmoPerClip;
		CurrentAmmo = WeaponConfig.AmmoPerClip * WeaponConfig.InitialClips;
	}

	DisableWeaponPhysics();
	EnableInteractionDetection();

	if (IsAttachedToPawn())
		DetachMeshFromPawn();

	// Set up collision and physics settings
	if (AVertLevelScriptActor* level = Cast<AVertLevelScriptActor>(GetWorld()->GetLevelScriptActor()))
	{
		WeaponMesh->SetEnableGravity(true);
		WeaponMesh->SetConstraintMode(EDOFMode::XZPlane);
	}
}

//************************************
// Method:    EndPlay
// FullName:  ABaseWeapon::EndPlay
// Access:    virtual protected 
// Returns:   void
// Qualifier:
// Parameter: const EEndPlayReason::Type EndPlayReason
//************************************
void ABaseWeapon::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
	GetWorldTimerManager().ClearAllTimersForObject(this);
}

//************************************
// Method:    PoolBeginPlay
// FullName:  ABaseWeapon::PoolBeginPlay
// Access:    protected 
// Returns:   void
// Qualifier:
//************************************
void ABaseWeapon::PoolBeginPlay()
{
	if (WeaponConfig.InitialClips > 0)
	{
		CurrentAmmoInClip = WeaponConfig.AmmoPerClip;
		CurrentAmmo = WeaponConfig.AmmoPerClip * WeaponConfig.InitialClips;
	}

	DisableWeaponPhysics();
	EnableInteractionDetection();

	WeaponMesh->SetConstraintMode(EDOFMode::XZPlane);
	
	if (IsAttachedToPawn())
		DetachMeshFromPawn();

	GetWorldTimerManager().ClearAllTimersForObject(this);

	StartDespawnTimer();
}

//************************************
// Method:    PoolEndPlay
// FullName:  ABaseWeapon::PoolEndPlay
// Access:    protected 
// Returns:   void
// Qualifier:
//************************************
void ABaseWeapon::PoolEndPlay()
{
	GetWorldTimerManager().ClearAllTimersForObject(this);
	Reset();
}

//************************************
// Method:    StartDespawnTimer
// FullName:  ABaseWeapon::StartDespawnTimer
// Access:    protected 
// Returns:   void
// Qualifier:
//************************************
void ABaseWeapon::StartDespawnTimer()
{
	GetWorldTimerManager().SetTimer(mTimerHandle_Despawn, this, &ABaseWeapon::PrepareForDespawn, DespawnTriggerTime, false);
}

//************************************
// Method:    PrepareForDespawn
// FullName:  ABaseWeapon::PrepareForDespawn
// Access:    protected 
// Returns:   void
// Qualifier:
//************************************
void ABaseWeapon::PrepareForDespawn()
{
	GetWorldTimerManager().SetTimer(mTimerHandle_DespawnFlash, this, &ABaseWeapon::DespawnFlash, FlashSpeed, true);
	GetWorldTimerManager().SetTimer(mTimerHandle_DespawnFinish, this, &ABaseWeapon::DisableAndDestroy, FlashForTimeBeforeDespawning, false);
}

//************************************
// Method:    DespawnFlash
// FullName:  ABaseWeapon::DespawnFlash
// Access:    protected 
// Returns:   void
// Qualifier:
//************************************
void ABaseWeapon::DespawnFlash()
{
	if (WeaponMesh->IsVisible())
		WeaponMesh->SetVisibility(false, true);
	else
		WeaponMesh->SetVisibility(true, true);
}

//************************************
// Method:    DisableAndDestroy
// FullName:  ABaseWeapon::DisableAndDestroy
// Access:    protected 
// Returns:   void
// Qualifier:
//************************************
void ABaseWeapon::DisableAndDestroy()
{
	GetWorldTimerManager().ClearAllTimersForObject(this);
	WeaponMesh->SetVisibility(true, true);

	if (DespawnFX)
	{
		UParticleSystemComponent* psc = UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), DespawnFX, GetActorTransform());
	}

	ReturnToPool();
}

//************************************
// Method:    CancelDespawn
// FullName:  ABaseWeapon::CancelDespawn
// Access:    protected 
// Returns:   void
// Qualifier:
//************************************
void ABaseWeapon::CancelDespawn()
{
	if (GetWorldTimerManager().IsTimerActive(mTimerHandle_Despawn))
		GetWorldTimerManager().ClearTimer(mTimerHandle_Despawn);

	if (GetWorldTimerManager().IsTimerActive(mTimerHandle_DespawnFlash))
		GetWorldTimerManager().ClearTimer(mTimerHandle_DespawnFlash);

	if (GetWorldTimerManager().IsTimerActive(mTimerHandle_DespawnFinish))
		GetWorldTimerManager().ClearTimer(mTimerHandle_DespawnFinish);

	WeaponMesh->SetVisibility(true, true);
}

//************************************
// Method:    Destroyed
// FullName:  ABaseWeapon::Destroyed
// Access:    virtual public 
// Returns:   void
// Qualifier:
//************************************
void ABaseWeapon::Destroyed()
{
	Super::Destroyed();

	StopSimulatingWeaponAttack();
}

//************************************
// Method:    NotifyEquipAnimationEnded
// FullName:  ABaseWeapon::NotifyEquipAnimationEnded
// Access:    public 
// Returns:   void
// Qualifier:
//************************************
void ABaseWeapon::NotifyEquipAnimationEnded()
{
	if (PendingEquip)
	{
		AttachMeshToPawn();

		PendingEquip = false;
		IsEquipped = true;

		DetermineWeaponState();

		WeaponReadyFromPickup();
	}
}

//************************************
// Method:    OnPickup
// FullName:  ABaseWeapon::OnPickup
// Access:    public 
// Returns:   void
// Qualifier:
// Parameter: AVertCharacter * NewOwner
//************************************
void ABaseWeapon::Pickup(AVertCharacter* NewOwner)
{
	CancelDespawn();
	DisableWeaponPhysics();

	if (NewOwner)
	{
		if (MyPawn)
		{
			MyPawn->MoveIgnoreActorRemove(this);
			WeaponMesh->MoveIgnoreActors.Empty(1);
		}
		NewOwner->MoveIgnoreActorAdd(this);
		WeaponMesh->MoveIgnoreActors.Add(NewOwner);
	}	

	WeaponMesh->SetSimulatePhysics(false);
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);

	SetOwningPawn(NewOwner);
	AttachMeshToPawn();
}

//************************************
// Method:    StartEquipping
// FullName:  ABaseWeapon::StartEquipping
// Access:    virtual public 
// Returns:   void
// Qualifier:
//************************************
void ABaseWeapon::StartEquipping()
{
	PendingEquip = true;
	DetermineWeaponState();

	if (!OverrideAnimCompleteNotify)
	{
		PlayWeaponAnimation(EquipAnim);
	}
	else
	{
		NotifyEquipAnimationEnded();
	}
}

//************************************
// Method:    OnDrop
// FullName:  ABaseWeapon::OnDrop
// Access:    virtual public 
// Returns:   void
// Qualifier:
//************************************
void ABaseWeapon::OnDrop()
{
	if (IsAttachedToPawn())
	{
		DetachMeshFromPawn();
		IsEquipped = false;
		StopAttacking(true);

		if (PendingReload)
		{
			StopWeaponAnimation(ReloadAnim);
			PendingReload = false;
		}

		if (PendingEquip)
		{
			StopWeaponAnimation(EquipAnim);
			PendingEquip = false;
		}

		Reset();
		DetermineWeaponState();
	}
}

//************************************
// Method:    AttachMeshToPawn
// FullName:  ABaseWeapon::AttachMeshToPawn
// Access:    protected 
// Returns:   void
// Qualifier:
//************************************
void ABaseWeapon::AttachMeshToPawn()
{
	if (MyPawn)
	{
		// Remove and hide both first and third person meshes
		DetachMeshFromPawn();

		// For locally controller players we attach both weapons and let the bOnlyOwnerSee, bOwnerNoSee flags deal with visibility.
		FName AttachPoint = MyPawn->ItemHandSocket;
		if (MyPawn->IsLocallyControlled() == true)
		{
			USkeletalMeshComponent* PawnMesh = MyPawn->GetMesh();
			AttachToComponent(PawnMesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale, AttachPoint);
		}
	}
}

//************************************
// Method:    DetachMeshFromPawn
// FullName:  ABaseWeapon::DetachMeshFromPawn
// Access:    protected 
// Returns:   void
// Qualifier:
//************************************
void ABaseWeapon::DetachMeshFromPawn()
{
	DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
}

//************************************
// Method:    Interact
// FullName:  ABaseWeapon::Interact
// Access:    virtual public 
// Returns:   void
// Qualifier:
// Parameter: const TWeakObjectPtr<class UCharacterInteractionComponent> & instigator
//************************************
void ABaseWeapon::Interact(const TWeakObjectPtr<class UCharacterInteractionComponent>& instigator)
{
	WeaponInteract(instigator.Get(), instigator->GetCharacterOwner());

	Super::Interact(instigator);
}

//************************************
// Method:    FireWeapon_Implementation
// FullName:  ABaseWeapon::FireWeapon_Implementation
// Access:    public 
// Returns:   bool
// Qualifier:
//************************************
bool ABaseWeapon::AttackWithWeapon_Implementation()
{
	UE_LOG(LogVertBaseWeapon, Fatal, TEXT("Call to pure virtual function ABaseWeapon::AttackWithWeapon_Implementation not allowed"));
	return false;
}

//************************************
// Method:    WeaponInteract_Implementation
// FullName:  ABaseWeapon::WeaponInteract_Implementation
// Access:    public 
// Returns:   void
// Qualifier:
// Parameter: UCharacterInteractionComponent * interactionComponent
// Parameter: AVertCharacter * character
//************************************
void ABaseWeapon::WeaponInteract_Implementation(UCharacterInteractionComponent* interactionComponent, AVertCharacter* character)
{
	if (interactionComponent != nullptr)
	{
		if (interactionComponent->GetHeldInteractive() != this)
		{
			FVector localOffset = FVector::ZeroVector;

			if (interactionComponent->HoldInteractive(this, localOffset, false))
			{
				DisableInteractionDetection();
				Instigator = character;

				UE_LOG(LogVertBaseWeapon, Log, TEXT("Weapon %s picked up by player %s"), *GetName(), *Instigator->GetName());
			}
		}
		else
			ThrowWeapon();
	}
}

//************************************
// Method:    ThrowWeapon_Implementation
// FullName:  ABaseWeapon::ThrowWeapon_Implementation
// Access:    public 
// Returns:   void
// Qualifier:
//************************************
void ABaseWeapon::ThrowWeapon_Implementation()
{
	if (AVertCharacter* character = Cast<AVertCharacter>(Instigator))
	{
		FVector launchDirection = UVertUtilities::SnapVectorToAngle(character->GetAxisPostisions().GetPlayerLeftThumbstickDirection(), 180.f);

		if (WeaponMesh)
		{
			character->GetInteractionComponent()->ThrowInteractive(WeaponMesh, FVector::ZeroVector, FVector::ZeroVector);

			if (launchDirection == FVector::ZeroVector)
			{
				EnableWeaponPhysics(true, false);
			}
			else
			{
				// Move it out of the player's collision
				SetActorLocation(GetActorLocation() + (launchDirection*character->GetCapsuleComponent()->GetScaledCapsuleRadius() + 10));
				InitThrowVelocity(launchDirection);
			}

			EnableInteractionDetection();
		}		

		UE_LOG(LogVertBaseWeapon, Log, TEXT("Weapon %s thrown by player %s"), *GetName(), *character->GetName());
		StartDespawnTimer();
	}
}

//************************************
// Method:    InitThrowVelocity
// FullName:  ABaseWeapon::InitThrowVelocity
// Access:    protected 
// Returns:   void
// Qualifier:
// Parameter: const FVector & throwDirection
//************************************
void ABaseWeapon::InitThrowVelocity(const FVector& throwDirection)
{
		static constexpr float scGravityFreeTime = .2f;

		SetActorRotation(throwDirection.X > 0 ? (-FVector::RightVector).Rotation().Quaternion() : FVector::RightVector.Rotation().Quaternion());

		EnableWeaponPhysics(true, false, throwDirection*(MyPawn ? MyPawn->ThrowForce : 0));

		if (FMath::Abs(throwDirection.X) > KINDA_SMALL_NUMBER)
		{
			WeaponMesh->SetEnableGravity(false);

			GetWorldTimerManager().SetTimer(mTimerHandle_ThrowGravityTimer, [this](void) {
				WeaponMesh->SetEnableGravity(true);
			}, scGravityFreeTime, false);
		}
}

//************************************
// Method:    EnableWeaponPhysics
// FullName:  ABaseWeapon::EnableWeaponPhysics
// Access:    public 
// Returns:   void
// Qualifier:
//************************************
void ABaseWeapon::EnableWeaponPhysics(bool simulating /*= false*/, bool shouldBlockPawn /*= true*/, const FVector& initialImpulse /*= FVector::ZeroVector*/, float blockDelay /*= 0.f*/)
{
	if (simulating)
	{
		if (blockDelay > KINDA_SMALL_NUMBER)
		{
			GetWorldTimerManager().SetTimer(mTimerHandle_BlockDelay, [shouldBlockPawn, this](void) {
				WeaponMesh->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
				WeaponMesh->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
				WeaponMesh->SetCollisionResponseToChannel(ECC_Pawn, shouldBlockPawn ? ECR_Block : ECR_Overlap);
			}, blockDelay, false);
		}
		else
		{
			WeaponMesh->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
			WeaponMesh->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
			WeaponMesh->SetCollisionResponseToChannel(ECC_Pawn, shouldBlockPawn ? ECR_Block : ECR_Overlap);
		}
		
		WeaponMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		WeaponMesh->SetSimulatePhysics(true);
		WeaponMesh->SetPhysicsLinearVelocity(initialImpulse);
		//WeaponMesh->AddImpulse(initialImpulse);
	}
	else
	{
		WeaponMesh->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
		WeaponMesh->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
		FTimerHandle timer;
		GetWorldTimerManager().SetTimer(timer, [shouldBlockPawn, this](void) {
			WeaponMesh->SetCollisionResponseToChannel(ECC_Pawn, shouldBlockPawn ? ECR_Block : ECR_Overlap);
		}, 0.1f, false);
	}
}

//************************************
// Method:    DisableWeaponPhysics
// FullName:  ABaseWeapon::DisableWeaponPhysics
// Access:    public 
// Returns:   void
// Qualifier:
//************************************
void ABaseWeapon::DisableWeaponPhysics()
{
	WeaponMesh->SetSimulatePhysics(false);
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);

	WeaponMesh->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Ignore);
	WeaponMesh->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Ignore);
	WeaponMesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
}

//************************************
// Method:    OnBeginOverlap_Implementation
// FullName:  ABaseWeapon::OnBeginOverlap_Implementation
// Access:    public 
// Returns:   void
// Qualifier:
// Parameter: UPrimitiveComponent * overlappedComponent
// Parameter: AActor * otherActor
// Parameter: UPrimitiveComponent * otherComp
// Parameter: int32 otherBodyIndex
// Parameter: bool fromSweep
// Parameter: const FHitResult & sweepResult
//************************************
void ABaseWeapon::OnBeginOverlap_Implementation(UPrimitiveComponent* overlappedComponent, AActor* otherActor, UPrimitiveComponent* otherComp, int32 otherBodyIndex, bool fromSweep, const FHitResult& sweepResult)
{
	if (otherActor && WeaponMesh->GetPhysicsLinearVelocity().SizeSquared() > FMath::Square(100.f) && mIgnoreThrowDamage.Find(otherActor) == INDEX_NONE)
	{
		UE_LOG(LogVertBaseWeapon, Log, TEXT("Overlapped with actor %s"), *otherActor->GetName());

		FHitResult hit;
		FCollisionQueryParams params;
		const bool didHit = GetWorld()->SweepSingleByObjectType(hit, GetActorLocation(), otherActor->GetActorLocation(), FQuat::Identity, ECC_Pawn, FCollisionShape::MakeSphere(20.f), params);

		if (didHit)
		{
			float velocity = WeaponMesh->GetPhysicsLinearVelocity().Size();
			float damageModifier = FMath::Clamp((velocity / 3500.f), 0.f, 1.f); // apply max damage with a velocity of 3500.f, reduce based on percentage;

			if (damageModifier > 0.2f)
			{
				FVertPointDamageEvent PointDmg;
				PointDmg.DamageTypeClass = ThrowingConfig.DamageType;
				PointDmg.HitInfo = hit;
				PointDmg.ShotDirection = -hit.ImpactNormal;
				PointDmg.Damage = ThrowingConfig.BaseDamage * damageModifier;
				PointDmg.Knockback = ThrowingConfig.Knockback * damageModifier;
				PointDmg.KnockbackScaling = ThrowingConfig.KnockbackScaling;
				PointDmg.StunTime = ThrowingConfig.StunTime;
				otherActor->TakeDamage(PointDmg.Damage, PointDmg, Instigator ? Instigator->Controller : nullptr, this);
			}

			// Apply bounce from hit if this weapon isn't set to 'pierce'
			if (!ThrowingConfig.Piercing)
			{
				FVector bounceDirection = (hit.ImpactNormal + FVector::UpVector).GetSafeNormal();
				float magnitude = WeaponMesh->GetPhysicsLinearVelocity().Size() * 0.5;
				WeaponMesh->SetPhysicsLinearVelocity(bounceDirection*magnitude);
				OnWeaponHit(otherActor, otherComp, hit.ImpactNormal);
			}
			
			if (AVertCharacter* vertCharacter = Cast<AVertCharacter>(otherActor))
			{
				vertCharacter->Dislodge();
			}

			// Ignore the actor that just took damage to avoid being hit multiple times in quick succession.
			mIgnoreThrowDamage.Add(otherActor);
			FTimerHandle timerHandle;
			GetWorldTimerManager().SetTimer(timerHandle, [otherActor, this](void) {

				if (!otherActor || !this)
					return;

				if (mIgnoreThrowDamage.Find(otherActor) != INDEX_NONE)
				{
					mIgnoreThrowDamage.Remove(otherActor);
				}
			}, .5f, false);
		}

		if (ThrowImpactSound)
		{
			UAkGameplayStatics::PostEvent(ThrowImpactSound, otherActor, false);
		}
	}
}

void ABaseWeapon::OnComponentHit(UPrimitiveComponent* hitComponent, AActor* otherActor, UPrimitiveComponent* otherComp, FVector normalImpulse, const FHitResult& hit)
{
	OnWeaponHit(otherActor, otherComp, hit.ImpactNormal);

	if (ThrowImpactSound)
	{
		UAkGameplayStatics::PostEventAtLocation(ThrowImpactSound, hit.ImpactPoint, FQuat::Identity.Rotator(), "", this);
	}
}

//************************************
// Method:    StartAttacking
// FullName:  ABaseWeapon::StartAttacking
// Access:    virtual public 
// Returns:   void
// Qualifier:
//************************************
void ABaseWeapon::StartAttacking()
{
	if (Role < ROLE_Authority)
	{
		ServerStartAttacking();
	}

	if (CurrentAmmoInClip <= 0 && CanReload())
	{
		StartReload();
	}
	else if (!WantsToFire)
	{
		WantsToFire = true;
		DetermineWeaponState();
	}

	OnStartAttacking();
}

//************************************
// Method:    DashAttack_Implementation
// FullName:  ABaseWeapon::DashAttack_Implementation
// Access:    public 
// Returns:   bool
// Qualifier:
//************************************
bool ABaseWeapon::DashAttack_Implementation()
{
	return false;
}

//************************************
// Method:    StopAttacking
// FullName:  ABaseWeapon::StopAttacking
// Access:    virtual public 
// Returns:   void
// Qualifier:
//************************************
void ABaseWeapon::StopAttacking(bool forced /*= false*/)
{
	if (Role < ROLE_Authority)
	{
		ServerStopAttacking(forced);
	}

	if (WantsToFire)
	{
		WantsToFire = false;
		const float GameTime = GetWorld()->GetTimeSeconds();
		if (mLastFireTime > 0 && WeaponConfig.TimeBetweenShots > 0.0f &&
			mLastFireTime + WeaponConfig.TimeBetweenShots > GameTime)
		{
			GetWorldTimerManager().SetTimer(mTimerHandle_NonAutoTriggerDelay, [this](void) {
				mAttackSpent = false;
			}, mLastFireTime + WeaponConfig.TimeBetweenShots - GameTime, false);
		}
		else
		{
			mAttackSpent = false;
		}

		static constexpr float scCombatIdleTime = 4.f;

		mCombatIdle = true;
		GetWorldTimerManager().SetTimer(mTimerHandle_CombatIdle, [this](void) {
			mCombatIdle = false;
			DetermineWeaponState();
		}, scCombatIdleTime, false);
		DetermineWeaponState();
	}

	OnStopAttacking(forced);
}

//************************************
// Method:    NotifyAttackAnimationActiveStarted_Implementation
// FullName:  ABaseWeapon::NotifyAttackAnimationActiveStarted_Implementation
// Access:    public 
// Returns:   void
// Qualifier:
//************************************
void ABaseWeapon::NotifyAttackAnimationActiveStarted_Implementation()
{
	// Do nothing, this exists purely to avoid having to cast up to a melee weapon
}

//************************************
// Method:    NotifyAttackAnimationActiveEnded_Implementation
// FullName:  ABaseWeapon::NotifyAttackAnimationActiveEnded_Implementation
// Access:    public 
// Returns:   void
// Qualifier:
//************************************
void ABaseWeapon::NotifyAttackAnimationActiveEnded_Implementation()
{
	// Do nothing, this exists purely to avoid having to cast up to a melee weapon
}

//************************************
// Method:    NotifyAttackAnimationEnded
// FullName:  ABaseWeapon::NotifyAttackAnimationEnded
// Access:    public 
// Returns:   void
// Qualifier:
//************************************
void ABaseWeapon::NotifyAttackAnimationEnded_Implementation()
{
	IsWaitingForAttackAnimEnd = false;
	OnAttackFinished();
	DetermineWeaponState();
}

//************************************
// Method:    StartReload
// FullName:  ABaseWeapon::StartReload
// Access:    virtual public 
// Returns:   void
// Qualifier:
// Parameter: bool bFromReplication
//************************************
void ABaseWeapon::StartReload(bool bFromReplication)
{
	if (!bFromReplication && Role < ROLE_Authority)
	{
		ServerStartReload();
	}

	if (bFromReplication || CanReload())
	{
		PendingReload = true;
		DetermineWeaponState();

		if (!OverrideAnimCompleteNotify)
		{
			PlayWeaponAnimation(ReloadAnim);
		}
		else
		{
			NotifyReloadAnimationEnded();
		}

		DetermineWeaponState();
	}
}

//************************************
// Method:    NotifyReloadAnimationEnded_Implementation
// FullName:  ABaseWeapon::NotifyReloadAnimationEnded_Implementation
// Access:    public 
// Returns:   void
// Qualifier:
//************************************
void ABaseWeapon::NotifyReloadAnimationEnded()
{
	if (PendingReload)
	{
		StopReload();
		if (Role == ROLE_Authority)
		{
			ReloadWeapon();
		}
	}
}

//************************************
// Method:    StopReload
// FullName:  ABaseWeapon::StopReload
// Access:    virtual public 
// Returns:   void
// Qualifier:
//************************************
void ABaseWeapon::StopReload()
{
	if (mCurrentState == EWeaponState::Reloading)
	{
		PendingReload = false;
		DetermineWeaponState();
		StopWeaponAnimation(ReloadAnim);
	}
}

//************************************
// Method:    ServerStartFire_Validate
// FullName:  ABaseWeapon::ServerStartFire_Validate
// Access:    public 
// Returns:   bool
// Qualifier:
//************************************
bool ABaseWeapon::ServerStartAttacking_Validate()
{
	return true;
}

//************************************
// Method:    ServerStartFire_Implementation
// FullName:  ABaseWeapon::ServerStartFire_Implementation
// Access:    public 
// Returns:   void
// Qualifier:
//************************************
void ABaseWeapon::ServerStartAttacking_Implementation()
{
	StartAttacking();
}

//************************************
// Method:    ServerStopFire_Validate
// FullName:  ABaseWeapon::ServerStopFire_Validate
// Access:    public 
// Returns:   bool
// Qualifier:
//************************************
bool ABaseWeapon::ServerStopAttacking_Validate(bool forced /*= false*/)
{
	return true;
}

//************************************
// Method:    ServerStopFire_Implementation
// FullName:  ABaseWeapon::ServerStopFire_Implementation
// Access:    public 
// Returns:   void
// Qualifier:
//************************************
void ABaseWeapon::ServerStopAttacking_Implementation(bool forced /*= false*/)
{
	StopAttacking(forced);
}

//************************************
// Method:    ServerStartReload_Validate
// FullName:  ABaseWeapon::ServerStartReload_Validate
// Access:    public 
// Returns:   bool
// Qualifier:
//************************************
bool ABaseWeapon::ServerStartReload_Validate()
{
	return true;
}

//************************************
// Method:    ServerStartReload_Implementation
// FullName:  ABaseWeapon::ServerStartReload_Implementation
// Access:    public 
// Returns:   void
// Qualifier:
//************************************
void ABaseWeapon::ServerStartReload_Implementation()
{
	StartReload();
}

//************************************
// Method:    ServerStopReload_Validate
// FullName:  ABaseWeapon::ServerStopReload_Validate
// Access:    public 
// Returns:   bool
// Qualifier:
//************************************
bool ABaseWeapon::ServerStopReload_Validate()
{
	return true;
}

//************************************
// Method:    ServerStopReload_Implementation
// FullName:  ABaseWeapon::ServerStopReload_Implementation
// Access:    public 
// Returns:   void
// Qualifier:
//************************************
void ABaseWeapon::ServerStopReload_Implementation()
{
	StopReload();
}

//************************************
// Method:    ClientStartReload_Implementation
// FullName:  ABaseWeapon::ClientStartReload_Implementation
// Access:    public 
// Returns:   void
// Qualifier:
//************************************
void ABaseWeapon::ClientStartReload_Implementation()
{
	StartReload();
}

//////////////////////////////////////////////////////////////////////////
// Control

//************************************
// Method:    CanFire
// FullName:  ABaseWeapon::CanFire
// Access:    public 
// Returns:   bool
// Qualifier: const
//************************************
bool ABaseWeapon::CanFire() const
{
	bool bCanFire = MyPawn && MyPawn->CanFire();
	bool bStateOKToFire = ((mCurrentState == EWeaponState::CombatIdle) || (mCurrentState == EWeaponState::PassiveIdle) || (mCurrentState == EWeaponState::CombatIdleWithIntentToFire));
	return ((bCanFire == true) && (bStateOKToFire == true) && (PendingReload == false));
}

//************************************
// Method:    CanReload
// FullName:  ABaseWeapon::CanReload
// Access:    public 
// Returns:   bool
// Qualifier: const
//************************************
bool ABaseWeapon::CanReload() const
{
	bool bCanReload = (!MyPawn || MyPawn->CanReload());
	bool bGotAmmo = (CurrentAmmoInClip < WeaponConfig.AmmoPerClip) && (CurrentAmmo - CurrentAmmoInClip > 0 || HasInfiniteClip());
	bool bStateOKToReload = ((mCurrentState == EWeaponState::CombatIdle) || (mCurrentState == EWeaponState::PassiveIdle) || (mCurrentState == EWeaponState::CombatIdleWithIntentToFire));

	return ((bCanReload == true) && (bGotAmmo == true) && (bStateOKToReload == true));
}


//////////////////////////////////////////////////////////////////////////
// Weapon usage

//************************************
// Method:    GiveAmmo
// FullName:  ABaseWeapon::GiveAmmo
// Access:    public 
// Returns:   void
// Qualifier:
// Parameter: int AddAmount
//************************************
void ABaseWeapon::GiveAmmo(int AddAmount)
{
	const int32 MissingAmmo = FMath::Max(0, GetMaxAmmo() - CurrentAmmo);
	AddAmount = FMath::Min(AddAmount, MissingAmmo);
	CurrentAmmo += AddAmount;

	// start reload if clip was empty
	if (GetCurrentAmmoInClip() <= 0 && CanReload() && MyPawn->GetCurrentWeapon() == this)
	{
		ClientStartReload();
	}
}

//************************************
// Method:    UseAmmo
// FullName:  ABaseWeapon::UseAmmo
// Access:    public 
// Returns:   void
// Qualifier:
//************************************
void ABaseWeapon::UseAmmo()
{
	if (!HasInfiniteAmmo())
	{
		CurrentAmmoInClip--;
	}

	if (!HasInfiniteAmmo() && !HasInfiniteClip())
	{
		CurrentAmmo--;
	}
}

//************************************
// Method:    HandleFiring
// FullName:  ABaseWeapon::HandleAttacking
// Access:    protected 
// Returns:   void
// Qualifier:
//************************************
void ABaseWeapon::HandleAttacking()
{
	if (!IsWaitingForAttackAnimEnd) // Don't fire if there's an animation playing with the flag HoldForAnimationEnd set to true
	{
		if (!mAttackSpent && (CurrentAmmoInClip > 0 || HasInfiniteClip() || HasInfiniteAmmo()) && CanFire() && (WeaponConfig.FiringMode == EFiringMode::Automatic || (WeaponConfig.FiringMode == EFiringMode::SemiAutomatic && BurstCounter == 0) || (WeaponConfig.FiringMode == EFiringMode::Burst && BurstCounter < WeaponConfig.BurstNumberOfShots)))
		{
			SimulateWeaponAttack();

			if (MyPawn && MyPawn->IsLocallyControlled())
			{
				// Play sounds and use ammo if the function returned true, ignore if not
				// Allows for charging weapons etc.
				if (AttackWithWeapon())
				{
					PlayWeaponAnimation(AttackAnim);
					UseAmmo();

					// update firing FX on remote clients if function was called on server
					BurstCounter++;
				}
			}
		}
		else if (MyPawn && MyPawn->IsLocallyControlled())
		{
			if (GetCurrentAmmo() == 0 && !Refiring)
			{
				// Play sound cue for out of ammo
				if(OutOfAmmoSound)
				{
					UAkGameplayStatics::PostEvent(OutOfAmmoSound, GetPawnOwner());
				}
			}

			// stop weapon fire FX, but stay in Firing state
			if (BurstCounter > 0)
			{
				OnBurstFinished();

				if (WeaponConfig.FiringMode == EFiringMode::SemiAutomatic || WeaponConfig.FiringMode == EFiringMode::Burst)
				{
					mAttackSpent = true;
				}
			}
		}
	}	

	if (MyPawn && MyPawn->IsLocallyControlled())
	{
		// local client will notify server
		if (Role < ROLE_Authority)
		{
			ServerHandleAttacking();
		}

		float desiredTimeBetweenShots = (WeaponConfig.FiringMode == EFiringMode::Burst) ? WeaponConfig.BurstTimeBetweenShots : WeaponConfig.TimeBetweenShots;

		// setup refire timer
		Refiring = (mCurrentState == EWeaponState::CombatIdleWithIntentToFire && desiredTimeBetweenShots > 0.0f);
		if (Refiring)
		{
			GetWorldTimerManager().SetTimer(mTimerHandle_HandleFiring, this, &ABaseWeapon::HandleAttacking, desiredTimeBetweenShots, false);
		}
	}

	if(!mAttackSpent)
		mLastFireTime = GetWorld()->GetTimeSeconds();
}

//************************************
// Method:    ServerHandleFiring_Validate
// FullName:  ABaseWeapon::ServerHandleFiring_Validate
// Access:    public 
// Returns:   bool
// Qualifier:
//************************************
bool ABaseWeapon::ServerHandleAttacking_Validate()
{
	return true;
}

//************************************
// Method:    ServerHandleFiring_Implementation
// FullName:  ABaseWeapon::ServerHandleFiring_Implementation
// Access:    public 
// Returns:   void
// Qualifier:
//************************************
void ABaseWeapon::ServerHandleAttacking_Implementation()
{
	// #MI_TODO: THIS NEEDS TO BE EDITED TO ACCOUNT FOR WEAPONS THAT SELECTIVELY USE AMMO!
	const bool bShouldUpdateAmmo = (CurrentAmmoInClip > 0 && CanFire());

	HandleAttacking();

	if (bShouldUpdateAmmo)
	{
		// update ammo
		UseAmmo();

		// update firing FX on remote clients
		BurstCounter++;

		if (WeaponConfig.FiringMode == EFiringMode::SemiAutomatic || (WeaponConfig.FiringMode == EFiringMode::Burst && BurstCounter >= WeaponConfig.BurstNumberOfShots))
		{
			StopAttacking();
		}
	}
}

//************************************
// Method:    ReloadWeapon
// FullName:  ABaseWeapon::ReloadWeapon
// Access:    virtual public 
// Returns:   void
// Qualifier:
//************************************
void ABaseWeapon::ReloadWeapon()
{
	int32 ClipDelta = FMath::Min(WeaponConfig.AmmoPerClip - CurrentAmmoInClip, CurrentAmmo - CurrentAmmoInClip);

	if (HasInfiniteClip())
	{
		ClipDelta = WeaponConfig.AmmoPerClip - CurrentAmmoInClip;
	}

	if (ClipDelta > 0)
	{
		CurrentAmmoInClip += ClipDelta;
	}

	if (HasInfiniteClip())
	{
		CurrentAmmo = FMath::Max(CurrentAmmoInClip, CurrentAmmo);
	}
}

void ABaseWeapon::Reset()
{
	IsWaitingForAttackAnimEnd = false;
	WantsToFire = false;
	PendingEquip = false;
	PendingReload = false;
	StopAttacking(true);

	DetermineWeaponState();
}

//************************************
// Method:    GetPlayerAnimForState
// FullName:  ABaseWeapon::GetPlayerAnimForState
// Access:    protected 
// Returns:   UAnimSequence*
// Qualifier:
// Parameter: EWeaponState state
//************************************
UAnimSequence* ABaseWeapon::GetPlayerAnimForState(EWeaponState state)
{
	switch (state)
	{
	case EWeaponState::Equipping:
		return EquipAnim.PlayerAnim;
	case EWeaponState::Reloading:
		return ReloadAnim.PlayerAnim;
	case EWeaponState::PassiveIdle:
		return PassiveIdleAnim.PlayerAnim;
	case EWeaponState::CombatIdle:
	case EWeaponState::CombatIdleWithIntentToFire:
		return CombatIdleAnim.PlayerAnim;
	}

	return nullptr;
}

//************************************
// Method:    WeaponAttackFire
// FullName:  ABaseWeapon::WeaponAttackFire
// Access:    virtual public 
// Returns:   void
// Qualifier:
// Parameter: float ratio
//************************************
void ABaseWeapon::WeaponAttackFire(float ratio /*= 0.f*/)
{
	if (IsWaitingForAttackAnimEnd)
		return;

	IsWaitingForAttackAnimEnd = AttackAnim.HoldForAnimationEnd;
	Delegate_OnWeaponAttackFire.Broadcast(AttackAnim, ratio);
}

//************************************
// Method:    GetWeaponType
// FullName:  ABaseWeapon::GetWeaponType
// Access:    protected 
// Returns:   UClass*
// Qualifier: const
//************************************
UClass* ABaseWeapon::GetWeaponType_Implementation() const
{
	return ABaseWeapon::StaticClass();
}

//************************************
// Method:    SetWeaponState
// FullName:  ABaseWeapon::SetWeaponState
// Access:    protected 
// Returns:   void
// Qualifier:
// Parameter: EWeaponState NewState
//************************************
void ABaseWeapon::SetWeaponState(EWeaponState NewState, bool broadcast /*= true*/)
{
	if (IsWaitingForAttackAnimEnd)
		return;

	const EWeaponState PrevState = mCurrentState;

	if (PrevState == EWeaponState::CombatIdleWithIntentToFire && NewState != EWeaponState::CombatIdleWithIntentToFire)
	{
		OnBurstFinished();
	}
	
	mCurrentState = NewState;

	if (PrevState != NewState && broadcast)
	{
		Delegate_OnWeaponStateChanged.Broadcast(this, NewState, GetPlayerAnimForState(NewState));
	}

	if (PrevState != EWeaponState::CombatIdleWithIntentToFire && NewState == EWeaponState::CombatIdleWithIntentToFire)
	{
		OnBurstStarted();
	}
}

//************************************
// Method:    DetermineWeaponState
// FullName:  ABaseWeapon::DetermineWeaponState
// Access:    protected 
// Returns:   void
// Qualifier:
//************************************
void ABaseWeapon::DetermineWeaponState(bool broadcast /*= true*/)
{
	EWeaponState NewState = (mCombatIdle) ? EWeaponState::CombatIdle : EWeaponState::PassiveIdle;

	if (IsEquipped)
	{
		if (PendingReload)
		{
			if (CanReload() == false)
			{
				NewState = mCurrentState;
			}
			else
			{
				NewState = EWeaponState::Reloading;
			}
		}
		else if ((PendingReload == false) && (WantsToFire == true) && (CanFire() == true))
		{
			NewState = EWeaponState::CombatIdleWithIntentToFire;
		}
	}
	else if (PendingEquip)
	{
		NewState = EWeaponState::Equipping;
	}
	
	SetWeaponState(NewState);
}

//************************************
// Method:    OnBurstStarted
// FullName:  ABaseWeapon::OnBurstStarted
// Access:    virtual protected 
// Returns:   void
// Qualifier:
//************************************
void ABaseWeapon::OnBurstStarted()
{
	// start firing, can be delayed to satisfy TimeBetweenShots
	if (!WeaponNotAutomatic() || !mAttackSpent)
	{
		const float GameTime = GetWorld()->GetTimeSeconds();
		if (mLastFireTime > 0 && WeaponConfig.TimeBetweenShots > 0.0f &&
			mLastFireTime + WeaponConfig.TimeBetweenShots > GameTime)
		{
			GetWorldTimerManager().SetTimer(mTimerHandle_HandleFiring, this, &ABaseWeapon::HandleAttacking, mLastFireTime + WeaponConfig.TimeBetweenShots - GameTime, false);
		}
		else
		{
			HandleAttacking();
		}
	}
}

//************************************
// Method:    OnBurstFinished
// FullName:  ABaseWeapon::OnBurstFinished
// Access:    virtual protected 
// Returns:   void
// Qualifier:
//************************************
void ABaseWeapon::OnBurstFinished()
{
	// stop firing FX on remote clients
	BurstCounter = 0;
	StopSimulatingWeaponAttack();
	
	GetWorldTimerManager().ClearTimer(mTimerHandle_HandleFiring);
	Refiring = false;

	if(!IsWaitingForAttackAnimEnd)
		OnAttackFinished();
}


//////////////////////////////////////////////////////////////////////////
// Weapon usage helpers

//************************************
// Method:    PlayWeaponAnimation
// FullName:  ABaseWeapon::PlayWeaponAnimation
// Access:    protected 
// Returns:   float
// Qualifier:
// Parameter: const FWeaponAnim & Animation
//************************************
void ABaseWeapon::PlayWeaponAnimation(const FWeaponAnim& Animation)
{
	if (Animation.WeaponAnim)
	{
		WeaponMesh->PlayAnimation(Animation.WeaponAnim, false);
	}
}

//************************************
// Method:    StopWeaponAnimation
// FullName:  ABaseWeapon::StopWeaponAnimation
// Access:    protected 
// Returns:   void
// Qualifier:
// Parameter: const FWeaponAnim & Animation
//************************************
void ABaseWeapon::StopWeaponAnimation(const FWeaponAnim& Animation)
{
	if (Animation.WeaponAnim)
	{
		WeaponMesh->Stop();
	}
}

//************************************
// Method:    WeaponTrace
// FullName:  ABaseWeapon::WeaponTrace
// Access:    protected 
// Returns:   FHitResult
// Qualifier: const
// Parameter: const FVector & StartTrace
// Parameter: const FVector & EndTrace
//************************************
FHitResult ABaseWeapon::WeaponTrace(const FVector& StartTrace, const FVector& EndTrace, bool useSphere /*= false*/, float sphereRadius /*= 0.f*/) const
{
	static FName WeaponFireTag = FName(TEXT("WeaponTrace"));

	// Perform trace to retrieve hit info
	FCollisionQueryParams TraceParams(WeaponFireTag, true, Instigator);
	TraceParams.bTraceAsyncScene = true;
	TraceParams.bReturnPhysicalMaterial = true;

	FHitResult Hit(ForceInit);
	if (useSphere)
	{
		GetWorld()->SweepSingleByChannel(Hit, StartTrace, EndTrace, FQuat::Identity, ECC_WeaponTrace, FCollisionShape::MakeSphere(sphereRadius), TraceParams);
	}
	else
	{
		GetWorld()->LineTraceSingleByChannel(Hit, StartTrace, EndTrace, ECC_WeaponTrace, TraceParams);
	}
	
	return Hit;
}

//************************************
// Method:    WeaponTraceWithPenetration
// FullName:  ABaseWeapon::WeaponTraceWithPenetration
// Access:    virtual protected 
// Returns:   TArray<FHitResult>
// Qualifier: const
// Parameter: const FVector & StartTrace
// Parameter: const FVector & EndTrace
// Parameter: bool traceMultiple
// Parameter: bool useSphere
// Parameter: float sphereRadius
//************************************
TArray<FHitResult> ABaseWeapon::WeaponTraceWithPenetration(const FVector& StartTrace, const FVector& EndTrace, bool useSphere /*= false*/, float sphereRadius /*= 0.f*/, bool ignoreBlocking /*= false*/) const
{
	static FName WeaponFireTag = FName(TEXT("WeaponTrace"));

	// Perform trace to retrieve hit info
	FCollisionQueryParams TraceParams(WeaponFireTag, true, Instigator);
	TraceParams.bTraceAsyncScene = true;
	TraceParams.bReturnPhysicalMaterial = true;
	TraceParams.bIgnoreBlocks = ignoreBlocking;

	TArray<FHitResult> Hits;
	if (useSphere)
	{
		GetWorld()->SweepMultiByChannel(Hits, StartTrace, EndTrace, FQuat::Identity, ECC_WeaponTracePenetrate, FCollisionShape::MakeSphere(sphereRadius), TraceParams);
	}
	else
	{
		GetWorld()->LineTraceMultiByChannel(Hits, StartTrace, EndTrace, ECC_WeaponTracePenetrate, TraceParams);
	}
	
	return Hits;
}

//************************************
// Method:    SetOwningPawn
// FullName:  ABaseWeapon::SetOwningPawn
// Access:    public 
// Returns:   void
// Qualifier:
// Parameter: AVertCharacter * NewOwner
//************************************
void ABaseWeapon::SetOwningPawn(AVertCharacter* NewOwner)
{
	if (MyPawn != NewOwner)
	{
		Instigator = NewOwner;
		MyPawn = NewOwner;
		// net owner for RPC calls
		SetOwner(NewOwner);
	}
}

//////////////////////////////////////////////////////////////////////////
// Replication & effects

//************************************
// Method:    OnRep_MyPawn
// FullName:  ABaseWeapon::OnRep_MyPawn
// Access:    protected 
// Returns:   void
// Qualifier:
//************************************
void ABaseWeapon::OnRep_MyPawn()
{
	if (MyPawn)
	{
		Pickup(MyPawn);
	}
	else
	{
		OnDrop();
	}
}

//************************************
// Method:    OnRep_BurstCounter
// FullName:  ABaseWeapon::OnRep_BurstCounter
// Access:    protected 
// Returns:   void
// Qualifier:
//************************************
void ABaseWeapon::OnRep_BurstCounter()
{
	if (BurstCounter > 0)
	{
		SimulateWeaponAttack();
	}
	else
	{
		StopSimulatingWeaponAttack();
	}
}

//************************************
// Method:    OnRep_Reload
// FullName:  ABaseWeapon::OnRep_Reload
// Access:    protected 
// Returns:   void
// Qualifier:
//************************************
void ABaseWeapon::OnRep_Reload()
{
	if (PendingReload)
	{
		StartReload(true);
	}
	else
	{
		StopReload();
	}
}

//************************************
// Method:    SimulateWeaponFire
// FullName:  ABaseWeapon::SimulateWeaponFire
// Access:    virtual protected 
// Returns:   void
// Qualifier:
//************************************
void ABaseWeapon::SimulateWeaponAttack()
{
	if (GetNetMode() != NM_DedicatedServer)
	{
		if (Role == ROLE_Authority && mCurrentState != EWeaponState::CombatIdleWithIntentToFire)
		{
			return;
		}

		ClientSimulateWeaponAttack();
	}
}

//************************************
// Method:    StopSimulatingWeaponFire
// FullName:  ABaseWeapon::StopSimulatingWeaponFire
// Access:    virtual protected 
// Returns:   void
// Qualifier:
//************************************
void ABaseWeapon::StopSimulatingWeaponAttack()
{
	// stop firing FX locally, unless it's a dedicated server
	if (GetNetMode() != NM_DedicatedServer)
	{
		ClientStopSimulateWeaponAttack();
	}
}

//************************************
// Method:    ClientSimulateWeaponFire_Implementation
// FullName:  ABaseWeapon::ClientSimulateWeaponFire_Implementation
// Access:    public 
// Returns:   void
// Qualifier:
//************************************
void ABaseWeapon::ClientSimulateWeaponAttack_Implementation()
{
	AVertPlayerController* PC = (MyPawn != NULL) ? Cast<AVertPlayerController>(MyPawn->Controller) : NULL;
	if (PC != NULL && PC->IsLocalController())
	{
		if (FireCameraShake != NULL)
		{
			PC->ClientPlayCameraShake(FireCameraShake, 1);
		}
		if (FireForceFeedback != NULL)
		{
			PC->ClientPlayForceFeedback(FireForceFeedback, false, "Weapon");
		}
	}
}

//************************************
// Method:    ClientStopSimulateWeaponFire_Implementation
// FullName:  ABaseWeapon::ClientStopSimulateWeaponFire_Implementation
// Access:    public 
// Returns:   void
// Qualifier:
//************************************
void ABaseWeapon::ClientStopSimulateWeaponAttack_Implementation()
{

}

//************************************
// Method:    GetLifetimeReplicatedProps
// FullName:  ABaseWeapon::GetLifetimeReplicatedProps
// Access:    public 
// Returns:   void
// Qualifier: const
// Parameter: TArray< FLifetimeProperty > & OutLifetimeProps
//************************************
void ABaseWeapon::GetLifetimeReplicatedProps(TArray< FLifetimeProperty > & OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ABaseWeapon, MyPawn);

	DOREPLIFETIME_CONDITION(ABaseWeapon, CurrentAmmo, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(ABaseWeapon, CurrentAmmoInClip, COND_OwnerOnly);

	DOREPLIFETIME_CONDITION(ABaseWeapon, BurstCounter, COND_SkipOwner);
	DOREPLIFETIME_CONDITION(ABaseWeapon, PendingReload, COND_SkipOwner);
}

//************************************
// Method:    GetPawnOwner
// FullName:  ABaseWeapon::GetPawnOwner
// Access:    public 
// Returns:   class AVertCharacter*
// Qualifier: const
//************************************
class AVertCharacter* ABaseWeapon::GetPawnOwner() const
{
	return MyPawn;
}

//************************************
// Method:    IsWeaponEquipped
// FullName:  ABaseWeapon::IsWeaponEquipped
// Access:    public 
// Returns:   bool
// Qualifier: const
//************************************
bool ABaseWeapon::IsWeaponEquipped() const
{
	return IsEquipped;
}

//************************************
// Method:    IsAttachedToPawn
// FullName:  ABaseWeapon::IsAttachedToPawn
// Access:    public 
// Returns:   bool
// Qualifier: const
//************************************
bool ABaseWeapon::IsAttachedToPawn() const
{
	return IsEquipped || PendingEquip;
}

//************************************
// Method:    GetCurrentState
// FullName:  ABaseWeapon::GetCurrentState
// Access:    public 
// Returns:   EWeaponState
// Qualifier: const
//************************************
EWeaponState ABaseWeapon::GetCurrentState() const
{
	return mCurrentState;
}

//************************************
// Method:    GetCurrentAmmo
// FullName:  ABaseWeapon::GetCurrentAmmo
// Access:    public 
// Returns:   int32
// Qualifier: const
//************************************
int32 ABaseWeapon::GetCurrentAmmo() const
{
	return CurrentAmmo;
}

//************************************
// Method:    GetCurrentAmmoInClip
// FullName:  ABaseWeapon::GetCurrentAmmoInClip
// Access:    public 
// Returns:   int32
// Qualifier: const
//************************************
int32 ABaseWeapon::GetCurrentAmmoInClip() const
{
	return CurrentAmmoInClip;
}

//************************************
// Method:    GetAmmoPerClip
// FullName:  ABaseWeapon::GetAmmoPerClip
// Access:    public 
// Returns:   int32
// Qualifier: const
//************************************
int32 ABaseWeapon::GetAmmoPerClip() const
{
	return WeaponConfig.AmmoPerClip;
}

//************************************
// Method:    GetMaxAmmo
// FullName:  ABaseWeapon::GetMaxAmmo
// Access:    public 
// Returns:   int32
// Qualifier: const
//************************************
int32 ABaseWeapon::GetMaxAmmo() const
{
	return WeaponConfig.AmmoPerClip * WeaponConfig.InitialClips;
}

bool ABaseWeapon::GetWantsToAttack() const 
{
	return WantsToFire;
}

//************************************
// Method:    HasInfiniteAmmo
// FullName:  ABaseWeapon::HasInfiniteAmmo
// Access:    public 
// Returns:   bool
// Qualifier: const
//************************************
bool ABaseWeapon::HasInfiniteAmmo() const
{
	const AVertPlayerController* MyPC = (MyPawn != NULL) ? Cast<const AVertPlayerController>(MyPawn->Controller) : NULL;
	return WeaponConfig.InfiniteAmmo || (MyPC && MyPC->HasInfiniteWeaponUsage());
}

//************************************
// Method:    HasInfiniteClip
// FullName:  ABaseWeapon::HasInfiniteClip
// Access:    public 
// Returns:   bool
// Qualifier: const
//************************************
bool ABaseWeapon::HasInfiniteClip() const
{
	const AVertPlayerController* MyPC = (MyPawn != NULL) ? Cast<const AVertPlayerController>(MyPawn->Controller) : NULL;
	return WeaponConfig.InfiniteClip || (MyPC && MyPC->HasInfiniteClip());
}

//************************************
// Method:    WeaponNotAutomatic
// FullName:  ABaseWeapon::WeaponNotAutomatic
// Access:    protected 
// Returns:   bool
// Qualifier: const
//************************************
bool ABaseWeapon::WeaponNotAutomatic() const
{
	return WeaponConfig.FiringMode != EFiringMode::Automatic;
}

//************************************
// Method:    AddBonusDamageAndKnockback
// FullName:  ABaseWeapon::AddBonusDamageAndKnockback
// Access:    protected 
// Returns:   void
// Qualifier:
// Parameter: float bonusDamage
// Parameter: float bonusKnockback
//************************************
void ABaseWeapon::AddBonusDamageAndKnockback(int32 bonusDamage /* = 0 */, float bonusKnockback /* = 0.f */)
{
	mBonusDamage += bonusDamage;
	mBonusKnockback += bonusKnockback;
}

//************************************
// Method:    ResetBonusDamageAndKnockback
// FullName:  ABaseWeapon::ResetBonusDamageAndKnockback
// Access:    protected 
// Returns:   void
// Qualifier:
//************************************
void ABaseWeapon::ResetBonusDamageAndKnockback()
{
	mBonusDamage = 0;
	mBonusKnockback = 0;
}