// Copyright Inside Out Games Ltd. 2017

#include "Vert.h"
#include "MeleeWeapon.h"


// Sets default values
AMeleeWeapon::AMeleeWeapon()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	CollisionComponent = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionComponent"));
	if (CollisionComponent)
	{
		CollisionComponent->SetCollisionProfileName(TEXT("InteractiveItem"));
		CollisionComponent->SetCollisionObjectType(ECC_Interactive);
		CollisionComponent->SetSimulatePhysics(false);
	}
	SetRootComponent(CollisionComponent);

	Sprite = CreateOptionalDefaultSubobject<UPaperFlipbookComponent>(APaperCharacter::SpriteComponentName);
	if (Sprite)
	{
		Sprite->AlwaysLoadOnClient = true;
		Sprite->AlwaysLoadOnServer = true;
		Sprite->bOwnerNoSee = false;
		Sprite->bAffectDynamicIndirectLighting = true;
		Sprite->PrimaryComponentTick.TickGroup = TG_PrePhysics;
		Sprite->SetupAttachment(CollisionComponent);
		static FName CollisionProfileName(TEXT("CharacterMesh"));
		Sprite->SetCollisionProfileName(CollisionProfileName);
		Sprite->bGenerateOverlapEvents = false;
		Sprite->SetIsReplicated(true);
		Sprite->SetupAttachment(RootComponent);
	}

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

void AMeleeWeapon::Interact(APawn* instigator)
{

}