// Copyright Inside Out Games Ltd. 2017

#include "Vert.h"
#include "VertSpectator.h"

FName AVertSpectator::SpriteComponentName(TEXT("Sprite0"));

AVertSpectator::AVertSpectator()
{
	Sprite = CreateOptionalDefaultSubobject<UPaperFlipbookComponent>(APaperCharacter::SpriteComponentName);
	if (Sprite)
	{
		Sprite->AlwaysLoadOnClient = true;
		Sprite->AlwaysLoadOnServer = true;
		Sprite->bOwnerNoSee = false;
		Sprite->bAffectDynamicIndirectLighting = true;
		Sprite->PrimaryComponentTick.TickGroup = TG_PrePhysics;
		Sprite->SetupAttachment(GetCollisionComponent());
		static FName CollisionProfileName(TEXT("CharacterMesh"));
		Sprite->SetCollisionProfileName(CollisionProfileName);
		Sprite->bGenerateOverlapEvents = false;
	}

	// Use only Yaw from the controller and ignore the rest of the rotation.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = true;
	bUseControllerRotationRoll = false;

	// Set the size of our collision capsule.
	GetCollisionComponent()->SetSphereRadius(40.0f);
	GetCollisionComponent()->SetSimulatePhysics(false);
	GetCollisionComponent()->SetCollisionProfileName(TEXT("OverlapAll"));
	GetMovementComponent()->UpdatedComponent = GetCollisionComponent();
	GetMovementComponent()->bConstrainToPlane = true;
	GetMovementComponent()->SetPlaneConstraintNormal(FVector(0.0f, -1.0f, 0.0f));

	// Enable replication on the Sprite component so animations show up when networked
	GetSprite()->SetIsReplicated(true);
	bReplicates = true;
}

void AVertSpectator::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	UpdateCharacter();
}

void AVertSpectator::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	if (!IsPendingKill())
	{
		if (Sprite)
		{
			// force animation tick after movement component updates
			if (Sprite->PrimaryComponentTick.bCanEverTick && GetMovementComponent())
			{
				Sprite->PrimaryComponentTick.AddPrerequisite(GetMovementComponent(), GetMovementComponent()->PrimaryComponentTick);
			}
		}
	}
}

void AVertSpectator::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	if (PlayerInputComponent)
	{
		PlayerInputComponent->BindAxis("MoveRight", this, &AVertSpectator::MoveRight);
		PlayerInputComponent->BindAxis("MoveUp", this, &AVertSpectator::MoveUp);
	}	
}

void AVertSpectator::UpdateAnimation()
{
	const FVector PlayerVelocity = GetVelocity();
	const float PlayerSpeedSqr = PlayerVelocity.SizeSquared();

	// Are we moving or standing still?
	UPaperFlipbook* DesiredAnimation = SpookyAnimation;
	if (GetSprite()->GetFlipbook() != DesiredAnimation)
	{
		GetSprite()->SetFlipbook(DesiredAnimation);
	}
}

void AVertSpectator::UpdateCharacter()
{
	// Update animation to match the motion
	UpdateAnimation();

	// Now setup the rotation of the controller based on the direction we are travelling
	const FVector PlayerVelocity = GetVelocity();
	float TravelDirection = PlayerVelocity.X;
	// Set the rotation so that the character faces his direction of travel.
	if (Controller != nullptr)
	{
		if (TravelDirection < 0.0f)
		{
			Controller->SetControlRotation(FRotator(0.0, 180.0f, 0.0f));
		}
		else if (TravelDirection > 0.0f)
		{
			Controller->SetControlRotation(FRotator(0.0f, 0.0f, 0.0f));
		}
	}
}

void AVertSpectator::MoveRight(float Value)
{
	if (Value > 0)
		Value = 1.f;
	else if (Value < 0)
		Value = -1.f;

	// Apply the input to the character motion
	AddMovementInput(FVector(1.0f, 0.0f, 0.0f), Value);
}

void AVertSpectator::MoveUp(float Value)
{
	if (Value > 0)
		Value = 1.f;
	else if (Value < 0)
		Value = -1.f;

	// Apply the input to the character motion
	AddMovementInput(FVector(0.0f, 0.0f, 1.0f), Value);
}
