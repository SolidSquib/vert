// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "BaseWeapon.h"
#include "Particles/ParticleSystemComponent.h"
#include "Engine/VertPlayerState.h"
#include "UserInterface/VertHUD.h"

DEFINE_LOG_CATEGORY(LogVertBaseWeapon);

ABaseWeapon::ABaseWeapon(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	WeaponMesh = ObjectInitializer.CreateDefaultSubobject<USkeletalMeshComponent>(this, TEXT("WeaponMesh1P"));
	WeaponMesh->MeshComponentUpdateFlag = EMeshComponentUpdateFlag::OnlyTickPoseWhenRendered;
	WeaponMesh->bReceivesDecals = false;
	WeaponMesh->CastShadow = true;
	WeaponMesh->SetCollisionObjectType(ECC_WorldDynamic);
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WeaponMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
	RootComponent = WeaponMesh;

	InteractionSphere->SetupAttachment(RootComponent);

	IsEquipped = false;
	WantsToFire = false;
	PendingReload = false;
	PendingEquip = false;
	mCurrentState = EWeaponState::Idle;

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

void ABaseWeapon::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	if (WeaponConfig.InitialClips > 0)
	{
		CurrentAmmoInClip = WeaponConfig.AmmoPerClip;
		CurrentAmmo = WeaponConfig.AmmoPerClip * WeaponConfig.InitialClips;
	}

	DetachMeshFromPawn();
}

void ABaseWeapon::Destroyed()
{
	Super::Destroyed();

	StopSimulatingWeaponFire();
}

//************************************
// Method:    OnEquip
// FullName:  ABaseWeapon::OnEquip
// Access:    virtual public 
// Returns:   void
// Qualifier:
//************************************
void ABaseWeapon::OnEquip()
{
	AttachMeshToPawn();

	PendingEquip = true;
	DetermineWeaponState();

	AttachMeshToPawn();

	IsEquipped = true;
	PendingEquip = false;

	// Determine the state so that the can reload checks will work
	DetermineWeaponState();

	if (MyPawn && MyPawn->IsLocallyControlled())
	{
		PlayWeaponSound(EquipSound);
	}
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
		PendingEquip = false;
		IsEquipped = true;
	}
	else { UE_LOG(LogVertBaseWeapon, Warning, TEXT("%s attempting to notify animation ended when equip is not pending. Check for multiple calls."), *GetName()); }
}

//************************************
// Method:    OnUnEquip
// FullName:  ABaseWeapon::OnUnEquip
// Access:    virtual public 
// Returns:   void
// Qualifier:
//************************************
void ABaseWeapon::OnUnEquip()
{
	DetachMeshFromPawn();
	IsEquipped = false;
	StopAttacking();

	if (PendingReload)
	{
		StopWeaponAnimation(ReloadAnim);
		PendingReload = false;

		GetWorldTimerManager().ClearTimer(mTimerHandle_StopReload);
		GetWorldTimerManager().ClearTimer(mTimerHandle_ReloadWeapon);
	}

	if (PendingEquip)
	{
		StopWeaponAnimation(EquipAnim);
		PendingEquip = false;

		GetWorldTimerManager().ClearTimer(mTimerHandle_OnEquipFinished);
	}

	DetermineWeaponState();
}

//************************************
// Method:    OnPickup
// FullName:  ABaseWeapon::OnPickup
// Access:    virtual public 
// Returns:   void
// Qualifier:
// Parameter: AVertCharacter * NewOwner
//************************************
void ABaseWeapon::OnPickup(AVertCharacter* NewOwner)
{
	SetOwningPawn(NewOwner);

	OnEquip();
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
	if (Role == ROLE_Authority)
	{
		SetOwningPawn(NULL);
	}

	if (IsAttachedToPawn())
	{
		OnUnEquip();
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
}

//************************************
// Method:    FireWeapon_Implementation
// FullName:  ABaseWeapon::FireWeapon_Implementation
// Access:    public 
// Returns:   bool
// Qualifier:
//************************************
bool ABaseWeapon::FireWeapon_Implementation()
{
	UE_LOG(LogVertBaseWeapon, Fatal, TEXT("Call to pure virtual function ABaseWeapon::FireWeapon_Implementation not allowed"));
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
		FVector launchDirection = UVertUtilities::SnapVectorToAngle(character->GetAxisPostisions().GetPlayerLeftThumbstickDirection(), 45.f);

		character->GetInteractionComponent()->ThrowInteractive(WeaponMesh, launchDirection*100000.f, FVector(1.f, 0, 0) * 5000.f);
		EnableInteractionDetection();

		UE_LOG(LogVertBaseWeapon, Log, TEXT("Weapon %s thrown by player %s"), *GetName(), *character->GetName());
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
		ServerStartFire();
	}

	if (!WantsToFire)
	{
		WantsToFire = true;
		DetermineWeaponState();
	}
}

//************************************
// Method:    StopAttacking
// FullName:  ABaseWeapon::StopAttacking
// Access:    virtual public 
// Returns:   void
// Qualifier:
//************************************
void ABaseWeapon::StopAttacking()
{
	if (Role < ROLE_Authority)
	{
		ServerStopFire();
	}

	if (WantsToFire && (WeaponConfig.FiringMode != EFiringMode::Burst || BurstCounter >= WeaponConfig.BurstNumberOfShots || GetCurrentAmmoInClip() <= 0))
	{
		WantsToFire = false;
		DetermineWeaponState();
	}
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

		PlayWeaponAnimation(ReloadAnim);

		//GetWorldTimerManager().SetTimer(mTimerHandle_StopReload, this, &ABaseWeapon::StopReload, AnimDuration, false);
		//if (Role == ROLE_Authority)
		//{
		//	GetWorldTimerManager().SetTimer(mTimerHandle_ReloadWeapon, this, &ABaseWeapon::ReloadWeapon, FMath::Max(0.1f, AnimDuration - 0.1f), false);
		//}

		if (MyPawn && MyPawn->IsLocallyControlled())
		{
			PlayWeaponSound(ReloadSound);
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
bool ABaseWeapon::ServerStartFire_Validate()
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
void ABaseWeapon::ServerStartFire_Implementation()
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
bool ABaseWeapon::ServerStopFire_Validate()
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
void ABaseWeapon::ServerStopFire_Implementation()
{
	StopAttacking();
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
	bool bStateOKToFire = ((mCurrentState == EWeaponState::Idle) || (mCurrentState == EWeaponState::Firing));
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
	bool bStateOKToReload = ((mCurrentState == EWeaponState::Idle) || (mCurrentState == EWeaponState::Firing));
	
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
	const int32 MissingAmmo = FMath::Max(0, WeaponConfig.MaxAmmo - CurrentAmmo);
	AddAmount = FMath::Min(AddAmount, MissingAmmo);
	CurrentAmmo += AddAmount;

	// start reload if clip was empty
	if (GetCurrentAmmoInClip() <= 0 && CanReload() && MyPawn->GetWeapon() == this)
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
// FullName:  ABaseWeapon::HandleFiring
// Access:    protected 
// Returns:   void
// Qualifier:
//************************************
void ABaseWeapon::HandleFiring()
{
	if ((CurrentAmmoInClip > 0 || HasInfiniteClip() || HasInfiniteAmmo()) && CanFire())
	{
		if (GetNetMode() != NM_DedicatedServer)
		{
			SimulateWeaponFire();
		}

		if (MyPawn && MyPawn->IsLocallyControlled())
		{
			if (FireWeapon())
			{
				PlayWeaponAnimation(FireAnim);
				PlayWeaponSound(FireSound);

				UseAmmo();

				// update firing FX on remote clients if function was called on server
				BurstCounter++;
			}			

			if (WeaponConfig.FiringMode == EFiringMode::SemiAutomatic || (WeaponConfig.FiringMode == EFiringMode::Burst && BurstCounter >= WeaponConfig.BurstNumberOfShots))
			{
				StopAttacking();
			}
		}
	}
	else if (CanReload())
	{
		StartReload();
	}
	else if (MyPawn && MyPawn->IsLocallyControlled())
	{
		if (GetCurrentAmmo() == 0 && !Refiring)
		{
			PlayWeaponSound(OutOfAmmoSound);
		}

		// stop weapon fire FX, but stay in Firing state
		if (BurstCounter > 0)
		{
			OnBurstFinished();
		}
	}

	if (MyPawn && MyPawn->IsLocallyControlled())
	{
		// local client will notify server
		if (Role < ROLE_Authority)
		{
			ServerHandleFiring();
		}

		// reload after firing last round
		if (CurrentAmmoInClip <= 0 && CanReload())
		{
			StartReload();
		}

		// setup refire timer
		Refiring = (mCurrentState == EWeaponState::Firing && WeaponConfig.TimeBetweenShots > 0.0f);
		if (Refiring)
		{
			GetWorldTimerManager().SetTimer(mTimerHandle_HandleFiring, this, &ABaseWeapon::HandleFiring, WeaponConfig.TimeBetweenShots, false);
		}
	}

	mLastFireTime = GetWorld()->GetTimeSeconds();
}

//************************************
// Method:    ServerHandleFiring_Validate
// FullName:  ABaseWeapon::ServerHandleFiring_Validate
// Access:    public 
// Returns:   bool
// Qualifier:
//************************************
bool ABaseWeapon::ServerHandleFiring_Validate()
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
void ABaseWeapon::ServerHandleFiring_Implementation()
{
	const bool bShouldUpdateAmmo = (CurrentAmmoInClip > 0 && CanFire());

	HandleFiring();

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
	case EWeaponState::Firing:
		return FireAnim.PlayerAnim;
	case EWeaponState::Reloading:
		return ReloadAnim.PlayerAnim;
	case EWeaponState::Idle:
	default:
		return IdleAnim.PlayerAnim;
	}
}

//************************************
// Method:    SetWeaponState
// FullName:  ABaseWeapon::SetWeaponState
// Access:    protected 
// Returns:   void
// Qualifier:
// Parameter: EWeaponState NewState
//************************************
void ABaseWeapon::SetWeaponState(EWeaponState NewState)
{
	const EWeaponState PrevState = mCurrentState;

	if (PrevState == EWeaponState::Firing && NewState != EWeaponState::Firing)
	{
		OnBurstFinished();
	}
	
	mCurrentState = NewState;

	if (PrevState != NewState)
	{
		Delegate_OnWeaponStateChanged.Broadcast(this, NewState, GetPlayerAnimForState(NewState));
	}

	if (PrevState != EWeaponState::Firing && NewState == EWeaponState::Firing)
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
void ABaseWeapon::DetermineWeaponState()
{
	EWeaponState NewState = EWeaponState::Idle;

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
			NewState = EWeaponState::Firing;
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
	const float GameTime = GetWorld()->GetTimeSeconds();
	if (mLastFireTime > 0 && WeaponConfig.TimeBetweenShots > 0.0f &&
		mLastFireTime + WeaponConfig.TimeBetweenShots > GameTime)
	{
		GetWorldTimerManager().SetTimer(mTimerHandle_HandleFiring, this, &ABaseWeapon::HandleFiring, mLastFireTime + WeaponConfig.TimeBetweenShots - GameTime, false);
	}
	else
	{
		HandleFiring();
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
	
	// stop firing FX locally, unless it's a dedicated server
	if (GetNetMode() != NM_DedicatedServer)
	{
		StopSimulatingWeaponFire();
	}

	GetWorldTimerManager().ClearTimer(mTimerHandle_HandleFiring);
	Refiring = false;
}


//////////////////////////////////////////////////////////////////////////
// Weapon usage helpers

//************************************
// Method:    PlayWeaponSound
// FullName:  ABaseWeapon::PlayWeaponSound
// Access:    protected 
// Returns:   UAudioComponent*
// Qualifier:
// Parameter: USoundCue * Sound
//************************************
UAudioComponent* ABaseWeapon::PlayWeaponSound(USoundCue* Sound)
{
	UAudioComponent* AC = NULL;
	if (Sound && MyPawn)
	{
		AC = UGameplayStatics::SpawnSoundAttached(Sound, MyPawn->GetRootComponent());
	}

	return AC;
}

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
FHitResult ABaseWeapon::WeaponTrace(const FVector& StartTrace, const FVector& EndTrace) const
{
	static FName WeaponFireTag = FName(TEXT("WeaponTrace"));

	// Perform trace to retrieve hit info
	FCollisionQueryParams TraceParams(WeaponFireTag, true, Instigator);
	TraceParams.bTraceAsyncScene = true;
	TraceParams.bReturnPhysicalMaterial = true;

	FHitResult Hit(ForceInit);
	GetWorld()->LineTraceSingleByChannel(Hit, StartTrace, EndTrace, ECC_WeaponTrace, TraceParams);

	return Hit;
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
		OnPickup(MyPawn);
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
		SimulateWeaponFire();
	}
	else
	{
		StopSimulatingWeaponFire();
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
void ABaseWeapon::SimulateWeaponFire()
{
	if (Role == ROLE_Authority && mCurrentState != EWeaponState::Firing)
	{
		return;
	}

	ClientSimulateWeaponFire();
}

//************************************
// Method:    StopSimulatingWeaponFire
// FullName:  ABaseWeapon::StopSimulatingWeaponFire
// Access:    virtual protected 
// Returns:   void
// Qualifier:
//************************************
void ABaseWeapon::StopSimulatingWeaponFire()
{
	ClientStopSimulateWeaponFire();
}

//************************************
// Method:    ClientSimulateWeaponFire_Implementation
// FullName:  ABaseWeapon::ClientSimulateWeaponFire_Implementation
// Access:    public 
// Returns:   void
// Qualifier:
//************************************
void ABaseWeapon::ClientSimulateWeaponFire_Implementation()
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
void ABaseWeapon::ClientStopSimulateWeaponFire_Implementation()
{

}

void ABaseWeapon::GetLifetimeReplicatedProps(TArray< FLifetimeProperty > & OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ABaseWeapon, MyPawn);

	DOREPLIFETIME_CONDITION(ABaseWeapon, CurrentAmmo, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(ABaseWeapon, CurrentAmmoInClip, COND_OwnerOnly);

	DOREPLIFETIME_CONDITION(ABaseWeapon, BurstCounter, COND_SkipOwner);
	DOREPLIFETIME_CONDITION(ABaseWeapon, PendingReload, COND_SkipOwner);
}

class AVertCharacter* ABaseWeapon::GetPawnOwner() const
{
	return MyPawn;
}

bool ABaseWeapon::IsWeaponEquipped() const
{
	return IsEquipped;
}

bool ABaseWeapon::IsAttachedToPawn() const
{
	return IsEquipped || PendingEquip;
}

EWeaponState ABaseWeapon::GetCurrentState() const
{
	return mCurrentState;
}

int32 ABaseWeapon::GetCurrentAmmo() const
{
	return CurrentAmmo;
}

int32 ABaseWeapon::GetCurrentAmmoInClip() const
{
	return CurrentAmmoInClip;
}

int32 ABaseWeapon::GetAmmoPerClip() const
{
	return WeaponConfig.AmmoPerClip;
}

int32 ABaseWeapon::GetMaxAmmo() const
{
	return WeaponConfig.MaxAmmo;
}

bool ABaseWeapon::HasInfiniteAmmo() const
{
	const AVertPlayerController* MyPC = (MyPawn != NULL) ? Cast<const AVertPlayerController>(MyPawn->Controller) : NULL;
	return WeaponConfig.InfiniteAmmo || (MyPC && MyPC->HasInfiniteWeaponUsage());
}

bool ABaseWeapon::HasInfiniteClip() const
{
	const AVertPlayerController* MyPC = (MyPawn != NULL) ? Cast<const AVertPlayerController>(MyPawn->Controller) : NULL;
	return WeaponConfig.InfiniteClip || (MyPC && MyPC->HasInfiniteClip());
}

float ABaseWeapon::GetEquipStartedTime() const
{
	return mEquipStartedTime;
}

float ABaseWeapon::GetEquipDuration() const
{
	return mEquipDuration;
}
