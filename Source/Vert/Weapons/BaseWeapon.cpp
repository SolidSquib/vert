// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "Vert.h"
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

	bPlayingFireAnim = false;
	bIsEquipped = false;
	bWantsToFire = false;
	bPendingReload = false;
	bPendingEquip = false;
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

//////////////////////////////////////////////////////////////////////////
// Inventory

void ABaseWeapon::OnEquip()
{
	AttachMeshToPawn();

	bPendingEquip = true;
	DetermineWeaponState();

	AttachMeshToPawn();

	bIsEquipped = true;
	bPendingEquip = false;

	// Determine the state so that the can reload checks will work
	DetermineWeaponState();

	if (MyPawn)
	{
		// try to reload empty clip
		if (MyPawn->IsLocallyControlled() &&
			CurrentAmmoInClip <= 0 &&
			CanReload())
		{
			StartReload();
		}
	}
	
	if (MyPawn && MyPawn->IsLocallyControlled())
	{
		PlayWeaponSound(EquipSound);
	}
}

void ABaseWeapon::OnUnEquip()
{
	DetachMeshFromPawn();
	bIsEquipped = false;
	StopFire();

	if (bPendingReload)
	{
		StopWeaponAnimation(ReloadAnim);
		bPendingReload = false;

		GetWorldTimerManager().ClearTimer(mTimerHandle_StopReload);
		GetWorldTimerManager().ClearTimer(mTimerHandle_ReloadWeapon);
	}

	if (bPendingEquip)
	{
		StopWeaponAnimation(EquipAnim);
		bPendingEquip = false;

		GetWorldTimerManager().ClearTimer(mTimerHandle_OnEquipFinished);
	}

	DetermineWeaponState();
}

void ABaseWeapon::OnPickup(AVertCharacter* NewOwner)
{
	SetOwningPawn(NewOwner);

	OnEquip();
}

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

void ABaseWeapon::DetachMeshFromPawn()
{
	DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
}

void ABaseWeapon::Interact(const TWeakObjectPtr<class UCharacterInteractionComponent>& instigator)
{
	WeaponInteract(instigator.Get(), instigator->GetCharacterOwner());
}

bool ABaseWeapon::FireWeapon_Implementation()
{
	UE_LOG(LogVertBaseWeapon, Fatal, TEXT("Call to pure virtual function ABaseWeapon::FireWeapon_Implementation not allowed"));
	return false;
}

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

void ABaseWeapon::ThrowWeapon_Implementation()
{
	if (AVertCharacter* character = Cast<AVertCharacter>(Instigator))
	{
		character->GetInteractionComponent()->DropInteractive();

		EnableInteractionDetection();

		FVector launchDirection = UVertUtilities::SnapVectorToAngle(character->GetAxisPostisions().GetPlayerLeftThumbstickDirection(), 45.f);
		WeaponMesh->SetSimulatePhysics(true);
		WeaponMesh->AddImpulse(launchDirection * 100000.f);
		WeaponMesh->AddAngularImpulse(FVector(1.f, 0, 0) * 5000.f);

		UE_LOG(LogVertBaseWeapon, Log, TEXT("Weapon %s thrown by player %s"), *GetName(), *character->GetName());
	}
}

//////////////////////////////////////////////////////////////////////////
// Input

void ABaseWeapon::StartFire()
{
	if (Role < ROLE_Authority)
	{
		ServerStartFire();
	}

	if (!bWantsToFire)
	{
		bWantsToFire = true;
		DetermineWeaponState();
	}
}

void ABaseWeapon::StopFire()
{
	if (Role < ROLE_Authority)
	{
		ServerStopFire();
	}

	if (bWantsToFire && (WeaponConfig.FiringMode != EFiringMode::Burst || BurstCounter >= WeaponConfig.BurstNumberOfShots || GetCurrentAmmoInClip() <= 0))
	{
		bWantsToFire = false;
		DetermineWeaponState();
	}
}

void ABaseWeapon::StartReload(bool bFromReplication)
{
	if (!bFromReplication && Role < ROLE_Authority)
	{
		ServerStartReload();
	}

	if (bFromReplication || CanReload())
	{
		bPendingReload = true;
		DetermineWeaponState();

		float AnimDuration = PlayWeaponAnimation(ReloadAnim);
		if (AnimDuration <= 0.0f)
		{
			AnimDuration = WeaponConfig.NoAnimReloadDuration;
		}

		GetWorldTimerManager().SetTimer(mTimerHandle_StopReload, this, &ABaseWeapon::StopReload, AnimDuration, false);
		if (Role == ROLE_Authority)
		{
			GetWorldTimerManager().SetTimer(mTimerHandle_ReloadWeapon, this, &ABaseWeapon::ReloadWeapon, FMath::Max(0.1f, AnimDuration - 0.1f), false);
		}

		if (MyPawn && MyPawn->IsLocallyControlled())
		{
			PlayWeaponSound(ReloadSound);
		}
	}
}

void ABaseWeapon::StopReload()
{
	if (mCurrentState == EWeaponState::Reloading)
	{
		bPendingReload = false;
		DetermineWeaponState();
		StopWeaponAnimation(ReloadAnim);
	}
}

bool ABaseWeapon::ServerStartFire_Validate()
{
	return true;
}

void ABaseWeapon::ServerStartFire_Implementation()
{
	StartFire();
}

bool ABaseWeapon::ServerStopFire_Validate()
{
	return true;
}

void ABaseWeapon::ServerStopFire_Implementation()
{
	StopFire();
}

bool ABaseWeapon::ServerStartReload_Validate()
{
	return true;
}

void ABaseWeapon::ServerStartReload_Implementation()
{
	StartReload();
}

bool ABaseWeapon::ServerStopReload_Validate()
{
	return true;
}

void ABaseWeapon::ServerStopReload_Implementation()
{
	StopReload();
}

void ABaseWeapon::ClientStartReload_Implementation()
{
	StartReload();
}

//////////////////////////////////////////////////////////////////////////
// Control

bool ABaseWeapon::CanFire() const
{
	bool bCanFire = MyPawn && MyPawn->CanFire();
	bool bStateOKToFire = ((mCurrentState == EWeaponState::Idle) || (mCurrentState == EWeaponState::Firing));
	return ((bCanFire == true) && (bStateOKToFire == true) && (bPendingReload == false));
}

bool ABaseWeapon::CanReload() const
{
	bool bCanReload = (!MyPawn || MyPawn->CanReload());
	bool bGotAmmo = (CurrentAmmoInClip < WeaponConfig.AmmoPerClip) && (CurrentAmmo - CurrentAmmoInClip > 0 || HasInfiniteClip());
	bool bStateOKToReload = ((mCurrentState == EWeaponState::Idle) || (mCurrentState == EWeaponState::Firing));
	
	return ((bCanReload == true) && (bGotAmmo == true) && (bStateOKToReload == true));
}


//////////////////////////////////////////////////////////////////////////
// Weapon usage

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
				StopFire();
			}
		}
	}
	else if (CanReload())
	{
		StartReload();
	}
	else if (MyPawn && MyPawn->IsLocallyControlled())
	{
		if (GetCurrentAmmo() == 0 && !bRefiring)
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
		bRefiring = (mCurrentState == EWeaponState::Firing && WeaponConfig.TimeBetweenShots > 0.0f);
		if (bRefiring)
		{
			GetWorldTimerManager().SetTimer(mTimerHandle_HandleFiring, this, &ABaseWeapon::HandleFiring, WeaponConfig.TimeBetweenShots, false);
		}
	}

	mLastFireTime = GetWorld()->GetTimeSeconds();
}

bool ABaseWeapon::ServerHandleFiring_Validate()
{
	return true;
}

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
			StopFire();
		}
	}
}

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

UAnimMontage* ABaseWeapon::GetPlayerAnimForState(EWeaponState state)
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

void ABaseWeapon::SetWeaponState(EWeaponState NewState)
{
	const EWeaponState PrevState = mCurrentState;

	if (PrevState == EWeaponState::Firing && NewState != EWeaponState::Firing)
	{
		OnBurstFinished();
	}
	
	mCurrentState = NewState;

	if (PrevState != NewState && MyPawn)
	{
		MyPawn->OnWeaponStateChanged.Broadcast(this, NewState, GetPlayerAnimForState(NewState));
	}

	if (PrevState != EWeaponState::Firing && NewState == EWeaponState::Firing)
	{
		OnBurstStarted();
	}
}

void ABaseWeapon::DetermineWeaponState()
{
	EWeaponState NewState = EWeaponState::Idle;

	if (bIsEquipped)
	{
		if (bPendingReload)
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
		else if ((bPendingReload == false) && (bWantsToFire == true) && (CanFire() == true))
		{
			NewState = EWeaponState::Firing;
		}
	}
	else if (bPendingEquip)
	{
		NewState = EWeaponState::Equipping;
	}
	
	SetWeaponState(NewState);
}

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
	bRefiring = false;
}


//////////////////////////////////////////////////////////////////////////
// Weapon usage helpers

UAudioComponent* ABaseWeapon::PlayWeaponSound(USoundCue* Sound)
{
	UAudioComponent* AC = NULL;
	if (Sound && MyPawn)
	{
		AC = UGameplayStatics::SpawnSoundAttached(Sound, MyPawn->GetRootComponent());
	}

	return AC;
}

float ABaseWeapon::PlayWeaponAnimation(const FWeaponAnim& Animation)
{
	float Duration = 0.0f;
	if (Animation.WeaponAnim)
	{
		WeaponMesh->PlayAnimation(Animation.WeaponAnim, false);
	}

	return Duration;
}

void ABaseWeapon::StopWeaponAnimation(const FWeaponAnim& Animation)
{
	if (Animation.WeaponAnim)
	{
		WeaponMesh->Stop();
	}
}

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

void ABaseWeapon::OnRep_Reload()
{
	if (bPendingReload)
	{
		StartReload(true);
	}
	else
	{
		StopReload();
	}
}

void ABaseWeapon::SimulateWeaponFire()
{
	if (Role == ROLE_Authority && mCurrentState != EWeaponState::Firing)
	{
		return;
	}

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

void ABaseWeapon::StopSimulatingWeaponFire()
{
	
}

void ABaseWeapon::GetLifetimeReplicatedProps(TArray< FLifetimeProperty > & OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ABaseWeapon, MyPawn);

	DOREPLIFETIME_CONDITION(ABaseWeapon, CurrentAmmo, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(ABaseWeapon, CurrentAmmoInClip, COND_OwnerOnly);

	DOREPLIFETIME_CONDITION(ABaseWeapon, BurstCounter, COND_SkipOwner);
	DOREPLIFETIME_CONDITION(ABaseWeapon, bPendingReload, COND_SkipOwner);
}

class AVertCharacter* ABaseWeapon::GetPawnOwner() const
{
	return MyPawn;
}

bool ABaseWeapon::IsEquipped() const
{
	return bIsEquipped;
}

bool ABaseWeapon::IsAttachedToPawn() const
{
	return bIsEquipped || bPendingEquip;
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
	return WeaponConfig.bInfiniteAmmo || (MyPC && MyPC->HasInfiniteWeaponUsage());
}

bool ABaseWeapon::HasInfiniteClip() const
{
	const AVertPlayerController* MyPC = (MyPawn != NULL) ? Cast<const AVertPlayerController>(MyPawn->Controller) : NULL;
	return WeaponConfig.bInfiniteClip || (MyPC && MyPC->HasInfiniteClip());
}

float ABaseWeapon::GetEquipStartedTime() const
{
	return mEquipStartedTime;
}

float ABaseWeapon::GetEquipDuration() const
{
	return mEquipDuration;
}
