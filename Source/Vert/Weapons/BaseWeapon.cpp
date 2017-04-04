// Copyright Inside Out Games Ltd. 2017

#include "Vert.h"
#include "BaseWeapon.h"

DECLARE_LOG_CATEGORY_CLASS(LogVertBaseWeapon, Log, All);

ABaseWeapon::ABaseWeapon()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

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

	BaseDamage = 10;

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

void ABaseWeapon::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void ABaseWeapon::Attack()
{
	if (mCharacterInteractionOwner.IsValid())
	{
		AVertCharacter* character = mCharacterInteractionOwner->GetCharacterOwner();
		if (!character)
			return;

		//character->GetSprite()->SetFlipbook(character->AttackAnimation);
		Sprite->SetFlipbook(AttackAnimation);
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
				mCharacterInteractionOwner = instigator;

				UE_LOG(LogVertBaseWeapon, Log, TEXT("Weapon %s picked up by player %s"), *GetName(), *instigator->GetName());
			}
		}
		else
		{
			Throw();
		}
	}
}

void ABaseWeapon::Throw()
{
	if (mCharacterInteractionOwner.IsValid())
	{
		mCharacterInteractionOwner->DropInteractive();

		if (AVertCharacter* character = mCharacterInteractionOwner->GetCharacterOwner())
		{
			FVector launchDirection = UVertUtilities::SnapVectorToAngle(character->GetAxisPostisions().GetPlayerLeftThumbstickDirection(), 45.f);
			Sprite->SetSimulatePhysics(true);
			Sprite->AddImpulse(launchDirection * 100000.f);
			Sprite->AddAngularImpulse(FVector(0, 1.f, 0) * 5000.f);

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

}

void ABaseWeapon::OnBeginOverlap_Implementation(class UPrimitiveComponent* overlappedComp, AActor* otherActor, UPrimitiveComponent* otherComp, int32 otherBodyIndex, bool fromSweep, const FHitResult& sweepResult)
{

}