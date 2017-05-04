// Copyright Inside Out Games Ltd. 2017

#include "Vert.h"
#include "BaseWeapon.h"

DECLARE_LOG_CATEGORY_CLASS(LogVertBaseWeapon, Log, All);

const FName ABaseWeapon::scCollisionProfileName = TEXT("InteractiveItemPhysics");
const FName ABaseWeapon::scAttackingCollisionProfileName = TEXT("AttackingWeapon");

ABaseWeapon::ABaseWeapon()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	Sprite = CreateDefaultSubobject<UPaperFlipbookComponent>(TEXT("Sprite"));
	if (Sprite)
	{
		Sprite->AlwaysLoadOnClient = true;
		Sprite->AlwaysLoadOnServer = true;
		Sprite->bOwnerNoSee = false;
		Sprite->bAffectDynamicIndirectLighting = true;
		Sprite->PrimaryComponentTick.TickGroup = TG_PrePhysics;
		Sprite->SetCollisionProfileName(scCollisionProfileName);
		Sprite->bGenerateOverlapEvents = true;
		Sprite->CanCharacterStepUpOn = ECanBeCharacterBase::ECB_No;
		Sprite->SetConstraintMode(EDOFMode::XZPlane);
		Sprite->SetMassOverrideInKg(NAME_None, 100.f);
		Sprite->SetSimulatePhysics(true);
		Sprite->SetIsReplicated(true);

		
		Sprite->OnComponentHit.AddDynamic(this, &ABaseWeapon::OnHit);
	}
	SetRootComponent(Sprite);

	AttachPoint = CreateDefaultSubobject<USceneComponent>(TEXT("AttachPoint"));
	AttachPoint->SetupAttachment(RootComponent);
	
	bReplicates = true;
}

void ABaseWeapon::BeginPlay()
{
	Super::BeginPlay();

	if (Sprite)
	{
		FScriptDelegate OnAnimationEndedDelegate;
		OnAnimationEndedDelegate.BindUFunction(this, TEXT("AttackAnimationEnd"));
		Sprite->OnFinishedPlaying.Add(OnAnimationEndedDelegate);

		Sprite->OnComponentBeginOverlap.AddDynamic(this, &ABaseWeapon::OnBeginOverlap);

		if (DefaultAnimation)
		{
			Sprite->SetFlipbook(DefaultAnimation);
		}		
	}
}

void ABaseWeapon::EndPlay(const EEndPlayReason::Type endPlayReason)
{
	Super::EndPlay(endPlayReason);

	GetWorld()->GetTimerManager().ClearAllTimersForObject(this);
}

void ABaseWeapon::Attack()
{
	switch (FiringMode)
	{
	case EWeaponFiremode::Automatic:
		Sprite->SetLooping(true);
		GetWorld()->GetTimerManager().SetTimer(mAttackTimer, this, &ABaseWeapon::ExecuteAttack, RateOfFire, true, 0.f);
		break;

	default:
		if (!mIsAttacking)
		{
			mIsAttacking = true;
			Sprite->SetLooping(false);
			
			GetWorld()->GetTimerManager().SetTimer(mDelayTimer, 
				[this]() -> void 
				{ 
					mIsAttacking = false;
				}, 
				RateOfFire, false);
			ExecuteAttack();
		}
		break;
	}
}

void ABaseWeapon::StopAttacking()
{
	if (FiringMode == EWeaponFiremode::Automatic)
	{
		GetWorld()->GetTimerManager().ClearTimer(mAttackTimer);

		Sprite->SetCollisionProfileName(scCollisionProfileName);
	}
}

void ABaseWeapon::ExecuteAttack_Implementation()
{
	if (!AttackAnimation)
	{
		UE_LOG(LogVertBaseWeapon, Warning, TEXT("Weapon %s has no valid attack animation, will not collide."), *GetName());
	}

	Sprite->SetFlipbook(AttackAnimation);
	if (!Sprite->IsPlaying())
		Sprite->Play();

	Sprite->SetCollisionProfileName(scAttackingCollisionProfileName);
}

void ABaseWeapon::Interact(TWeakObjectPtr<class UCharacterInteractionComponent> instigator)
{
	if (instigator.IsValid())
	{
		if (instigator->GetHeldInteractive() != this)
		{
			FVector localOffset = FVector::ZeroVector; // -AttachPoint->RelativeLocation;

			if (instigator->HoldInteractive(this, localOffset, false))
			{
				DisableInteractionDetection();
				mCharacterInteractionOwner = instigator;

				UE_LOG(LogVertBaseWeapon, Log, TEXT("Weapon %s picked up by player %s"), *GetName(), *instigator->GetName());
			}
		}
		else
		{
			NativeOnThrow();
		}
	}
}

void ABaseWeapon::NativeOnThrow()
{
	if (mCharacterInteractionOwner.IsValid())
	{
		mCharacterInteractionOwner->DropInteractive();

		if (AVertCharacter* character = mCharacterInteractionOwner->GetCharacterOwner())
		{
			EnableInteractionDetection();

			FVector launchDirection = UVertUtilities::SnapVectorToAngle(character->GetAxisPostisions().GetPlayerLeftThumbstickDirection(), 45.f);
			Sprite->SetSimulatePhysics(true);
			Sprite->AddImpulse(launchDirection * 100000.f);
			Sprite->AddAngularImpulse(FVector(1.f, 0, 0) * 5000.f);

			if (auto i = Cast<IWeaponPickup>(this))
			{
				i->Execute_OnThrow(Cast<UObject>(i), character);
			}

			UE_LOG(LogVertBaseWeapon, Log, TEXT("Weapon %s thrown by player %s"), *GetName(), *character->GetName());
		}
	}
}

void ABaseWeapon::OnHit(class UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	NativeOnHit(HitComp, OtherActor, OtherComp, NormalImpulse, Hit); 
}

void ABaseWeapon::OnBeginOverlap(class UPrimitiveComponent* overlappedComp, AActor* otherActor, UPrimitiveComponent* otherComp, int32 otherBodyIndex, bool fromSweep, const FHitResult& sweepResult)
{
	NativeOnBeginOverlap(overlappedComp, otherActor, otherComp, otherBodyIndex, fromSweep, sweepResult);
}

void ABaseWeapon::NativeOnHit_Implementation(class UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	UE_LOG(LogVertBaseWeapon, Log, TEXT("Weapon %s hit %s"), *GetName(), OtherActor ? *OtherActor->GetName() : TEXT("NULL"));
}

void ABaseWeapon::NativeOnBeginOverlap_Implementation(class UPrimitiveComponent* overlappedComp, AActor* otherActor, UPrimitiveComponent* otherComp, int32 otherBodyIndex, bool fromSweep, const FHitResult& sweepResult)
{
	UE_LOG(LogVertBaseWeapon, Log, TEXT("Weapon %s begin overlap with %s"), *GetName(), otherActor ? *otherActor->GetName() : TEXT("NULL"));
}

void ABaseWeapon::AttackAnimationEnd_Implementation()
{
	Sprite->SetCollisionProfileName(scCollisionProfileName);
	Sprite->SetPlaybackPositionInFrames(0, false);
	Sprite->SetLooping(true);
	Sprite->Play();
	Sprite->SetFlipbook(DefaultAnimation);
}

void ABaseWeapon::DisableInteractionDetection()
{
	Sprite->SetCollisionObjectType(ECC_HeldWeapon);
	Sprite->SetSpriteColor(FLinearColor::Green);
}

void ABaseWeapon::EnableInteractionDetection()
{
	Sprite->SetCollisionObjectType(ECC_Interactive);
	Sprite->SetSpriteColor(FLinearColor::Red);
}