// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "Vert.h"
#include "VertCharacter.h"
#include "PaperFlipbookComponent.h"

DEFINE_LOG_CATEGORY_STATIC(SideScrollerCharacter, Log, All);
//////////////////////////////////////////////////////////////////////////
// AVertCharacter

AVertCharacter::AVertCharacter(const FObjectInitializer & ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UVertCharacterMovementComponent>(ACharacter::CharacterMovementComponentName)),
	MaxGrapples(1),
	MaxDashes(1),
	DisableInputWhenDashingOrGrappling(false)
{
	// Use only Yaw from the controller and ignore the rest of the rotation.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = true;
	bUseControllerRotationRoll = false;

	// Set the size of our collision capsule.
	GetCapsuleComponent()->SetCapsuleHalfHeight(96.0f);
	GetCapsuleComponent()->SetCapsuleRadius(40.0f);

	// Create a camera boom attached to the root (capsule)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 500.0f;
	CameraBoom->SocketOffset = FVector(0.0f, 0.0f, 75.0f);
	CameraBoom->bAbsoluteRotation = true;
	CameraBoom->bDoCollisionTest = false;
	CameraBoom->RelativeRotation = FRotator(0.0f, -90.0f, 0.0f);

	// Create an orthographic camera (no perspective) and attach it to the boom
	SideViewCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("SideViewCamera"));
	SideViewCameraComponent->ProjectionMode = ECameraProjectionMode::Orthographic;
	SideViewCameraComponent->OrthoWidth = 2048.0f;
	SideViewCameraComponent->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);

	// Prevent all automatic rotation behavior on the camera, character, and camera component
	CameraBoom->bAbsoluteRotation = true;
	SideViewCameraComponent->bUsePawnControlRotation = false;
	SideViewCameraComponent->bAutoActivate = true;
	GetCharacterMovement()->bOrientRotationToMovement = false;

	// Configure character movement
	GetCharacterMovement()->GravityScale = 2.0f;
	GetCharacterMovement()->AirControl = 0.80f;
	GetCharacterMovement()->JumpZVelocity = 1000.f;
	GetCharacterMovement()->GroundFriction = 3.0f;
	GetCharacterMovement()->MaxWalkSpeed = 600.0f;
	GetCharacterMovement()->MaxFlySpeed = 600.0f;

	// Lock character motion onto the XZ plane, so the character can't move in or out of the screen
	GetCharacterMovement()->bConstrainToPlane = true;
	GetCharacterMovement()->SetPlaneConstraintNormal(FVector(0.0f, -1.0f, 0.0f));

	// Behave like a traditional 2D platformer character, with a flat bottom instead of a curved capsule bottom
	// Note: This can cause a little floating when going up inclines; you can choose the tradeoff between better
	// behavior on the edge of a ledge versus inclines by setting this to true or false
	GetCharacterMovement()->bUseFlatBaseForFloorChecks = true;

	// Enable replication on the Sprite component so animations show up when networked
	GetSprite()->SetIsReplicated(true);
	bReplicates = true;
}

//////////////////////////////////////////////////////////////////////////
// Animation

void AVertCharacter::UpdateAnimation()
{
	const FVector PlayerVelocity = GetVelocity();
	const float PlayerSpeedSqr = PlayerVelocity.SizeSquared();

	// Are we moving or standing still?
	UPaperFlipbook* DesiredAnimation = (PlayerSpeedSqr > 0.0f) ? RunningAnimation : IdleAnimation;
	if (GetSprite()->GetFlipbook() != DesiredAnimation)
	{
		GetSprite()->SetFlipbook(DesiredAnimation);
	}
}

void AVertCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	UpdateCharacter();

	if (Dash.IsDashing)
	{
		TickDash(DeltaSeconds);
	}

	RechargeDashAndGrapple(DeltaSeconds);

#if !UE_BUILD_SHIPPING
	PrintDebugInfo();
#endif
}

void AVertCharacter::BeginPlay()
{
	Super::BeginPlay();

	mRemainingGrapples = MaxGrapples;
	mRemainingDashes = MaxDashes;

	if (AController* controller = GetController())
	{
		if (APlayerController* playerController = Cast<APlayerController>(controller))
		{
			playerController->bShowMouseCursor = true;
			playerController->bEnableClickEvents = true;
			playerController->bEnableMouseOverEvents = true;
		}
	}

	if (GrappleClass != nullptr && GrappleHandSocket != NAME_None)
	{
		if (UWorld* world = GetWorld())
		{
			//Setup spawn parameters for the actor.
			FActorSpawnParameters spawnParameters;
			spawnParameters.Name = TEXT("GrappleLauncher");
			spawnParameters.Owner = this;
			spawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

			//Spawn the actor.
			AGrappleLauncher* spawnedGrapple = world->SpawnActor<AGrappleLauncher>(GrappleClass, spawnParameters);

			//Assert that the actor exists.
			check(spawnedGrapple);

			if (spawnedGrapple)
			{
				//Attach it to the player's hand.
				spawnedGrapple->AttachToComponent(GetSprite(), FAttachmentTransformRules(EAttachmentRule::SnapToTarget, EAttachmentRule::KeepWorld, EAttachmentRule::SnapToTarget, false), GrappleHandSocket);
				mGrappleLauncher = spawnedGrapple;
			}
		}
	}
}


void AVertCharacter::PreInitializeComponents()
{
	Super::PreInitializeComponents();
}

//////////////////////////////////////////////////////////////////////////
// Input

void AVertCharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	// Note: the 'Jump' action and the 'MoveRight' axis are bound to actual keys/buttons/sticks in DefaultInput.ini (editable from Project Settings..Input)
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &AVertCharacter::Jump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);
	PlayerInputComponent->BindAction("GrappleShoot", IE_Pressed, this, &AVertCharacter::GrappleShoot);
	PlayerInputComponent->BindAction("GrappleShootGamepad", IE_Pressed, this, &AVertCharacter::GrappleShootGamepad);
	PlayerInputComponent->BindAction("Dash", IE_Pressed, this, &AVertCharacter::DoDash);

	PlayerInputComponent->BindAxis("MoveRight", this, &AVertCharacter::MoveRight);
	PlayerInputComponent->BindAxis("LeftThumbstickMoveX", this, &AVertCharacter::LeftThumbstickMoveX);
	PlayerInputComponent->BindAxis("LeftThumbstickMoveY", this, &AVertCharacter::LeftThumbstickMoveY);
	PlayerInputComponent->BindAxis("RightThumbstickMoveX", this, &AVertCharacter::RightThumbstickMoveX);
	PlayerInputComponent->BindAxis("RightThumbstickMoveY", this, &AVertCharacter::RightThumbstickMoveY);
}


void AVertCharacter::OnHooked_Implementation()
{
	if (DisableInputWhenDashingOrGrappling)
	{
		mDisableDash = mDisableGrapple = mDisableMovement = mDisableJump = false;
	}
}

void AVertCharacter::OnFired_Implementation()
{
	if (DisableInputWhenDashingOrGrappling)
	{
		mDisableDash = mDisableGrapple = mDisableMovement = mDisableJump = true;
	}
}

void AVertCharacter::OnReturned_Implementation()
{
	if (DisableInputWhenDashingOrGrappling)
	{
		mDisableDash = mDisableGrapple = mDisableMovement = mDisableJump = false;
	}
}

void AVertCharacter::OnLatched_Implementation(AGrappleHook* hook)
{
	mDisableMovement = mDisableDash = true;
}

void AVertCharacter::OnUnLatched_Implementation(AGrappleHook* hook)
{
	mDisableMovement = mDisableDash = false;
}

void AVertCharacter::MoveRight(float Value)
{
	if (mDisableMovement)
		return;

	if (Value > 0)
		Value = 1.f;
	else if (Value < 0)
		Value = -1.f;

	// Apply the input to the character motion
	AddMovementInput(FVector(1.0f, 0.0f, 0.0f), Value);
}

void AVertCharacter::Jump()
{
	if (mDisableJump)
		return;

	if (mGrappleLauncher.IsValid())
		mGrappleLauncher->ResetGrapple();

	Super::Jump();
}

void AVertCharacter::RightThumbstickMoveX(float value)
{
	mThumbstickPositions.RightX = value;
}

void AVertCharacter::RightThumbstickMoveY(float value)
{
	mThumbstickPositions.RightY = value;
}

void AVertCharacter::LeftThumbstickMoveX(float value)
{
	mThumbstickPositions.LeftX = value;
}

void AVertCharacter::LeftThumbstickMoveY(float value)
{
	mThumbstickPositions.LeftY = value;
}

void AVertCharacter::GrappleShoot()
{
	if (mDisableGrapple)
		return;

	APlayerController* playerController = Cast<APlayerController>(GetController());
	FVector worldLocation, worldDirection;
	playerController->DeprojectMousePositionToWorld(worldLocation, worldDirection);

	if (mGrappleLauncher.IsValid())
	{
		FVector characterToClick = worldLocation - mGrappleLauncher->GetActorLocation();
		FVector fixedCharToClick(characterToClick.X, 0.f, characterToClick.Z);
		FVector fixedDirection = (fixedCharToClick * 100).GetSafeNormal();

		mGrappleLauncher->FireGrapple(UVertUtilities::LimitAimTrajectory(Grapple.AimFreedom, fixedDirection));
	}
}

void AVertCharacter::GrappleShootGamepad()
{
	if (mDisableGrapple)
		return;

	mGrappleLauncher->FireGrapple(UVertUtilities::LimitAimTrajectory(Grapple.AimFreedom, GetPlayerRightThumbstickDirection()));
}

void AVertCharacter::DoDash()
{
	if (mDisableDash)
		return;

	if (mRemainingDashes <= 0 && !ShowDebug.InfiniteDashGrapple)
		return;

	if (Dash.IsDashing)
		return;

	if (GetVertCharacterMovement()->CanDash())
	{
		if (Dash.AimMode == EDashAimMode::AimDirection)
			Dash.DirectionOfTravel = GetPlayerRightThumbstickDirection();
		else if (Dash.AimMode == EDashAimMode::PlayerDirection)
			Dash.DirectionOfTravel = GetPlayerLeftThumbstickDirection();

		Dash.DirectionOfTravel = UVertUtilities::LimitAimTrajectory(Dash.AimFreedom, Dash.DirectionOfTravel);

		Dash.IsDashing = true;
		if (Dash.DisableGravityWhenDashing)
			GetCharacterMovement()->GravityScale = 0.f;
		GetCharacterMovement()->GroundFriction = 0.f;
		mRemainingDashes--;
	}
}

#if !UE_BUILD_SHIPPING
void AVertCharacter::PrintDebugInfo()
{
	if (ShowDebug.CharacterMovement.Enabled)
	{
		GEngine->AddOnScreenDebugMessage(0, 3.f, ShowDebug.CharacterMovement.MessageColour, FString::Printf(TEXT("[Character-Movement] Gravity Scale: %f"), GetCharacterMovement()->GravityScale));
		GEngine->AddOnScreenDebugMessage(1, 3.f, ShowDebug.CharacterMovement.MessageColour, FString::Printf(TEXT("[Character-Movement] Friction: %f"), GetCharacterMovement()->GroundFriction));
	}

	if (ShowDebug.Dash.Enabled)
	{
		FVector dashDirection = (Dash.AimMode == EDashAimMode::PlayerDirection) ? GetPlayerLeftThumbstickDirection() : GetPlayerRightThumbstickDirection();
		dashDirection = UVertUtilities::LimitAimTrajectory(Dash.AimFreedom, dashDirection);

		DrawDebugDirectionalArrow(GetWorld(), GetActorLocation(), GetActorLocation() + (dashDirection * 500), 100.f, ShowDebug.Dash.MessageColour);

		GEngine->AddOnScreenDebugMessage(2, 3.f, ShowDebug.Dash.MessageColour, FString::Printf(TEXT("[Character-Dash] Remaining Dashes: %i / %i"), mRemainingDashes, MaxDashes));
	}

	if (ShowDebug.Grapple.Enabled)
	{
		GEngine->AddOnScreenDebugMessage(3, 3.f, ShowDebug.Grapple.MessageColour, FString::Printf(TEXT("[Character-Grapple] Grapple Launched | %i / %i uses remaining"), mRemainingGrapples, MaxGrapples));
		GEngine->AddOnScreenDebugMessage(4, 3.f, ShowDebug.Grapple.MessageColour, FString::Printf(TEXT("[Character-Grapple] State: %s"), *UVertUtilities::GetEnumValueToString<EGrappleState>(TEXT("EGrappleState"), mGrappleLauncher->GetGrappleHook()->GetGrappleState())));
		GEngine->AddOnScreenDebugMessage(5, 3.f, ShowDebug.Grapple.MessageColour, FString::Printf(TEXT("[Character-Grapple] Hook Velocity: %f, %f"), mGrappleLauncher->GetGrappleHook()->GetHookVelocity().X, mGrappleLauncher->GetGrappleHook()->GetHookVelocity().Z));
		GEngine->AddOnScreenDebugMessage(6, 3.f, ShowDebug.Grapple.MessageColour, FString::Printf(TEXT("[Character-Grapple] Hook Active: %s"), (mGrappleLauncher->GetGrappleHook()->GetProjectileMovementComponentIsActive()) ? TEXT("true") : TEXT("false")));

		DrawDebugDirectionalArrow(GetWorld(), GetActorLocation(), GetActorLocation() + (GetPlayerRightThumbstickDirection() * 500), 50.f, ShowDebug.Grapple.MessageColour, false, -1.f, 1, 3.f);
	}
}
#endif

void AVertCharacter::TickDash(float deltaSeconds)
{
	if (Dash.UseMomentum) // Add a one time impulse in the desired direction
	{
		if (Dash.Timer <= 0.f)
			LaunchCharacter(Dash.DirectionOfTravel*Dash.LaunchForce, Dash.OverrideXY, Dash.OverrideZ);

		Dash.Timer += deltaSeconds;

		if (Dash.Timer >= Dash.TimeToDash)
		{
			Dash.IsDashing = false;
			Dash.Timer = 0.f;
		}
	}
	else // Else it's a timed thing, and we gotta use more tick ¯\_(ツ)_/¯
	{
		float distanceToTravel = Dash.LinearSpeed*deltaSeconds;
		float remainingDistance = Dash.DashLength - Dash.DistanceTravelled;
		SetActorLocation(GetActorLocation() + (Dash.DirectionOfTravel * FMath::Min(distanceToTravel, remainingDistance)));

		Dash.DistanceTravelled += distanceToTravel;

		if (remainingDistance <= distanceToTravel)
		{
			Dash.IsDashing = false;
			Dash.DistanceTravelled = 0.f;
		}
	}

	if (!Dash.IsDashing)
	{
		GetVertCharacterMovement()->LoadGravityScale();
		GetVertCharacterMovement()->LoadGroundFriction();

		if (Dash.RechargeMode == ERechargeRule::OnContactGroundOrWall || Dash.RechargeMode == ERechargeRule::OnContactGround)
		{
			FFindFloorResult findFloorResult;
			GetCharacterMovement()->FindFloor(GetActorLocation(), findFloorResult, false);

			if (findFloorResult.FloorDist <= 0)
			{
				mRemainingDashes = MaxDashes;
			}
		}
	}
}

void AVertCharacter::UpdateCharacter()
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

void AVertCharacter::RechargeDashAndGrapple(float deltaTime)
{
	if (mRemainingDashes < MaxDashes)
	{

	}
}

void AVertCharacter::RegisterGrappleHookDelegates(AGrappleHook* hook)
{
	if (hook)
	{
		FScriptDelegate onHookedDelegate;
		onHookedDelegate.BindUFunction(this, TEXT("OnHooked"));
		hook->OnHooked.Add(onHookedDelegate);

		FScriptDelegate onFiredDelegate;
		onFiredDelegate.BindUFunction(this, TEXT("OnFired"));
		hook->OnFired.Add(onFiredDelegate);

		FScriptDelegate onReturnedDelegate;
		onReturnedDelegate.BindUFunction(this, TEXT("OnReturned"));
		hook->OnReturned.Add(onReturnedDelegate);

		FScriptDelegate onLatchedDelegate;
		onLatchedDelegate.BindUFunction(this, TEXT("OnLatched"));
		hook->OnLatched.Add(onLatchedDelegate);

		FScriptDelegate onUnLatchedDelegate;
		onUnLatchedDelegate.BindUFunction(this, TEXT("OnUnLatched"));
		hook->OnUnLatched.Add(onUnLatchedDelegate);

		if (UVertCharacterMovementComponent* movement = GetVertCharacterMovement())
		{
			movement->RegisterHookDelegates(hook);
		}
	}
}

bool AVertCharacter::IsGrounded() const
{
	FFindFloorResult findFloorResult;
	GetCharacterMovement()->FindFloor(GetActorLocation(), findFloorResult, false);

	if (findFloorResult.FloorDist <= 0)
	{
		return true;
	}

	return false;
}