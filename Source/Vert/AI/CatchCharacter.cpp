// Copyright Inside Out Games Ltd. 2017

#include "CatchCharacter.h"
#include "VertCharacter.h"
#include "GameFramework/Controller.h"
#include "Engine/VertPlayerController.h"

DECLARE_LOG_CATEGORY_CLASS(LogCatch, Log, All);

// Sets default values
ACatchCharacter::ACatchCharacter(const class FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	GetCharacterMovement()->bOrientRotationToMovement = false;

 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

// Called when the game starts or when spawned
//************************************
// Method:    BeginPlay
// FullName:  ACatchCharacter::BeginPlay
// Access:    virtual protected 
// Returns:   void
// Qualifier:
//************************************
void ACatchCharacter::BeginPlay()
{
	Super::BeginPlay();
	
	if (AVertGameMode* gameMode = GetWorld()->GetAuthGameMode<AVertGameMode>())
	{
		for (auto controller : gameMode->GetPlayerControllerArray())
		{
			mDamageMap.Add(controller, 0);
		}
	}
}

//************************************
// Method:    UpdateCharacter
// FullName:  ACatchCharacter::UpdateCharacter
// Access:    protected 
// Returns:   void
// Qualifier:
//************************************
void ACatchCharacter::UpdateCharacter(AActor* targetActor /*= nullptr*/, const FVector& targetVector /*= FVector::ZeroVector*/)
{
	// Now setup the rotation of the controller based on the direction we are travelling
	const FVector PlayerDirection = targetActor ? targetActor->GetActorLocation() - GetActorLocation() : targetVector - GetActorLocation();

	// Set the rotation so that the character faces his direction of travel.
	if (Controller != nullptr)
	{
		if (PlayerDirection.X < 0.0f)
		{
			Controller->SetControlRotation(FRotator(0.0, 180.0f, 0.0f));
		}
		else if (PlayerDirection.X > 0.0f)
		{
			Controller->SetControlRotation(FRotator(0.0f, 0.0f, 0.0f));
		}
	}
}

//************************************
// Method:    NotifyArenaIsReadyToEnter
// FullName:  ACatchCharacter::NotifyArenaIsReadyToEnter
// Access:    public 
// Returns:   void
// Qualifier:
//************************************
void ACatchCharacter::NotifyArenaIsReadyToEnter()
{
	mIsWaitingToEnterArena = false;
}

//************************************
// Method:    NotifyEntryStateFinished
// FullName:  ACatchCharacter::NotifyEntryStateFinished
// Access:    public 
// Returns:   void
// Qualifier:
//************************************
void ACatchCharacter::NotifyEntryStateFinished()
{
	mIsReadyForCombat = true;
	OnReadyToAttack.Broadcast();
}

//************************************
// Method:    ExecuteRoarAttack
// FullName:  ACatchCharacter::ExecuteRoarAttack
// Access:    public 
// Returns:   void
// Qualifier:
//************************************
void ACatchCharacter::ExecuteRoarAttack_Implementation()
{
	// implement in blueprint.
	GetWorldTimerManager().SetTimer(mTimerHandle_ChargeCooldown, RoarCooldownTime, false);
}

//************************************
// Method:    ExecuteChestPound_Implementation
// FullName:  ACatchCharacter::ExecuteChestPound_Implementation
// Access:    public 
// Returns:   void
// Qualifier:
//************************************
void ACatchCharacter::ExecuteChestPound_Implementation()
{
	// implement in blueprint.
	GetWorldTimerManager().SetTimer(mTimerHandle_ChargeCooldown, ChestPoundCooldownTime, false);
}

//************************************
// Method:    ExecutePunchAttack_Implementation
// FullName:  ACatchCharacter::ExecutePunchAttack_Implementation
// Access:    public 
// Returns:   void
// Qualifier:
//************************************
void ACatchCharacter::ExecutePunchAttack_Implementation()
{
	// implement in blueprint.
	GetWorldTimerManager().SetTimer(mTimerHandle_ChargeCooldown, PunchCooldownTime, false);
}

//************************************
// Method:    ExecuteSlamAttack_Implementation
// FullName:  ACatchCharacter::ExecuteSlamAttack_Implementation
// Access:    public 
// Returns:   void
// Qualifier:
//************************************
void ACatchCharacter::ExecuteSlamAttack_Implementation()
{
	//implement in blueprint.
	GetWorldTimerManager().SetTimer(mTimerHandle_ChargeCooldown, SlamCooldownTime, false);
}

//************************************
// Method:    ExecuteSpitAttack_Implementation
// FullName:  ACatchCharacter::ExecuteSpitAttack_Implementation
// Access:    public 
// Returns:   void
// Qualifier:
//************************************
void ACatchCharacter::ExecuteSpitAttack_Implementation()
{
	//implement in blueprint.
	GetWorldTimerManager().SetTimer(mTimerHandle_ChargeCooldown, SpitCooldownTime, false);
}

//************************************
// Method:    ExecuteChargeAttack_Implementation
// FullName:  ACatchCharacter::ExecuteChargeAttack_Implementation
// Access:    public 
// Returns:   void
// Qualifier:
//************************************
void ACatchCharacter::ExecuteChargeAttack_Implementation()
{
	//implement in blueprint.
	GetWorldTimerManager().SetTimer(mTimerHandle_ChargeCooldown, ChargeCooldownTime, false);
}

//************************************
// Method:    IsRunning
// FullName:  ACatchCharacter::IsRunning
// Access:    public 
// Returns:   bool
// Qualifier: const
//************************************
bool ACatchCharacter::IsRunning() const
{
	return IsGrounded() && FMath::Abs(GetVelocity().X) > KINDA_SMALL_NUMBER;
}

//************************************
// Method:    Landed
// FullName:  ACatchCharacter::Landed
// Access:    virtual protected 
// Returns:   void
// Qualifier:
// Parameter: const FHitResult & Hit
//************************************
void ACatchCharacter::Landed(const FHitResult& Hit)
{
	Super::Landed(Hit);

	if (LandingCameraShake)
	{
		if (AVertGameMode* gameMode = GetWorld()->GetAuthGameMode<AVertGameMode>())
		{
			TArray<APlayerController*> controllers = gameMode->GetPlayerControllerArray();
			for (auto pc : controllers)
			{
				float shakeScale = (GetVelocity().Z / -4000.f) * 10.f;
				pc->ClientPlayCameraShake(LandingCameraShake, FMath::Clamp(shakeScale, 0.f, 25.f));

				if (AVertCharacter* player = Cast<AVertCharacter>(pc->GetPawn()))
				{
					if (Hit.Actor.IsValid() && ((player->GetFloorActor() == Hit.Actor.Get()) || player->GetGrapplingComponent()->GetHookedActor() == Hit.Actor.Get()))
					{
						player->Dislodge();
					}
				}
			}
		}
	}

	OnCatchLanded.Broadcast();
}

const TMap<AController*, float>& ACatchCharacter::GetDamageDealtMap() const
{
	return mDamageMap;
}

void ACatchCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearAllTimersForObject(this);

	Super::EndPlay(EndPlayReason);
}

//************************************
// Method:    NotifyJumpApex
// FullName:  ACatchCharacter::NotifyJumpApex
// Access:    virtual protected 
// Returns:   void
// Qualifier:
//************************************
void ACatchCharacter::NotifyJumpApex()
{
	Super::NotifyJumpApex();
	OnJumpApex();
}

float ACatchCharacter::TakeDamage(float Damage, const FDamageEvent& DamageEvent, class AController* EventInstigator, class AActor* DamageCauser)
{
	float damageTaken = Super::TakeDamage(Damage, DamageEvent, EventInstigator, DamageCauser);

	if (EventInstigator)
	{
		if (mDamageMap.Contains(EventInstigator))
		{
			mDamageMap[EventInstigator] += damageTaken;
		}
		else
		{
			mDamageMap.Add(EventInstigator, damageTaken);
		}
	}

	AVertGameMode* GameMode = GetWorld()->GetAuthGameMode<AVertGameMode>();
	AVertPlayerController* PlayerController = Cast<AVertPlayerController>(EventInstigator);
	if (GameMode && PlayerController && !PlayerController->IsPendingKill())
	{
		AVertPlayerState* PlayerState = Cast<AVertPlayerState>(PlayerController->PlayerState);
		GameMode->ScoreMonsterHunter(damageTaken, PlayerState);
	}

	return damageTaken;
}

//************************************
// Method:    IsChestPoundOnCooldown
// FullName:  ACatchCharacter::IsChestPoundOnCooldown
// Access:    public 
// Returns:   bool
// Qualifier: const
//************************************
bool ACatchCharacter::IsChestPoundOnCooldown() const
{
	return GetWorldTimerManager().IsTimerActive(mTimerHandle_ChestPoundCooldown);
}

//************************************
// Method:    IsChargeOnCooldown
// FullName:  ACatchCharacter::IsChargeOnCooldown
// Access:    public 
// Returns:   bool
// Qualifier: const
//************************************
bool ACatchCharacter::IsChargeOnCooldown() const
{
	return GetWorldTimerManager().IsTimerActive(mTimerHandle_ChargeCooldown);
}

//************************************
// Method:    IsSpitOnCooldown
// FullName:  ACatchCharacter::IsSpitOnCooldown
// Access:    public 
// Returns:   bool
// Qualifier: const
//************************************
bool ACatchCharacter::IsSpitOnCooldown() const
{
	return GetWorldTimerManager().IsTimerActive(mTimerHandle_SpitCooldown);
}

//************************************
// Method:    IsSlamOnCooldown
// FullName:  ACatchCharacter::IsSlamOnCooldown
// Access:    public 
// Returns:   bool
// Qualifier: const
//************************************
bool ACatchCharacter::IsSlamOnCooldown() const
{
	return GetWorldTimerManager().IsTimerActive(mTimerHandle_SlamCooldown);
}

//************************************
// Method:    IsPunchOnCooldown
// FullName:  ACatchCharacter::IsPunchOnCooldown
// Access:    public 
// Returns:   bool
// Qualifier: const
//************************************
bool ACatchCharacter::IsPunchOnCooldown() const
{
	return GetWorldTimerManager().IsTimerActive(mTimerHandle_PunchCooldown);
}

//************************************
// Method:    IsRoarOnCooldown
// FullName:  ACatchCharacter::IsRoarOnCooldown
// Access:    public 
// Returns:   bool
// Qualifier: const
//************************************
bool ACatchCharacter::IsRoarOnCooldown() const
{
	return GetWorldTimerManager().IsTimerActive(mTimerHandle_RoarCooldown);
}

//************************************
// Method:    ChooseTargetActor
// FullName:  ACatchCharacter::ChooseTargetActor
// Access:    public 
// Returns:   AActor*
// Qualifier:
//************************************
AActor* ACatchCharacter::ChooseTargetActor()
{
	AController* targetPlayer = nullptr;
	float mostDamage = 0;

	TArray<AController*> mControllers;

	for (auto iter = mDamageMap.CreateIterator(); iter; ++iter)
	{
		if (iter.Key()->GetPawn() && !iter.Key()->GetPawn()->IsPendingKill() && (!targetPlayer || iter.Value() > mostDamage))
		{
			targetPlayer = iter.Key();
			mostDamage = iter.Value();
		}

		mControllers.Add(iter.Key());
	}

	if (targetPlayer)
	{
		UE_LOG(LogCatch, Log, TEXT("Target is highest damaging player"));
		return targetPlayer->GetPawn();
	}
	else if (mControllers.Num() > 0)
	{
		UE_LOG(LogCatch, Log, TEXT("Target is random controller"));
		return mControllers[FMath::RandRange(0, mControllers.Num() - 1)];
	}

	UE_LOG(LogCatch, Log, TEXT("Target is no-one"));
	return nullptr;
}

//************************************
// Method:    GetDamageDealtByPlayer
// FullName:  ACatchCharacter::GetDamageDealtByPlayer
// Access:    public 
// Returns:   float
// Qualifier: const
// Parameter: AController * player
//************************************
float ACatchCharacter::GetDamageDealtByPlayer(AController* player) const
{
	if (mDamageMap.Contains(player))
	{
		return mDamageMap[player];
	}

	return 0.f;
}