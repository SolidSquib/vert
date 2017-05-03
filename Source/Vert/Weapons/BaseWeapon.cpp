// Copyright Inside Out Games Ltd. 2017

#include "Vert.h"
#include "BaseWeapon.h"

DECLARE_LOG_CATEGORY_CLASS(LogVertBaseWeapon, Log, All);

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
		static FName CollisionProfileName(TEXT("CharacterMesh"));
		Sprite->SetCollisionProfileName(CollisionProfileName);
		Sprite->bGenerateOverlapEvents = true;
		Sprite->SetCollisionProfileName(TEXT("InteractiveItem"));
		Sprite->SetCollisionObjectType(ECC_Interactive);
		Sprite->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		Sprite->SetConstraintMode(EDOFMode::XZPlane);
		Sprite->SetMassOverrideInKg(NAME_None, 100.f);
		Sprite->SetSimulatePhysics(true);
		Sprite->SetIsReplicated(true);
	}
	SetRootComponent(Sprite);

	AttachPoint = CreateDefaultSubobject<USceneComponent>(TEXT("AttachPoint"));
	AttachPoint->SetupAttachment(RootComponent);
	
	bReplicates = true;
}

void ABaseWeapon::BeginPlay()
{
	Super::BeginPlay();

	if (DefaultAnimation && Sprite)
	{
		Sprite->SetFlipbook(DefaultAnimation);
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
		GetWorld()->GetTimerManager().SetTimer(mAttackTimer, this, &ABaseWeapon::ExecuteAttack, RateOfFire, true, 0.f);
		break;

	default:
		ExecuteAttack();
		break;
	}
}

void ABaseWeapon::StopAttacking()
{
	mIsAttacking = false;
	GetWorld()->GetTimerManager().ClearTimer(mAttackTimer);
}

void ABaseWeapon::ExecuteAttack_Implementation()
{
	if (mCharacterInteractionOwner.IsValid())
	{
		Sprite->SetFlipbook(AttackAnimation);
		UE_LOG(LogVertBaseWeapon, Log, TEXT("%s is attacking with %s"), *mCharacterInteractionOwner->GetCharacterOwner()->GetName(), *GetName());
	}
}

void ABaseWeapon::Interact(TWeakObjectPtr<class UCharacterInteractionComponent> instigator)
{
	if (instigator.IsValid())
	{
		if (instigator->GetHeldInteractive() != this)
		{
			FVector localOffset = -AttachPoint->RelativeLocation;

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

void ABaseWeapon::OnHit_Implementation(class UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	UE_LOG(LogVertBaseWeapon, Log, TEXT("Weapon %s hit %s"), *GetName(), OtherActor ? *OtherActor->GetName() : TEXT("NULL"));
}

void ABaseWeapon::OnBeginOverlap_Implementation(class UPrimitiveComponent* overlappedComp, AActor* otherActor, UPrimitiveComponent* otherComp, int32 otherBodyIndex, bool fromSweep, const FHitResult& sweepResult)
{
	UE_LOG(LogVertBaseWeapon, Log, TEXT("Weapon %s begin overlap with %s"), *GetName(), otherActor ? *otherActor->GetName() : TEXT("NULL"));
}

void ABaseWeapon::DisableInteractionDetection()
{
	Sprite->SetCollisionObjectType(ECC_DisabledInteractive);
}

void ABaseWeapon::EnableInteractionDetection()
{
	Sprite->SetCollisionObjectType(ECC_Interactive);
}