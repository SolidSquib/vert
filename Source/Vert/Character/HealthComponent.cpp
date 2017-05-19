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

void UHealthComponent::EndPlay(EEndPlayReason::Type endPlayReason)
{
	Super::EndPlay(endPlayReason);

	// #Timer manager plays funny in components? hmm...
// 	FTimerManager& timerMan = GetWorld()->GetTimerManager();
// 	if (timerMan.IsTimerActive(mUpdateShownDamageTakenTimer))
// 	{
// 		timerMan.ClearTimer(mUpdateShownDamageTakenTimer);
// 	}
}

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

int32 UHealthComponent::HealDamage(int32 magnitude)
{
	SetDamageTaken(mDamageTaken - magnitude);
	OnHeal.Broadcast();

	return mDamageTaken;
}

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
			mCharacterOwner->ApplyDamageMomentum(weapon->KnockbackMagnitude*GetCurrentDamageModifier(), DamageEvent, PawnInstigator, DamageCauser);
		}
	}
}

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

void UHealthComponent::Kill(const FHitResult& hit)
{
	AVertGameMode* gameMode = Cast<AVertGameMode>(GetWorld()->GetAuthGameMode());
	OnDeath.Broadcast(LastTakeHitInfo);
}

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

void UHealthComponent::OnRep_LastTakeHitInfo()
{
	PlayHit(LastTakeHitInfo.ActualDamage, LastTakeHitInfo.GetDamageEvent(), LastTakeHitInfo.PawnInstigator.Get(), LastTakeHitInfo.DamageCauser.Get());
}

void UHealthComponent::GetLifetimeReplicatedProps(TArray< FLifetimeProperty > & OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(UHealthComponent, LastTakeHitInfo, COND_Custom);
	DOREPLIFETIME(UHealthComponent, DamageModifier);
}