// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "Vert.h"
#include "VertCharacter.h"
#include "PaperFlipbookComponent.h"

DEFINE_LOG_CATEGORY(LogVertCharacter);

//////////////////////////////////////////////////////////////////////////
// AVertCharacter

AVertCharacter::AVertCharacter(const FObjectInitializer & ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UVertCharacterMovementComponent>(ACharacter::CharacterMovementComponentName)),
	HealthComponent(CreateDefaultSubobject<UHealthComponent>(TEXT("HealthComponent"))),
	InteractionComponent(CreateDefaultSubobject<UCharacterInteractionComponent>(TEXT("InteractionComponent"))),
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

	SortAbilityRechargeState();
	Dash.RechargeTimer.TickTimer(DeltaSeconds);
	Grapple.RechargeTimer.TickTimer(DeltaSeconds);

#if !UE_BUILD_SHIPPING
	PrintDebugInfo();
#endif
}

void AVertCharacter::BeginPlay()
{
	Super::BeginPlay();

	mRemainingGrapples = MaxGrapples;
	mRemainingDashes = MaxDashes;

	Dash.RechargeTimer.BindAlarm(this, TEXT("OnDashRechargeTimerFinished"));
	Grapple.RechargeTimer.BindAlarm(this, TEXT("OnGrappleRechargeTimerFinished"));

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
			//spawnParameters.Name = TEXT("GrappleLauncher");
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

void AVertCharacter::PreInitializeComponents()
{
	Super::PreInitializeComponents();
}

void AVertCharacter::Landed(const FHitResult& Hit)
{
	if (Dash.RecieveChargeOnGroundOnly)
	{
		mRemainingDashes += Dash.RechargeTimer.PopAlarmBacklog();
	}

	if (Grapple.RecieveChargeOnGroundOnly)
	{
		mRemainingGrapples += Grapple.RechargeTimer.PopAlarmBacklog();
	}
}

//////////////////////////////////////////////////////////////////////////
// Input

void AVertCharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	// Note: the 'Jump' action and the 'MoveRight' axis are bound to actual keys/buttons/sticks in DefaultInput.ini (editable from Project Settings..Input)
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &AVertCharacter::Jump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);
	PlayerInputComponent->BindAction("GrappleShootMK", IE_Pressed, this, &AVertCharacter::GrappleShootMK);
	PlayerInputComponent->BindAction("DashMK", IE_Pressed, this, &AVertCharacter::DashMK);
	PlayerInputComponent->BindAction("DashGamepad", IE_Pressed, this, &AVertCharacter::DashGamepad);
	PlayerInputComponent->BindAction("Interact", IE_Pressed, this, &AVertCharacter::Interact);

	PlayerInputComponent->BindAxis("MoveRight", this, &AVertCharacter::MoveRight);
	PlayerInputComponent->BindAxis("LeftThumbstickMoveX", this, &AVertCharacter::LeftThumbstickMoveX);
	PlayerInputComponent->BindAxis("LeftThumbstickMoveY", this, &AVertCharacter::LeftThumbstickMoveY);
	PlayerInputComponent->BindAxis("RightThumbstickMoveX", this, &AVertCharacter::RightThumbstickMoveX);
	PlayerInputComponent->BindAxis("RightThumbstickMoveY", this, &AVertCharacter::RightThumbstickMoveY);
	PlayerInputComponent->BindAxis("MouseMove", this, &AVertCharacter::MouseMove);
}


void AVertCharacter::OnDashRechargeTimerFinished_Implementation()
{
	if(!Dash.RecieveChargeOnGroundOnly || IsGrounded())
		mRemainingDashes += Dash.RechargeTimer.PopAlarmBacklog();
	Dash.RechargeTimer.Reset();
}

void AVertCharacter::OnGrappleRechargeTimerFinished_Implementation()
{
	if(!Grapple.RecieveChargeOnGroundOnly || IsGrounded())
		mRemainingGrapples += Grapple.RechargeTimer.PopAlarmBacklog();
	Grapple.RechargeTimer.Reset();	
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

void AVertCharacter::GrappleShootMK()
{
	if (mDisableGrapple)
		return;

	if (mGrappleLauncher.IsValid())
	{
		mGrappleLauncher->FireGrapple(UVertUtilities::LimitAimTrajectory(Grapple.AimFreedom, mAxisPositions.GetPlayerMouseDirection()));
	}
}

void AVertCharacter::GrappleShootGamepad(const FVector2D& axis)
{
	if (mDisableGrapple)
		return;

	if (mGrappleLauncher.IsValid())
	{
		FVector2D axisFixedDirection = (axis * 100).GetSafeNormal();
		mGrappleLauncher->FireGrapple(UVertUtilities::LimitAimTrajectory2D(Grapple.AimFreedom, axisFixedDirection), true);
	}
}

void AVertCharacter::DashMK()
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
			Dash.DirectionOfTravel = mAxisPositions.GetPlayerMouseDirection();
		else if (Dash.AimMode == EDashAimMode::PlayerDirection)
		{
			Dash.DirectionOfTravel = FVector::ZeroVector;
			if (AVertPlayerController* controller = GetPlayerController())
			{
				if (controller->IsInputKeyDown(EKeys::W))
					Dash.DirectionOfTravel.Z = 1.0f;
				else if (controller->IsInputKeyDown(EKeys::S))
					Dash.DirectionOfTravel.Z = -1.0f;
				
				if (controller->IsInputKeyDown(EKeys::A))
					Dash.DirectionOfTravel.X = -1.0f;
				else if (controller->IsInputKeyDown(EKeys::D))
					Dash.DirectionOfTravel.X = 1.0f;
			}
		}			

		if (Dash.DirectionOfTravel.X == 0 && Dash.DirectionOfTravel.Z == 0)
		{
			Dash.DirectionOfTravel = (Controller) ? Controller->GetControlRotation().RotateVector(FVector(1.f, 0.f, 0.f)) : FVector(1.f, 0.f, 0.f);
		}

		Dash.DirectionOfTravel = UVertUtilities::LimitAimTrajectory(Dash.AimFreedom, Dash.DirectionOfTravel);

		Dash.IsDashing = true;
		if (Dash.DisableGravityWhenDashing)
			GetCharacterMovement()->GravityScale = 0.f;
		GetCharacterMovement()->GroundFriction = 0.f;
		mRemainingDashes--;
	}
}

void AVertCharacter::DashGamepad()
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
			Dash.DirectionOfTravel = mAxisPositions.GetPlayerRightThumbstickDirection();
		else if (Dash.AimMode == EDashAimMode::PlayerDirection)
			Dash.DirectionOfTravel = mAxisPositions.GetPlayerLeftThumbstickDirection();

		if (Dash.DirectionOfTravel.X == 0 && Dash.DirectionOfTravel.Y == 0)
		{
			Dash.DirectionOfTravel = (Controller) ? Controller->GetControlRotation().RotateVector(FVector(1.f, 0.f, 0.f)) : FVector(1.f, 0.f, 0.f);
		}

		Dash.DirectionOfTravel = UVertUtilities::LimitAimTrajectory(Dash.AimFreedom, Dash.DirectionOfTravel);

		Dash.IsDashing = true;
		if (Dash.DisableGravityWhenDashing)
			GetCharacterMovement()->GravityScale = 0.f;
		GetCharacterMovement()->GroundFriction = 0.f;
		mRemainingDashes--;
	}
}

void AVertCharacter::Interact()
{
	if (InteractionComponent)
	{
		IInteractive* interactive = InteractionComponent->AttemptInteract();
	}
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
	}

	if (ShowDebug.Dash.Enabled)
	{
		FVector dashDirection = (Dash.AimMode == EDashAimMode::PlayerDirection) ? mAxisPositions.GetPlayerLeftThumbstickDirection() : mAxisPositions.GetPlayerRightThumbstickDirection();
		dashDirection = UVertUtilities::LimitAimTrajectory(Dash.AimFreedom, dashDirection);

		DrawDebugDirectionalArrow(GetWorld(), GetActorLocation(), GetActorLocation() + (dashDirection * 500), 100.f, ShowDebug.Dash.MessageColour);

		GEngine->AddOnScreenDebugMessage(debugIndex++, 3.f, ShowDebug.Dash.MessageColour, FString::Printf(TEXT("[Character-Dash] Remaining Dashes: %i / %i"), mRemainingDashes, MaxDashes));
		GEngine->AddOnScreenDebugMessage(debugIndex++, 3.f, ShowDebug.Dash.MessageColour, FString::Printf(TEXT("[Character-Dash] Recharge at %f% (%s)"), Dash.RechargeTimer.GetProgressPercent(), Dash.RechargeTimer.IsRunning() ? TEXT("active") : TEXT("inactive")));
	}

	if (ShowDebug.Grapple.Enabled)
	{
		GEngine->AddOnScreenDebugMessage(debugIndex++, 3.f, ShowDebug.Grapple.MessageColour, FString::Printf(TEXT("[Character-Grapple] Grapple Launched | %i / %i uses remaining"), mRemainingGrapples, MaxGrapples));
		GEngine->AddOnScreenDebugMessage(debugIndex++, 3.f, ShowDebug.Grapple.MessageColour, FString::Printf(TEXT("[Character-Grapple] State: %s"), *UVertUtilities::GetEnumValueToString<EGrappleState>(TEXT("EGrappleState"), mGrappleLauncher->GetGrappleHook()->GetGrappleState())));
		GEngine->AddOnScreenDebugMessage(debugIndex++, 3.f, ShowDebug.Grapple.MessageColour, FString::Printf(TEXT("[Character-Grapple] Hook Velocity: %f, %f"), mGrappleLauncher->GetGrappleHook()->GetHookVelocity().X, mGrappleLauncher->GetGrappleHook()->GetHookVelocity().Z));
		GEngine->AddOnScreenDebugMessage(debugIndex++, 3.f, ShowDebug.Grapple.MessageColour, FString::Printf(TEXT("[Character-Grapple] Hook Active: %s"), (mGrappleLauncher->GetGrappleHook()->GetProjectileMovementComponentIsActive()) ? TEXT("true") : TEXT("false")));
		GEngine->AddOnScreenDebugMessage(debugIndex++, 3.f, ShowDebug.Grapple.MessageColour, FString::Printf(TEXT("[Character-Grapple] Recharge at %f% (%s)"), Grapple.RechargeTimer.GetProgressPercent(), Grapple.RechargeTimer.IsRunning() ? TEXT("active") : TEXT("inactive")));

		DrawDebugDirectionalArrow(GetWorld(), GetActorLocation(), GetActorLocation() + (mAxisPositions.GetPlayerRightThumbstickDirection() * 500), 50.f, ShowDebug.Grapple.MessageColour, false, -1.f, 1, 3.f);
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

void AVertCharacter::SortAbilityRechargeState()
{
	if (mRemainingDashes < MaxDashes && Dash.RechargeTimer.GetAlarmBacklog() < (MaxDashes - mRemainingDashes))
	{
		(CanRecharge(Dash.RechargeMode))
			? Dash.RechargeTimer.Start()
			: Dash.RechargeTimer.Stop();
	}

	if (mRemainingGrapples < MaxGrapples && Grapple.RechargeTimer.GetAlarmBacklog() < (MaxGrapples - mRemainingGrapples))
	{
		(CanRecharge(Grapple.RechargeMode))
			? Grapple.RechargeTimer.Start()
			: Grapple.RechargeTimer.Stop();
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


bool AVertCharacter::CanRecharge(ERechargeRule rule)
{
	AGrappleHook* hook = GetGrappleHook();
	AActor* hookedActor = hook ? hook->GetHookedActor() : nullptr;
	AGrapplePoint* grapplePoint = Cast<AGrapplePoint>(hookedActor);
	if (hookedActor)
	{
		UE_LOG(LogVertCharacter, Warning, TEXT("Hooked actor found with name [%s]"), *hookedActor->GetName());
	}

	return IsGrounded() ||
		rule == ERechargeRule::OnRechargeTimer ||
		(rule == ERechargeRule::OnContactGroundOrLatchedAnywhere && hook && hook->GetGrappleState() == EGrappleState::Latched) ||
		(rule == ERechargeRule::OnContactGroundOrLatchedToHook && hook && hook->GetGrappleState() == EGrappleState::Latched && grapplePoint);
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
			GrappleShootGamepad(axis);
			return true;
		}
	}

	return false;
}

void AVertCharacter::RightThumbstickMoveX(float value)
{
	mAxisPositions.RightX = value;
	if (FMath::Abs(value) > 0.1f)
		CheckShootGrappleGamepad();
}

void AVertCharacter::RightThumbstickMoveY(float value)
{
	mAxisPositions.RightY = value;
	if(FMath::Abs(value) > 0.1f)
		CheckShootGrappleGamepad();
}

void AVertCharacter::LeftThumbstickMoveX(float value)
{
	mAxisPositions.LeftX = value;
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