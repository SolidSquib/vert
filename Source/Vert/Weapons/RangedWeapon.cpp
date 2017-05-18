// Copyright Inside Out Games Ltd. 2017

#include "Vert.h"
#include "RangedWeapon.h"

DEFINE_LOG_CATEGORY(LogRangedWeapon);

ARangedWeapon::ARangedWeapon(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	LoopedMuzzleFX = false;
	mLoopedFireAnim = false;
	mPlayingFireAnim = false;
	mIsEquipped = false;
	mWantsToFire = false;
	PendingReload = false;
	PendingEquip = false;
	CurrentState = EWeaponState::Idle;

	CurrentAmmo = 0;
	CurrentAmmoInClip = 0;
	BurstCounter = 0;
	LastFireTime = 0.0f;
	mLoopedFireAnim = FiringMode != EWeaponFiremode::SemiAutomatic;

	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PrePhysics;
	SetRemoteRoleForBackwardsCompat(ROLE_SimulatedProxy);
	bReplicates = true;
	bNetUseOwnerRelevancy = true;
}

void ARangedWeapon::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	if (WeaponConfig.InitialClips > 0)
	{
		CurrentAmmoInClip = WeaponConfig.AmmoPerClip;
		CurrentAmmo = WeaponConfig.AmmoPerClip * WeaponConfig.InitialClips;
	}
}

void ARangedWeapon::Destroyed()
{
	Super::Destroyed();

	StopSimulatingWeaponFire();
}

void ARangedWeapon::OnEnterInventory(AVertCharacter* NewOwner)
{
	SetOwningPawn(NewOwner);
}

void ARangedWeapon::OnLeaveInventory()
{
	if (Role == ROLE_Authority)
	{
		SetOwningPawn(NULL);
	}
}

//////////////////////////////////////////////////////////////////////////
// Input

void ARangedWeapon::StartFire()
{
	if (Role < ROLE_Authority)
	{
		ServerStartFire();
	}

	if (!mWantsToFire)
	{
		mWantsToFire = true;
		DetermineWeaponState();
	}
}

void ARangedWeapon::StopFire()
{
	if (Role < ROLE_Authority)
	{
		ServerStopFire();
	}

	if (mWantsToFire)
	{
		mWantsToFire = false;
		DetermineWeaponState();
	}
}

void ARangedWeapon::StartReload(bool bFromReplication)
{
	if (!bFromReplication && Role < ROLE_Authority)
	{
		ServerStartReload();
	}

	if (bFromReplication || CanReload())
	{
		PendingReload = true;
		DetermineWeaponState();

		float AnimDuration = PlayWeaponAnimation(ReloadAnim);
		if (AnimDuration <= 0.0f)
		{
			AnimDuration = WeaponConfig.NoAnimReloadDuration;
		}

		GetWorldTimerManager().SetTimer(TimerHandle_StopReload, this, &ARangedWeapon::StopReload, AnimDuration, false);
		if (Role == ROLE_Authority)
		{
			GetWorldTimerManager().SetTimer(TimerHandle_ReloadWeapon, this, &ARangedWeapon::ReloadWeapon, FMath::Max(0.1f, AnimDuration - 0.1f), false);
		}

		if (mCharacterInteractionOwner->GetCharacterOwner() && mCharacterInteractionOwner->GetCharacterOwner()->IsLocallyControlled())
		{
			PlayWeaponSound(ReloadSound);
		}
	}
}

void ARangedWeapon::StopReload()
{
	if (CurrentState == EWeaponState::Reloading)
	{
		PendingReload = false;
		DetermineWeaponState();
		Sprite->SetFlipbook(DefaultAnimation);
	}
}

bool ARangedWeapon::ServerStartFire_Validate()
{
	return true;
}

void ARangedWeapon::ServerStartFire_Implementation()
{
	StartFire();
}

bool ARangedWeapon::ServerStopFire_Validate()
{
	return true;
}

void ARangedWeapon::ServerStopFire_Implementation()
{
	StopFire();
}

bool ARangedWeapon::ServerStartReload_Validate()
{
	return true;
}

void ARangedWeapon::ServerStartReload_Implementation()
{
	StartReload();
}

bool ARangedWeapon::ServerStopReload_Validate()
{
	return true;
}

void ARangedWeapon::ServerStopReload_Implementation()
{
	StopReload();
}

void ARangedWeapon::ClientStartReload_Implementation()
{
	StartReload();
}

//////////////////////////////////////////////////////////////////////////
// Control

bool ARangedWeapon::CanFire() const
{
	bool bStateOKToFire = ((CurrentState == EWeaponState::Idle) || (CurrentState == EWeaponState::Firing));
	return ((bStateOKToFire == true) && (PendingReload == false));
}

bool ARangedWeapon::CanReload() const
{
	bool bGotAmmo = (CurrentAmmoInClip < WeaponConfig.AmmoPerClip) && (CurrentAmmo - CurrentAmmoInClip > 0 || HasInfiniteClip());
	bool bStateOKToReload = ((CurrentState == EWeaponState::Idle) || (CurrentState == EWeaponState::Firing));
	return ((bGotAmmo == true) && (bStateOKToReload == true));
}


//////////////////////////////////////////////////////////////////////////
// Weapon usage

void ARangedWeapon::GiveAmmo(int AddAmount)
{
	const int32 MissingAmmo = FMath::Max(0, WeaponConfig.MaxAmmo - CurrentAmmo);
	AddAmount = FMath::Min(AddAmount, MissingAmmo);
	CurrentAmmo += AddAmount;
}

void ARangedWeapon::UseAmmo()
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

void ARangedWeapon::HandleFiring()
{
	if ((CurrentAmmoInClip > 0 || HasInfiniteClip() || HasInfiniteAmmo()) && CanFire())
	{
		if (GetNetMode() != NM_DedicatedServer)
		{
			SimulateWeaponFire();
		}

		if (mCharacterInteractionOwner.IsValid() && mCharacterInteractionOwner->GetCharacterOwner()->IsLocallyControlled())
		{
			FireWeapon();

			UseAmmo();

			// update firing FX on remote clients if function was called on server
			BurstCounter++;
		}
	}
	else if (CanReload())
	{
		StartReload();
	}
	else if (mCharacterInteractionOwner.IsValid() && mCharacterInteractionOwner->GetCharacterOwner()->IsLocallyControlled())
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

	if (mCharacterInteractionOwner.IsValid() && mCharacterInteractionOwner->GetCharacterOwner()->IsLocallyControlled())
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
		bRefiring = (CurrentState == EWeaponState::Firing && WeaponConfig.TimeBetweenShots > 0.0f);
		if (bRefiring)
		{
			GetWorldTimerManager().SetTimer(TimerHandle_HandleFiring, this, &ARangedWeapon::HandleFiring, WeaponConfig.TimeBetweenShots, false);
		}
	}

	LastFireTime = GetWorld()->GetTimeSeconds();
}

bool ARangedWeapon::ServerHandleFiring_Validate()
{
	return true;
}

void ARangedWeapon::ServerHandleFiring_Implementation()
{
	const bool bShouldUpdateAmmo = (CurrentAmmoInClip > 0 && CanFire());

	HandleFiring();

	if (bShouldUpdateAmmo)
	{
		// update ammo
		UseAmmo();

		// update firing FX on remote clients
		BurstCounter++;
	}
}

void ARangedWeapon::ReloadWeapon()
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

void ARangedWeapon::SetWeaponState(EWeaponState::Type NewState)
{
	const EWeaponState::Type PrevState = CurrentState;

	if (PrevState == EWeaponState::Firing && NewState != EWeaponState::Firing)
	{
		OnBurstFinished();
	}

	CurrentState = NewState;

	if (PrevState != EWeaponState::Firing && NewState == EWeaponState::Firing)
	{
		OnBurstStarted();
	}
}

void ARangedWeapon::DetermineWeaponState()
{
	EWeaponState::Type NewState = EWeaponState::Idle;

	if (mIsEquipped)
	{
		if (PendingReload)
		{
			if (CanReload() == false)
			{
				NewState = CurrentState;
			}
			else
			{
				NewState = EWeaponState::Reloading;
			}
		}
		else if ((PendingReload == false) && (mWantsToFire == true) && (CanFire() == true))
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

void ARangedWeapon::OnBurstStarted()
{
	// start firing, can be delayed to satisfy TimeBetweenShots
	const float GameTime = GetWorld()->GetTimeSeconds();
	if (LastFireTime > 0 && WeaponConfig.TimeBetweenShots > 0.0f &&
		LastFireTime + WeaponConfig.TimeBetweenShots > GameTime)
	{
		GetWorldTimerManager().SetTimer(TimerHandle_HandleFiring, this, &ARangedWeapon::HandleFiring, LastFireTime + WeaponConfig.TimeBetweenShots - GameTime, false);
	}
	else
	{
		HandleFiring();
	}
}

void ARangedWeapon::OnBurstFinished()
{
	// stop firing FX on remote clients
	BurstCounter = 0;

	// stop firing FX locally, unless it's a dedicated server
	if (GetNetMode() != NM_DedicatedServer)
	{
		StopSimulatingWeaponFire();
	}

	GetWorldTimerManager().ClearTimer(TimerHandle_HandleFiring);
	bRefiring = false;
}


//////////////////////////////////////////////////////////////////////////
// Weapon usage helpers

UAudioComponent* ARangedWeapon::PlayWeaponSound(USoundCue* Sound)
{
	UAudioComponent* AC = NULL;
	if (Sound && mCharacterInteractionOwner.IsValid())
	{
		AC = UGameplayStatics::SpawnSoundAttached(Sound, mCharacterInteractionOwner->GetCharacterOwner()->GetRootComponent());
	}

	return AC;
}

FVector ARangedWeapon::GetCameraAim() const
{
	AVertPlayerController* const PlayerController = Instigator ? Cast<AVertPlayerController>(Instigator->Controller) : NULL;
	FVector FinalAim = FVector::ZeroVector;

	if (PlayerController)
	{
		FVector CamLoc;
		FRotator CamRot;
		PlayerController->GetPlayerViewPoint(CamLoc, CamRot);
		FinalAim = CamRot.Vector();
	}
	else if (Instigator)
	{
		FinalAim = Instigator->GetBaseAimRotation().Vector();
	}

	return FinalAim;
}

FVector ARangedWeapon::GetAdjustedAim() const
{
	if (AController* controller = Instigator->GetController())
	{
		UE_LOG(LogTemp, Warning, TEXT("Ding Dong"));
		return controller->GetControlRotation().Vector();
	}

	return FVector::ZeroVector;
}

void ARangedWeapon::ExecuteAttack_Implementation()
{
	if (!AttackAnimation)
	{
		UE_LOG(LogVertBaseWeapon, Warning, TEXT("Weapon %s has no valid attack animation, will not collide."), *GetName());
	}

	Sprite->SetFlipbook(AttackAnimation);
	if (!Sprite->IsPlaying())
		Sprite->Play();

	FireWeapon();
}

FVector ARangedWeapon::GetMuzzleLocation() const
{
	return Sprite->GetSocketLocation(MuzzleAttachPoint);
}

FVector ARangedWeapon::GetMuzzleDirection() const
{
	return Sprite->GetSocketRotation(MuzzleAttachPoint).Vector();
}

FHitResult ARangedWeapon::WeaponTrace(const FVector& StartTrace, const FVector& EndTrace) const
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

void ARangedWeapon::SetOwningPawn(AVertCharacter* NewOwner)
{
	if (!mCharacterInteractionOwner.IsValid() || mCharacterInteractionOwner->GetCharacterOwner() != NewOwner)
	{
		Instigator = NewOwner;
		// net owner for RPC calls
		SetOwner(NewOwner);
	}
}

void ARangedWeapon::OnRep_BurstCounter()
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

void ARangedWeapon::OnRep_Reload()
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

float ARangedWeapon::PlayWeaponAnimation(UPaperFlipbook* newAnim)
{
	if (newAnim)
	{
		Sprite->SetFlipbook(newAnim);
		return newAnim->GetTotalDuration();
	}

	return 0.f;
}

void ARangedWeapon::SimulateWeaponFire()
{
	if (Role == ROLE_Authority && CurrentState != EWeaponState::Firing)
	{
		return;
	}

	if (MuzzleFX)
	{
		if (!LoopedMuzzleFX || MuzzlePSC == NULL)
		{
			// Split screen requires we create 2 effects. One that we see and one that the other player sees.
			if ((mCharacterInteractionOwner.IsValid()) && (mCharacterInteractionOwner->GetCharacterOwner()->IsLocallyControlled() == true))
			{
				AController* PlayerCon = mCharacterInteractionOwner->GetCharacterOwner()->GetController();
				if (PlayerCon != NULL)
				{
					Sprite->GetSocketLocation(MuzzleAttachPoint);
					MuzzlePSC = UGameplayStatics::SpawnEmitterAttached(MuzzleFX, Sprite, MuzzleAttachPoint);
					MuzzlePSC->bOwnerNoSee = false;
					MuzzlePSC->bOnlyOwnerSee = false;
				}
			}
			else
			{
				MuzzlePSC = UGameplayStatics::SpawnEmitterAttached(MuzzleFX, Sprite, MuzzleAttachPoint);
			}
		}
	}

	if (!mLoopedFireAnim || !mPlayingFireAnim)
	{
		PlayWeaponAnimation(AttackAnimation);
		mPlayingFireAnim = true;
	}

	if (FiringMode != EWeaponFiremode::SemiAutomatic)
	{
		if (FireAC == NULL)
		{
			FireAC = PlayWeaponSound(FireLoopSound);
		}
	}
	else
	{
		PlayWeaponSound(FireSound);
	}

	AVertPlayerController* PC = (mCharacterInteractionOwner.IsValid()) ? Cast<AVertPlayerController>(mCharacterInteractionOwner->GetCharacterOwner()->Controller) : NULL;
	if (PC != NULL && PC->IsLocalController())
	{
		if (FireForceFeedback != NULL)
		{
			PC->ClientPlayForceFeedback(FireForceFeedback, false, "Weapon");
		}
	}
}

void ARangedWeapon::StopSimulatingWeaponFire()
{
	if (LoopedMuzzleFX)
	{
		if (MuzzlePSC != NULL)
		{
			MuzzlePSC->DeactivateSystem();
			MuzzlePSC = NULL;
		}
		if (MuzzlePSCSecondary != NULL)
		{
			MuzzlePSCSecondary->DeactivateSystem();
			MuzzlePSCSecondary = NULL;
		}
	}

	if (mLoopedFireAnim && mPlayingFireAnim)
	{
		Sprite->SetFlipbook(DefaultAnimation);
		mPlayingFireAnim = false;
	}

	if (FireAC)
	{
		FireAC->FadeOut(0.1f, 0.0f);
		FireAC = NULL;

		PlayWeaponSound(FireFinishSound);
	}
}

void ARangedWeapon::GetLifetimeReplicatedProps(TArray< FLifetimeProperty > & OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(ARangedWeapon, CurrentAmmo, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(ARangedWeapon, CurrentAmmoInClip, COND_OwnerOnly);

	DOREPLIFETIME_CONDITION(ARangedWeapon, BurstCounter, COND_SkipOwner);
	DOREPLIFETIME_CONDITION(ARangedWeapon, PendingReload, COND_SkipOwner);
}

class AVertCharacter* ARangedWeapon::GetPawnOwner() const
{
	return mCharacterInteractionOwner.IsValid() ? mCharacterInteractionOwner->GetCharacterOwner() : nullptr;
}

bool ARangedWeapon::IsEquipped() const
{
	return mIsEquipped;
}

bool ARangedWeapon::IsAttachedToPawn() const
{
	return mIsEquipped || PendingEquip;
}

EWeaponState::Type ARangedWeapon::GetCurrentState() const
{
	return CurrentState;
}

int32 ARangedWeapon::GetCurrentAmmo() const
{
	return CurrentAmmo;
}

int32 ARangedWeapon::GetCurrentAmmoInClip() const
{
	return CurrentAmmoInClip;
}

int32 ARangedWeapon::GetAmmoPerClip() const
{
	return WeaponConfig.AmmoPerClip;
}

int32 ARangedWeapon::GetMaxAmmo() const
{
	return WeaponConfig.MaxAmmo;
}

bool ARangedWeapon::HasInfiniteAmmo() const
{
	const AVertPlayerController* MyPC = (mCharacterInteractionOwner.IsValid()) ? Cast<const AVertPlayerController>(mCharacterInteractionOwner->GetCharacterOwner()->Controller) : NULL;
	return WeaponConfig.bInfiniteAmmo || (MyPC && MyPC->HasInfiniteWeaponUsage());
}

bool ARangedWeapon::HasInfiniteClip() const
{
	const AVertPlayerController* MyPC = (mCharacterInteractionOwner.IsValid()) ? Cast<const AVertPlayerController>(mCharacterInteractionOwner->GetCharacterOwner()->Controller) : NULL;
	return WeaponConfig.bInfiniteClip || (MyPC && MyPC->HasInfiniteClip());
}

float ARangedWeapon::GetEquipStartedTime() const
{
	return EquipStartedTime;
}

float ARangedWeapon::GetEquipDuration() const
{
	return EquipDuration;
}
