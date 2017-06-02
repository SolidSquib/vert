// Copyright Inside Out Games Ltd. 2017

#include "Vert.h"
#include "HealthComponent.h"

// Sets default values for this component's properties
UHealthComponent::UHealthComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	DamageModifier = 1.f;
}

void UHealthComponent::BeginPlay()
{
	Super::BeginPlay();

	AActor* owner = GetOwner();
	if (ACharacter* character = Cast<ACharacter>(owner))
	{
		mCharacterOwner = character;
	}

	SetDamageTaken(mDamageTaken);
}

//************************************
// Method:    DealDamage
// FullName:  UHealthComponent::DealDamage
// Access:    public 
// Returns:   int32
// Qualifier:
// Parameter: float DamageTaken
// Parameter: const FDamageEvent & DamageEvent
// Parameter: APawn * PawnInstigator
// Parameter: AActor * DamageCauser
//************************************
int32 UHealthComponent::DealDamage(float DamageTaken, const FDamageEvent& DamageEvent, APawn* PawnInstigator, AActor* DamageCauser)
{
	check(mCharacterOwner.IsValid());
	
	// #MI_TODO: setup damage types (resistences etc)
	// or modify based on game rules:
	// AVertGameMode* const Game = GetWorld()->GetAuthGameMode<AVertGameMode>();
	// Damage = Game ? Game->ModifyDamage(Damage, this, DamageEvent, EventInstigator, DamageCauser) : 0.f;

	PlayHit(DamageTaken, DamageEvent, PawnInstigator, DamageCauser);
	mCharacterOwner->MakeNoise(1.0f, PawnInstigator ? PawnInstigator : mCharacterOwner.Get());
	
	SetDamageTaken(mDamageTaken + DamageTaken);

	return mDamageTaken;
}

//************************************
// Method:    HealDamage
// FullName:  UHealthComponent::HealDamage
// Access:    public 
// Returns:   int32
// Qualifier:
// Parameter: int32 magnitude
//************************************
int32 UHealthComponent::HealDamage(int32 magnitude)
{
	SetDamageTaken(mDamageTaken - magnitude);
	OnHeal.Broadcast();

	return mDamageTaken;
}

//************************************
// Method:    SetDamageTaken
// FullName:  UHealthComponent::SetDamageTaken
// Access:    private 
// Returns:   void
// Qualifier:
// Parameter: int32 totalDamage
//************************************
void UHealthComponent::SetDamageTaken(int32 totalDamage)
{
	mDamageTaken = totalDamage;

	FTimerManager& timerMan = GetWorld()->GetTimerManager();
	if (!timerMan.IsTimerActive(mUpdateShownDamageTakenTimer))
	{
		timerMan.SetTimer(mUpdateShownDamageTakenTimer, [this, &timerMan]() -> void {
			mShownDamageTakenFloat = FMath::FInterpConstantTo(mShownDamageTakenFloat, mDamageTaken, GetWorld()->GetDeltaSeconds(), DamageApplicationRate);
			mShownDamageTaken = FMath::FloorToInt(mShownDamageTakenFloat);
			if (mShownDamageTaken == mDamageTaken)
			{
				timerMan.ClearTimer(mUpdateShownDamageTakenTimer);
			}
		}, 1 / 60.f, true, 0.f);
	}

	if (mCharacterOwner.IsValid() && mCharacterOwner->PlayerState)
	{
		AVertPlayerState* playerState = Cast<AVertPlayerState>(mCharacterOwner->PlayerState);
		if (playerState)
			playerState->SetDamageTaken(mDamageTaken, mShownDamageTaken);
	}
}

//************************************
// Method:    PlayHit
// FullName:  UHealthComponent::PlayHit
// Access:    virtual protected 
// Returns:   void
// Qualifier:
// Parameter: float DamageTaken
// Parameter: const FDamageEvent & DamageEvent
// Parameter: APawn * PawnInstigator
// Parameter: AActor * DamageCauser
//************************************
void UHealthComponent::PlayHit(float DamageTaken, const FDamageEvent& DamageEvent, APawn* PawnInstigator, AActor* DamageCauser)
{
	check(mCharacterOwner.IsValid());

	if (mCharacterOwner->Role == ROLE_Authority)
	{
		ReplicateHit(DamageTaken, DamageEvent, PawnInstigator, DamageCauser, false);

		// play the force feedback effect on the client player controller
		APlayerController* PC = Cast<APlayerController>(mCharacterOwner->GetController());
		if (PC && DamageEvent.DamageTypeClass)
		{
			UVertDamageType* DamageType = Cast<UVertDamageType>(DamageEvent.DamageTypeClass->GetDefaultObject());
			if (DamageType && DamageType->HitForceFeedback)
			{
				PC->ClientPlayForceFeedback(DamageType->HitForceFeedback, false, "Damage");
			}
		}
	}

	if (DamageTaken > 0.f)
	{
		if (const ABaseWeapon* weapon = Cast<ABaseWeapon>(DamageCauser))
		{
			//LaunchCharacter(DamageEvent, weapon);
			mCharacterOwner->ApplyDamageMomentum(weapon->GetKnockbackMagnitude()*GetCurrentDamageModifier(), DamageEvent, PawnInstigator, DamageCauser);
		}
	}
}

//************************************
// Method:    ReplicateHit
// FullName:  UHealthComponent::ReplicateHit
// Access:    protected 
// Returns:   void
// Qualifier:
// Parameter: float Damage
// Parameter: const FDamageEvent & DamageEvent
// Parameter: APawn * PawnInstigator
// Parameter: AActor * DamageCauser
// Parameter: bool bKilled
//************************************
void UHealthComponent::ReplicateHit(float Damage, const FDamageEvent& DamageEvent, APawn* PawnInstigator, AActor* DamageCauser, bool bKilled)
{
	const float TimeoutTime = GetWorld()->GetTimeSeconds() + 0.5f;

	FDamageEvent const& LastDamageEvent = LastTakeHitInfo.GetDamageEvent();
	if ((PawnInstigator == LastTakeHitInfo.PawnInstigator.Get()) && (LastDamageEvent.DamageTypeClass == LastTakeHitInfo.DamageTypeClass) && (mLastTakeHitTimeTimeout == TimeoutTime))
	{
		// same frame damage
		if (bKilled && LastTakeHitInfo.bKilled)
		{
			// Redundant death take hit, just ignore it
			return;
		}

		// otherwise, accumulate damage done this frame
		Damage += LastTakeHitInfo.ActualDamage;
	}

	LastTakeHitInfo.ActualDamage = Damage;
	LastTakeHitInfo.PawnInstigator = Cast<AVertCharacter>(PawnInstigator);
	LastTakeHitInfo.DamageCauser = DamageCauser;
	LastTakeHitInfo.SetDamageEvent(DamageEvent);
	LastTakeHitInfo.bKilled = bKilled;
	LastTakeHitInfo.EnsureReplication();

	mLastTakeHitTimeTimeout = TimeoutTime;
}

//************************************
// Method:    GetCurrentDamageModifier
// FullName:  UHealthComponent::GetCurrentDamageModifier
// Access:    public 
// Returns:   int32
// Qualifier: const
//************************************
int32 UHealthComponent::GetCurrentDamageModifier() const
{
	// #MI_TODO: Something will probably need changing here for networking purposes.

 	switch (EffectiveDamageType)
 	{
 	case EEffectiveDamageType::ActualDamageModifier:
		return mDamageTaken;
 	case EEffectiveDamageType::ShownDamageModifier:
		return mShownDamageTaken;
 	default:
 		return 1;
 		break;		
 	}
}

//************************************
// Method:    OnRep_LastTakeHitInfo
// FullName:  UHealthComponent::OnRep_LastTakeHitInfo
// Access:    protected 
// Returns:   void
// Qualifier:
//************************************
void UHealthComponent::OnRep_LastTakeHitInfo()
{
	PlayHit(LastTakeHitInfo.ActualDamage, LastTakeHitInfo.GetDamageEvent(), LastTakeHitInfo.PawnInstigator.Get(), LastTakeHitInfo.DamageCauser.Get());
}

//************************************
// Method:    GetLifetimeReplicatedProps
// FullName:  UHealthComponent::GetLifetimeReplicatedProps
// Access:    public 
// Returns:   void
// Qualifier: const
// Parameter: TArray< FLifetimeProperty > & OutLifetimeProps
//************************************
void UHealthComponent::GetLifetimeReplicatedProps(TArray< FLifetimeProperty > & OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(UHealthComponent, LastTakeHitInfo, COND_Custom);
	DOREPLIFETIME(UHealthComponent, DamageModifier);
}

//************************************
// Method:    PreReplication
// FullName:  UHealthComponent::PreReplication
// Access:    virtual public 
// Returns:   void
// Qualifier:
// Parameter: IRepChangedPropertyTracker & ChangedPropertyTracker
//************************************
void UHealthComponent::PreReplication(IRepChangedPropertyTracker & ChangedPropertyTracker)
{
	Super::PreReplication(ChangedPropertyTracker);

	// Only replicate this property for a short duration after it changes so join in progress players don't get spammed with fx when joining late
	DOREPLIFETIME_ACTIVE_OVERRIDE(UHealthComponent, LastTakeHitInfo, GetWorld() && GetWorld()->GetTimeSeconds() < mLastTakeHitTimeTimeout);
}
