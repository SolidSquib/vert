// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "Vert.h"
#include "VertCharacter.h"
#include "PaperFlipbookComponent.h"
#include "ContainerAllocationPolicies.h"

DEFINE_LOG_CATEGORY(LogVertCharacter);

//////////////////////////////////////////////////////////////////////////
// AVertCharacter

AVertCharacter::AVertCharacter(const FObjectInitializer & ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UVertCharacterMovementComponent>(ACharacter::CharacterMovementComponentName)),
	HealthComponent(CreateDefaultSubobject<UHealthComponent>(TEXT("HealthComponent"))),
	InteractionComponent(CreateDefaultSubobject<UCharacterInteractionComponent>(TEXT("InteractionComponent"))),
	GrapplingComponent(CreateDefaultSubobject<UGrapplingComponent>(TEXT("GrapplingComponent"))),
	DashingComponent(CreateDefaultSubobject<UDashingComponent>(TEXT("DashingComponent"))),
	StateManager(CreateDefaultSubobject<UCharacterStateManager>(TEXT("StateManager"))),
	DisableInputWhenDashingOrGrappling(false)
{
	// Use only Yaw from the controller and ignore the rest of the rotation.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = true;
	bUseControllerRotationRoll = false;

	// Set the size of our collision capsule.
	GetCapsuleComponent()->SetCapsuleHalfHeight(96.0f);
	GetCapsuleComponent()->SetCapsuleRadius(40.0f);
	GetCapsuleComponent()->SetSimulatePhysics(false);

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

void AVertCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	UpdateCharacter();

#if !UE_BUILD_SHIPPING
	PrintDebugInfo();
#endif
}

void AVertCharacter::BeginPlay()
{
	Super::BeginPlay();
	
	if (AController* controller = GetController())
	{
		if (APlayerController* playerController = Cast<APlayerController>(controller))
		{
			playerController->bShowMouseCursor = true;
			playerController->bEnableClickEvents = true;
			playerController->bEnableMouseOverEvents = true;
		}
	}
}

void AVertCharacter::EndPlay(const EEndPlayReason::Type endPlayReason)
{
	if (endPlayReason == EEndPlayReason::Destroyed)
	{
		if (UWorld* world = GetWorld())
		{
			if (AGameModeBase* gameMode = world->GetAuthGameMode())
			{
				if (AVertGameMode* vertGameMode = Cast<AVertGameMode>(gameMode))
				{
					if (AVertPlayerCameraActor* camera = vertGameMode->GetActivePlayerCamera())
					{
						camera->UnregisterPlayerPawn(this);
					}
				}
			}
		}
	}
}

void AVertCharacter::Landed(const FHitResult& Hit)
{
	GrapplingComponent->OnLanded();
	DashingComponent->OnLanded();

	Super::Landed(Hit);
}

//////////////////////////////////////////////////////////////////////////
// Input

void AVertCharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	// Note: the 'Jump' action and the 'MoveRight' axis are bound to actual keys/buttons/sticks in DefaultInput.ini (editable from Project Settings..Input)
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &AVertCharacter::ActionJump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);
	PlayerInputComponent->BindAction("GrappleShoot", IE_Pressed, this, &AVertCharacter::ActionGrappleShoot);
	PlayerInputComponent->BindAction("Dash", IE_Pressed, this, &AVertCharacter::ActionDash);
	PlayerInputComponent->BindAction("Interact", IE_Pressed, this, &AVertCharacter::ActionInteract);

	PlayerInputComponent->BindAxis("MoveRight", this, &AVertCharacter::ActionMoveRight);
	PlayerInputComponent->BindAxis("LeftThumbstickMoveY", this, &AVertCharacter::LeftThumbstickMoveY);
	PlayerInputComponent->BindAxis("RightThumbstickMoveX", this, &AVertCharacter::RightThumbstickMoveX);
	PlayerInputComponent->BindAxis("RightThumbstickMoveY", this, &AVertCharacter::RightThumbstickMoveY);
	PlayerInputComponent->BindAxis("MouseMove", this, &AVertCharacter::MouseMove);
}

void AVertCharacter::ActionMoveRight(float Value)
{
	mAxisPositions.LeftX = Value;

	if (FMath::Abs(Value) > 0.0f)
	{
		StateManager->NotifyActionTaken(ECharacterActions::Move);
	}
}

void AVertCharacter::ActionJump()
{
	if (StateManager->NotifyActionTaken(ECharacterActions::Jump))
	{
		Jump();
	}
}

void AVertCharacter::ActionGrappleShoot()
{
	switch (UsingGamepad())
	{
	case true:
		if (!mGamepadOnStandby)
		{
			mGamepadOnStandby = true;
			GetWorld()->GetTimerManager().SetTimer(mTimerHandle, this, &AVertCharacter::ExecuteActionGrappleShoot, SMALL_NUMBER, false);
			GetWorld()->GetTimerManager().SetTimer(mGamepadGrappleDelay, this, &AVertCharacter::EndGamepadStandby, 0.1f, false);			
		}
		break;

	case false:
		ExecuteActionGrappleShoot();
		break;
	}
}

void AVertCharacter::ExecuteActionGrappleShoot()
{
	StateManager->NotifyActionTaken(ECharacterActions::Grapple);
}

void AVertCharacter::ActionDash()
{
	StateManager->NotifyActionTaken(ECharacterActions::Dash);
}

void AVertCharacter::ActionInteract()
{
	StateManager->NotifyActionTaken(ECharacterActions::Interact);
}

#if !UE_BUILD_SHIPPING
void AVertCharacter::PrintDebugInfo()
{
	int debugIndex = 0;
	if (ShowDebug.CharacterMovement.Enabled)
	{
		GEngine->AddOnScreenDebugMessage(debugIndex++, 3.f, ShowDebug.CharacterMovement.MessageColour, FString::Printf(TEXT("[Character-Movement] Gravity Scale: %f"), GetCharacterMovement()->GravityScale));
		GEngine->AddOnScreenDebugMessage(debugIndex++, 3.f, ShowDebug.CharacterMovement.MessageColour, FString::Printf(TEXT("[Character-Movement] Friction: %f"), GetCharacterMovement()->GroundFriction));
		GEngine->AddOnScreenDebugMessage(debugIndex++, 3.f, ShowDebug.CharacterMovement.MessageColour, FString::Printf(TEXT("[Character-Movement] Is Flying: %s"), GetCharacterMovement()->IsFlying() ? TEXT("true") : TEXT("false")));
		GEngine->AddOnScreenDebugMessage(debugIndex++, 3.f, ShowDebug.CharacterMovement.MessageColour, FString::Printf(TEXT("[Character-Movement] Is Falling: %s"), GetCharacterMovement()->IsFalling() ? TEXT("true") : TEXT("false")));
		GEngine->AddOnScreenDebugMessage(debugIndex++, 3.f, ShowDebug.CharacterMovement.MessageColour, FString::Printf(TEXT("[Character-Movement] Using Gamepad: %s"), UsingGamepad() ? TEXT("true") : TEXT("false")));
	}

	if (ShowDebug.Dash.Enabled)
	{
		FVector dashDirection = mAxisPositions.GetPlayerLeftThumbstickDirection();
		dashDirection = UVertUtilities::LimitAimTrajectory(DashingComponent->AimFreedom, dashDirection);

		DrawDebugDirectionalArrow(GetWorld(), GetActorLocation(), GetActorLocation() + (dashDirection * 500), 100.f, ShowDebug.Dash.MessageColour);

		GEngine->AddOnScreenDebugMessage(debugIndex++, 3.f, ShowDebug.Dash.MessageColour, FString::Printf(TEXT("[Character-Dash] Remaining Dashes: %i / %i"), DashingComponent->GetRemainingDashes(), DashingComponent->MaxDashes));
		GEngine->AddOnScreenDebugMessage(debugIndex++, 3.f, ShowDebug.Dash.MessageColour, FString::Printf(TEXT("[Character-Dash] Recharge at %f% (%s)"), DashingComponent->RechargeTimer.GetProgressPercent(), DashingComponent->RechargeTimer.IsRunning() ? TEXT("active") : TEXT("inactive")));
	}

	if (ShowDebug.Grapple.Enabled)
	{
		GEngine->AddOnScreenDebugMessage(debugIndex++, 3.f, ShowDebug.Grapple.MessageColour, FString::Printf(TEXT("[Character-Grapple] Grapple Launched | %i / %i uses remaining"), GrapplingComponent->GetRemainingGrapples(), GrapplingComponent->MaxGrapples));
		GEngine->AddOnScreenDebugMessage(debugIndex++, 3.f, ShowDebug.Grapple.MessageColour, FString::Printf(TEXT("[Character-Grapple] State: %s"), *UVertUtilities::GetEnumValueToString<EGrappleState>(TEXT("EGrappleState"), GrapplingComponent->GetGrappleHook()->GetGrappleState())));
		GEngine->AddOnScreenDebugMessage(debugIndex++, 3.f, ShowDebug.Grapple.MessageColour, FString::Printf(TEXT("[Character-Grapple] Recharge at %f% (%s)"), GrapplingComponent->GetRechargePercent(), GrapplingComponent->IsRecharging() ? TEXT("active") : TEXT("inactive")));

		DrawDebugDirectionalArrow(GetWorld(), GetActorLocation(), GetActorLocation() + (mAxisPositions.GetPlayerRightThumbstickDirection() * 500), 50.f, ShowDebug.Grapple.MessageColour, false, -1.f, 1, 3.f);
	}

	if (ShowDebug.States.Enabled)
	{
		GEngine->AddOnScreenDebugMessage(debugIndex++, 3.f, ShowDebug.States.MessageColour, FString::Printf(TEXT("[Character-State] Current State: %s"), *UVertUtilities::GetEnumValueToString<ECharacterState>(TEXT("ECharacterState"), StateManager->GetCurrentState())));
	}
}
#endif

bool AVertCharacter::CanComponentRecharge(ERechargeRule rule)
{
	if (GrapplingComponent)
	{
		AActor* hookedActor = GrapplingComponent->GetHookedActor();

		return IsGrounded() ||
			rule == ERechargeRule::OnRechargeTimer ||
			(rule == ERechargeRule::OnContactGroundOrLatchedAnywhere && GrapplingComponent->GetGrappleState() == EGrappleState::Latched);
	}
	else
		UE_LOG(LogVertCharacter, Error, TEXT("Hook associated with character [%s] is not valid."), *GetOwner()->GetName());

	return false;
}

void AVertCharacter::UpdateCharacter()
{
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

bool AVertCharacter::CheckShootGrappleGamepad()
{
	static float deadzone = 0.25f;

	FVector2D axis = FVector2D::ZeroVector;
	if (AVertPlayerController* controller = GetPlayerController())
	{
		controller->GetInputAnalogStickState(EControllerAnalogStick::CAS_RightStick, axis.X, axis.Y);
		axis.Y = -axis.Y;

		if (axis.SizeSquared() > FMath::Square(deadzone))
		{
			return true;
		}
	}

	return false;
}

void AVertCharacter::RightThumbstickMoveX(float value)
{
	mAxisPositions.RightX = value;
}

void AVertCharacter::RightThumbstickMoveY(float value)
{
	mAxisPositions.RightY = value;
}

void AVertCharacter::LeftThumbstickMoveY(float value)
{
	mAxisPositions.LeftY = value;
}

void AVertCharacter::MouseMove(float value)
{	
	if (AVertPlayerController* controller = GetPlayerController())
	{
		const ULocalPlayer* localPlayer = controller->GetLocalPlayer();
		if (localPlayer && localPlayer->ViewportClient)
		{
			FVector2D mousePosition;
			if (localPlayer->ViewportClient->GetMousePosition(mousePosition))
			{
				FVector worldLocation, worldDirection;
				FVector2D playerScreenLocation, mouseDirection;
				if (controller->ProjectWorldLocationToScreen(GetActorLocation(), playerScreenLocation))
				{
					mouseDirection = mousePosition - playerScreenLocation;
					mouseDirection *= 100;
					mouseDirection = mouseDirection.GetSafeNormal();
					mAxisPositions.MouseDirection = FVector(mouseDirection.X, 0.f, -mouseDirection.Y);
				}
			}
		}
	}
}

const bool AVertCharacter::UsingGamepad() const
{
	return GetPlayerController()->UsingGamepad();
}