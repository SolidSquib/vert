// Copyright Inside Out Games Ltd. 2017

#include "Vert.h"
#include "MeleeWeapon.h"

DECLARE_LOG_CATEGORY_CLASS(LogVertMeleeWeapon, Log, All);

// Sets default values
AMeleeWeapon::AMeleeWeapon()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	Sprite = CreateDefaultSubobject<UPaperFlipbookComponent>(APaperCharacter::SpriteComponentName);
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
		Sprite->SetSimulatePhysics(true);
		Sprite->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		Sprite->SetCollisionProfileName(TEXT("InteractiveItem"));
		Sprite->SetCollisionObjectType(ECC_Interactive);
		Sprite->SetIsReplicated(true);
	}
	SetRootComponent(Sprite);

	AttachPoint = CreateDefaultSubobject<USceneComponent>(TEXT("AttachPoint"));
	AttachPoint->SetupAttachment(RootComponent);

	bReplicates = true;
}

// Called when the game starts or when spawned
void AMeleeWeapon::BeginPlay()
{
	Super::BeginPlay();
	
	if (DefaultAnimation && Sprite)
	{
		Sprite->SetFlipbook(DefaultAnimation);
	}
}

// Called every frame
void AMeleeWeapon::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AMeleeWeapon::Attack()
{

}

void AMeleeWeapon::Interact(TWeakObjectPtr<UCharacterInteractionComponent> instigator)
{
	if (instigator.IsValid())
	{
		if (instigator->GetHeldInteractive() != this)
		{
			FVector localOffset = -AttachPoint->RelativeLocation;

			if (instigator->HoldInteractive(this, localOffset, false))
			{
				mCharacterInteractionOwner = instigator;

				UE_LOG(LogVertMeleeWeapon, Log, TEXT("Weapon %s picked up by player %s"), *GetName(), *instigator->GetName());
			}
		}
		else
		{
			Throw();
		}
	}	
}

void AMeleeWeapon::Throw()
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

			UE_LOG(LogVertMeleeWeapon, Log, TEXT("Weapon %s thrown by player %s"), *GetName(), *character->GetName());
		}
	}
}