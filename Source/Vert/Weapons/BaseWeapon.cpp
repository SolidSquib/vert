// Copyright Inside Out Games Ltd. 2017

#include "Vert.h"
#include "BaseWeapon.h"

DECLARE_LOG_CATEGORY_CLASS(LogVertBaseWeapon, Log, All);

const FName ABaseWeapon::scCollisionProfileName = TEXT("InteractiveItemPhysics");
const FName ABaseWeapon::scAttackingCollisionProfileName = TEXT("AttackingWeapon");

ABaseWeapon::ABaseWeapon(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
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

	InteractiveCollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("InteractiveCollisionComponent"));
	InteractiveCollisionComponent->SetCollisionObjectType(ECC_Interactive);
	InteractiveCollisionComponent->SetCollisionProfileName(TEXT("InteractiveItem"));
	InteractiveCollisionComponent->SetupAttachment(RootComponent);

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

void ABaseWeapon::NotifyAttackCommand()
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

void ABaseWeapon::NotifyStopAttacking()
{
	if (FiringMode == EWeaponFiremode::Automatic)
	{
		GetWorld()->GetTimerManager().ClearTimer(mAttackTimer);

		Sprite->SetFlipbook(DefaultAnimation);
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
				Instigator = mCharacterInteractionOwner->GetCharacterOwner();

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
			OnThrow();

			UE_LOG(LogVertBaseWeapon, Log, TEXT("Weapon %s thrown by player %s"), *GetName(), *character->GetName());
		}
	}
}

bool ABaseWeapon::ShouldDealDamage(AActor* TestActor) const
{
	// if we're an actor on the server, or the actor's role is authoritative, we should register damage
	if (TestActor)
	{
		if (GetNetMode() != NM_Client ||
			TestActor->Role == ROLE_Authority ||
			TestActor->bTearOff)
		{
			return true;
		}
	}

	return false;
}

void ABaseWeapon::DealDamage(const FHitResult& Impact, const FVector& ShootDir)
{
	FPointDamageEvent PointDmg;
	PointDmg.DamageTypeClass = DamageType;
	PointDmg.HitInfo = Impact;
	PointDmg.ShotDirection = ShootDir;
	PointDmg.Damage = BaseDamage;

	Impact.GetActor()->TakeDamage(PointDmg.Damage, PointDmg, mCharacterInteractionOwner->GetCharacterOwner()->GetController(), this);
}

void ABaseWeapon::OnHit_Implementation(class UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	UE_LOG(LogVertBaseWeapon, Log, TEXT("Weapon %s hit %s"), *GetName(), OtherActor ? *OtherActor->GetName() : TEXT("NULL"));
}

void ABaseWeapon::OnBeginOverlap_Implementation(class UPrimitiveComponent* overlappedComp, AActor* otherActor, UPrimitiveComponent* otherComp, int32 otherBodyIndex, bool fromSweep, const FHitResult& sweepResult)
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