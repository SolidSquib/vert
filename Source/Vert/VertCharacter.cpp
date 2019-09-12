// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "VertCharacter.h"
#include "Engine/VertGlobals.h"
#include "ContainerAllocationPolicies.h"
#include "UserInterface/AimArrowWidgetComponent.h"

DEFINE_LOG_CATEGORY(LogVertCharacter);
//////////////////////////////////////////////////////////////////////////
// AVertCharacter

AVertCharacter::AVertCharacter(const FObjectInitializer & ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UVertCharacterMovementComponent>(ACharacter::CharacterMovementComponentName)),
	InteractionComponent(CreateDefaultSubobject<UCharacterInteractionComponent>(TEXT("InteractionComponent"))),
	GrapplingComponent(CreateDefaultSubobject<UGrapplingComponent>(TEXT("GrapplingComponent"))),
	DashingComponent(CreateDefaultSubobject<UDashingComponent>(TEXT("DashingComponent"))),
	ClimbingComponent(CreateDefaultSubobject<ULedgeGrabbingComponent>(TEXT("ClimbingComponent"))),
	AimArrowComponent(CreateDefaultSubobject<UAimArrowWidgetComponent>(TEXT("GrappleAimArrowUI"))),
	AimArrowAnchor(CreateDefaultSubobject<USceneComponent>(TEXT("GrappleAimArrowAnchor"))),
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
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_StreamingBounds, ECR_Overlap);

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
	SideViewCameraComponent->ProjectionMode = ECameraProjectionMode::Perspective;
	SideViewCameraComponent->OrthoWidth = 2048.0f;
	SideViewCameraComponent->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);

	// Prevent all automatic rotation behavior on the camera, character, and camera component
	CameraBoom->bAbsoluteRotation = true;
	SideViewCameraComponent->bUsePawnControlRotation = false;
	SideViewCameraComponent->bAutoActivate = true;
	
	GrapplingComponent->SetupAttachment(GetMesh());

	AimArrowAnchor->SetupAttachment(GrapplingComponent);
	AimArrowComponent->SetupAttachment(AimArrowAnchor);

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = false;
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

	ClimbingComponent->SetupAttachment(GetRootComponent());

	// Enable replication on the Sprite component so animations show up when networked
	GetMesh()->SetIsReplicated(true);
	GetMesh()->SetRenderCustomDepth(true);
	bReplicates = true;
}

//************************************
// Method:    PostInitializeComponents
// FullName:  AVertCharacter::PostInitializeComponents
// Access:    virtual public 
// Returns:   void
// Qualifier:
//************************************
void AVertCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	// create material instance for setting team colors (3rd person view)
	for (int32 iMat = 0; iMat < GetMesh()->GetNumMaterials(); iMat++)
	{
		MeshMIDs.Add(GetMesh()->CreateAndSetMaterialInstanceDynamic(iMat));
	}
}

//************************************
// Method:    BeginPlay
// FullName:  AVertCharacter::BeginPlay
// Access:    virtual public 
// Returns:   void
// Qualifier:
//************************************
void AVertCharacter::BeginPlay()
{
	Super::BeginPlay();
	
	GetCapsuleComponent()->OnComponentHit.AddDynamic(this, &AVertCharacter::OnCharacterHit);
	
	mOnWeaponStateChangedDelegate.BindUFunction(this, TEXT("Character_OnWeaponStateChangeExecuted"));
	mOnWeaponFiredDelegate.BindUFunction(this, TEXT("Character_OnAttackFired"));

	mStartMovementSpeed = GetCharacterMovement()->MaxWalkSpeed;

	if (InteractionComponent)
	{
		InteractionComponent->Delegate_OnDropInteractive.AddDynamic(this, &AVertCharacter::OnDropInteractiveInternal);
		InteractionComponent->Delegate_OnPickupInteractive.AddDynamic(this, &AVertCharacter::OnPickupInteractiveInternal);

		if (InteractionComponent->GetDefaultWeapon())
		{
			InteractionComponent->GetDefaultWeapon()->Delegate_OnWeaponAttackFire.Add(mOnWeaponFiredDelegate);
			InteractionComponent->GetDefaultWeapon()->Delegate_OnWeaponStateChanged.Add(mOnWeaponStateChangedDelegate);
		}
	}

	if (DashingComponent)
	{
		DashingComponent->OnDashEnd.AddDynamic(this, &AVertCharacter::Character_OnDashEnded);
	}

	if (GrapplingComponent)
	{
		GrapplingComponent->OnGrappleToLedgeTransition.AddDynamic(this, &AVertCharacter::AttemptToGrabGrappledLedge);
		GrapplingComponent->OnHookReleased.AddDynamic(this, &AVertCharacter::GrappleDetached);
		GrapplingComponent->OnHooked.AddDynamic(this, &AVertCharacter::GrappleHooked);
		GrapplingComponent->OnReturned.AddDynamic(this, &AVertCharacter::GrappleReturned);
		GrapplingComponent->OnHookBreak.AddDynamic(this, &AVertCharacter::Character_OnGrappleBeginReturn);
	}

	if (ClimbingComponent)
	{
		ClimbingComponent->WantsToAttack.AddDynamic(this, &AVertCharacter::PerformLedgeAttack);
		ClimbingComponent->OnLedgeTransition.AddDynamic(this, &AVertCharacter::Character_OnLedgeTransition);
		ClimbingComponent->HoldingLedge.AddDynamic(this, &AVertCharacter::OnLedgeGrabbed);
	}

	if (AimArrowComponent->GetUserWidgetObject())
	{
		AimArrowComponent->GetUserWidgetObject()->SetVisibility(ESlateVisibility::Hidden);
	}
}

//************************************
// Method:    EndPlay
// FullName:  AVertCharacter::EndPlay
// Access:    virtual protected 
// Returns:   void
// Qualifier:
// Parameter: const EEndPlayReason::Type EndPlayReason
//************************************
void AVertCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (EndPlayReason == EEndPlayReason::Destroyed)
	{
		if (ABaseWeapon* weapon = GetHeldWeapon())
		{
			if (weapon->GetOwningPool() != nullptr)
			{
				weapon->ReturnToPool();
			}
			else
			{
				weapon->Destroy();
			}
		}
	}
}

//************************************
// Method:    PossessedBy
// FullName:  AVertCharacter::PossessedBy
// Access:    virtual public 
// Returns:   void
// Qualifier:
// Parameter: AController * NewController
//************************************
void AVertCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	VertController = Cast<AVertPlayerController>(NewController);
	UpdateTeamColoursAllMIDs();
}

//************************************
// Method:    OnRep_PlayerState
// FullName:  AVertCharacter::OnRep_PlayerState
// Access:    virtual public 
// Returns:   void
// Qualifier:
//************************************
void AVertCharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

	// [client] as soon as PlayerState is assigned, set team colors of this pawn for local player
	if (PlayerState != NULL)
	{
		UpdateTeamColoursAllMIDs();
	}
}

//************************************
// Method:    Dislodge
// FullName:  AVertCharacter::Dislodge
// Access:    public 
// Returns:   void
// Qualifier:
//************************************
void AVertCharacter::Dislodge(bool dropItem /*= false*/)
{
	GrapplingComponent->Reset();
	ClimbingComponent->TransitionLedge(ELedgeTransition::DropFromGrabbedLedge);
	DashingComponent->StopDashing();

	if (dropItem)
		InteractionComponent->DropInteractive();
}

//************************************
// Method:    Die
// FullName:  AVertCharacter::Die
// Access:    virtual protected 
// Returns:   bool
// Qualifier:
//************************************
bool AVertCharacter::Die()
{
	if (!CanDie()) // Already dead, don't attempt to kill again!
		return false;

	if (AVertGameMode* gm = GetWorld()->GetAuthGameMode<AVertGameMode>())
	{
		AVertPlayerController* pc = Cast<AVertPlayerController>(GetController());
		check(pc);

		if (DeathFX)
		{
			if (AVertPlayerCameraActor* camera = gm->GetActivePlayerCamera())
			{
				FVector spawnLocation = FVector::ZeroVector;
				if (AVertCameraManager* cameraMan = Cast<AVertCameraManager>(pc->PlayerCameraManager))
				{
					FVector topLeft, bottomRight, topRight, bottomLeft;
					FVector playerLocation = GetActorLocation();
					FVector camLocation = camera->GetActorLocation();
					cameraMan->GetCurrentPlayerBounds(topLeft, bottomRight, bottomLeft, topRight);
					
					float minX, maxX;
					cameraMan->GetBoundsXMinMax(minX, maxX);

					spawnLocation.X = FMath::Clamp(GetActorLocation().X, minX, maxX);
					spawnLocation.Z = FMath::Clamp(GetActorLocation().Z, bottomRight.Z, topLeft.Z);
					spawnLocation.Y = GetActorLocation().Y;
				}

				if (PlayerDeathSound)
				{
					UAkGameplayStatics::PostEvent(PlayerDeathSound, this);
				}
				
				UParticleSystemComponent* emitter = UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), DeathFX, FTransform(GetActorRotation(), spawnLocation));
				if (pc && emitter)
				{
					AVertPlayerState* state = pc->GetVertPlayerState();
					emitter->SetColorParameter("PlayerDeath_Col", state->GetPlayerColours().PrimaryColour);
				}
			}
		}

		if (DeathFeedback && UsingGamepad())
		{
			if (pc)
			{
				static const FName scDeathFeedbackName(TEXT("PlayerDeathFeedback"));
				pc->ClientPlayForceFeedback(DeathFeedback, false, scDeathFeedbackName);
			}
		}

		if (DeathCameraShakeEvent)
		{
			APlayerController* pc = Cast<APlayerController>(GetController());
			if (pc)
			{
				pc->ClientPlayCameraShake(DeathCameraShakeEvent, DeathCameraShakeScale);
			}
		}
	}

	return Super::Die();
}

//************************************
// Method:    Landed
// FullName:  AVertCharacter::Landed
// Access:    virtual public 
// Returns:   void
// Qualifier:
// Parameter: const FHitResult & Hit
//************************************
void AVertCharacter::Landed(const FHitResult& Hit)
{
	GrapplingComponent->OnLanded();
	DashingComponent->OnLanded();

	if (JumpLandSound)
	{
		UAkGameplayStatics::PostEvent(JumpLandSound, this);
	}

	Super::Landed(Hit);
}

//************************************
// Method:    StopAttacking
// FullName:  AVertCharacter::StopAttacking
// Access:    public 
// Returns:   void
// Qualifier:
//************************************
void AVertCharacter::StopAttacking()
{
	InteractionComponent->StopAttacking();
	Character_OnStopAttackExecuted();
}

//************************************
// Method:    CanFire
// FullName:  AVertCharacter::CanFire
// Access:    public 
// Returns:   bool
// Qualifier: const
//************************************
bool AVertCharacter::CanFire() const
{
	return !IsInHitstun() && !ClimbingComponent->IsTransitioning() && VertController && VertController->IsMovementInputEnabled();
}

//************************************
// Method:    CanReload
// FullName:  AVertCharacter::CanReload
// Access:    public 
// Returns:   bool
// Qualifier: const
//************************************
bool AVertCharacter::CanReload() const
{
	return !IsInHitstun() && !ClimbingComponent->IsTransitioning() && VertController && VertController->IsMovementInputEnabled();
}

//************************************
// Method:    CanGrapple
// FullName:  AVertCharacter::CanGrapple
// Access:    public 
// Returns:   bool
// Qualifier: const
//************************************
bool AVertCharacter::CanGrapple() const
{
	return !IsInHitstun() && !ClimbingComponent->IsTransitioning() && (GrapplingComponent->GetGrappleState() == EGrappleState::HookSheathed || GrapplingComponent->GetHookedPrimitive()) && VertController && VertController->IsMovementInputEnabled();
}

//************************************
// Method:    CanDash
// FullName:  AVertCharacter::CanDash
// Access:    public 
// Returns:   bool
// Qualifier: const
//************************************
bool AVertCharacter::CanDash() const
{
	return !IsInHitstun() && !ClimbingComponent->IsTransitioning() && VertController && VertController->IsMovementInputEnabled();
}

//************************************
// Method:    CanMove
// FullName:  AVertCharacter::CanMove
// Access:    public 
// Returns:   bool
// Qualifier: const
//************************************
bool AVertCharacter::CanMove() const
{
	return !IsInHitstun() && !ClimbingComponent->IsTransitioning() && VertController && VertController->IsMovementInputEnabled();
}

//************************************
// Method:    CanAttack
// FullName:  AVertCharacter::CanAttack
// Access:    public 
// Returns:   bool
// Qualifier: const
//************************************
bool AVertCharacter::CanAttack() const
{
	return !IsInHitstun() && !ClimbingComponent->IsTransitioning() && !DashingComponent->IsCurrentlyDashing() && VertController && VertController->IsMovementInputEnabled();
}

//************************************
// Method:    CanInteract
// FullName:  AVertCharacter::CanInteract
// Access:    public 
// Returns:   bool
// Qualifier: const
//************************************
bool AVertCharacter::CanInteract() const
{
	return !IsInHitstun() && !ClimbingComponent->IsTransitioning() && VertController && VertController->IsMovementInputEnabled();
}

//************************************
// Method:    CanWallJump
// FullName:  AVertCharacter::CanWallJump
// Access:    public 
// Returns:   bool
// Qualifier: const
//************************************
bool AVertCharacter::CanWallJump() const
{
	if (!IsGrounded())
	{
		return false;
	}

	return false;
}

//************************************
// Method:    CanJumpInternal_Implementation
// FullName:  AVertCharacter::CanJumpInternal_Implementation
// Access:    virtual protected 
// Returns:   bool
// Qualifier: const
//************************************
bool AVertCharacter::CanJumpInternal_Implementation() const
{
	if (ClimbingComponent->IsLaunchingFromLedge())
	{
		return true;
	}

	// Nerf the pole-vaulting by not allowing a jump from grapple above a certain grapple threshold.
	if (GrapplingComponent->IsGrappleDeployed())
	{
		constexpr float cThreshold = -0.7f;
		float DotProduct = FVector::DotProduct(GrapplingComponent->GetLineDirection(), FVector::UpVector);

		if (DotProduct > cThreshold)
			return true;
		else
			GrapplingComponent->Reset();
	}

	// Manipulate JumpMaxCount to allow for extra jumps in given situations
	if ((Super::CanJumpInternal_Implementation() || CanWallJump()) && !IsInHitstun() && !ClimbingComponent->IsTransitioning())
	{
		return true;
	}

	return false;
}

//************************************
// Method:    Jump
// FullName:  AVertCharacter::Jump
// Access:    virtual protected 
// Returns:   void
// Qualifier:
//************************************
void AVertCharacter::Jump()
{
	if (GrapplingComponent->IsGrappleDeployed() && !IsGrounded())
	{
		mJumpedFromGrapple = true;
	}

	Super::Jump();
}

//************************************
// Method:    OnJumped_Implementation
// FullName:  AVertCharacter::OnJumped_Implementation
// Access:    virtual protected 
// Returns:   void
// Qualifier:
//************************************
void AVertCharacter::OnJumped_Implementation()
{
	if (GrapplingComponent->IsGrappleDeployed())
	{
		GrapplingComponent->Reset();
	}

	if(mJumpedFromGrapple)
	{
		JumpCurrentCount = 1;
		mJumpedFromGrapple = false;
	}

	if (JumpCurrentCount <= 1)
	{
		if (JumpSound)
		{
			UAkGameplayStatics::PostEvent(JumpSound, this);
		}

		Character_OnJumpExecuted();
	}
	else
	{
		if (DoubleJumpSound)
		{
			UAkGameplayStatics::PostEvent(DoubleJumpSound, this);
		}

		Character_OnAirJumpExecuted();
	}
}

//////////////////////////////////////////////////////////////////////////
// Input

//************************************
// Method:    SetupPlayerInputComponent
// FullName:  AVertCharacter::SetupPlayerInputComponent
// Access:    virtual protected 
// Returns:   void
// Qualifier:
// Parameter: class UInputComponent * PlayerInputComponent
//************************************
void AVertCharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	// Note: the 'Jump' action and the 'MoveRight' axis are bound to actual keys/buttons/sticks in DefaultInput.ini (editable from Project Settings..Input)
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &AVertCharacter::ActionJump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);
	PlayerInputComponent->BindAction("GrappleShoot", IE_Pressed, this, &AVertCharacter::ActionGrappleShoot);
	PlayerInputComponent->BindAction("Dash", IE_Pressed, this, &AVertCharacter::ActionDash);
	PlayerInputComponent->BindAction("Interact", IE_Pressed, this, &AVertCharacter::ActionInteract);
	PlayerInputComponent->BindAction("Attack", IE_Pressed, this, &AVertCharacter::ActionAttack);
	PlayerInputComponent->BindAction("Attack", IE_Released, this, &AVertCharacter::StopAttacking);
	PlayerInputComponent->BindAction("DropDown", IE_Pressed, this, &AVertCharacter::ActionDropDown);
	PlayerInputComponent->BindAction("GrappleShootGamepadAlt", IE_Pressed, this, &AVertCharacter::ActionGrappleShootAltDown);
	PlayerInputComponent->BindAction("GrappleShootGamepadAlt", IE_Released, this, &AVertCharacter::ActionGrappleShootAltUp);

	PlayerInputComponent->BindAxis("MoveRight", this, &AVertCharacter::ActionMoveRight);
	PlayerInputComponent->BindAxis("LeftThumbstickMoveY", this, &AVertCharacter::LeftThumbstickMoveY);
	PlayerInputComponent->BindAxis("RightThumbstickMoveX", this, &AVertCharacter::RightThumbstickMoveX);
	PlayerInputComponent->BindAxis("RightThumbstickMoveY", this, &AVertCharacter::RightThumbstickMoveY);
	PlayerInputComponent->BindAxis("MouseMove", this, &AVertCharacter::MouseMove);
	
}

void AVertCharacter::ApplyDamageHitstun(float hitstunTime)
{
	Super::ApplyDamageHitstun(hitstunTime);
	GrapplingComponent->Reset();
	ClimbingComponent->TransitionLedge(ELedgeTransition::DropFromGrabbedLedge);
}

//************************************
// Method:    UpdateCharacter
// FullName:  AVertCharacter::UpdateCharacter
// Access:    protected 
// Returns:   void
// Qualifier:
//************************************
void AVertCharacter::UpdateCharacter()
{
	// Now setup the rotation of the controller based on the direction we are travelling
	const FVector PlayerDirection = GetAxisPostisions().GetPlayerLeftThumbstick();

	// Set the rotation so that the character faces his direction of travel.
	if (Controller != nullptr)
	{
		if (PlayerDirection.X < 0.0f)
		{
			Controller->SetControlRotation(FRotator(0.0, 180.0f, 0.0f));
		}
		else if (PlayerDirection.X > 0.0f)
		{
			Controller->SetControlRotation(FRotator(0.0f, 0.0f, 0.0f));
		}
	}
}

//************************************
// Method:    ActionMoveRight
// FullName:  AVertCharacter::ActionMoveRight
// Access:    protected 
// Returns:   void
// Qualifier:
// Parameter: float Value
//************************************
void AVertCharacter::ActionMoveRight(float Value)
{
	if (CanMove())
	{
		mAxisPositions.LeftX = Value;

		if (ClimbingComponent->IsClimbingLedge())
		{
			static constexpr float direction_threshold = 0.25;

			if (FMath::Abs(Value) > KINDA_SMALL_NUMBER)
			{
				FVector direction = ClimbingComponent->GetLedgeDirection(EAimFreedom::Horizontal);
				FVector moveDirection(Value, 0, 0);
				float cos = FVector::DotProduct(moveDirection, direction);
				if (cos > direction_threshold)
				{
					ClimbingComponent->TransitionLedge(ELedgeTransition::ClimbUpLedge);
				}
				else if (cos < -direction_threshold)
				{
					ClimbingComponent->TransitionLedge(ELedgeTransition::JumpAwayFromGrabbedLedge);
				}
			}
		}
		else
		{
			AddMovementInput(FVector(1.f, 0.f, 0.f), Value);
			UpdateCharacter();
		}
	}
}

//************************************
// Method:    ActionJump
// FullName:  AVertCharacter::ActionJump
// Access:    protected 
// Returns:   void
// Qualifier:
//************************************
void AVertCharacter::ActionJump()
{
	if (VertController && VertController->IsMovementInputEnabled())
	{
		if (ClimbingComponent->IsClimbingLedge())
		{
			if (CanMove())
				ClimbingComponent->TransitionLedge(ELedgeTransition::LaunchFromGrabbedLedge);
		}
		else
		{
			Jump();
		}
	}
}

//************************************
// Method:    ActionDropDown
// FullName:  AVertCharacter::ActionDropDown
// Access:    protected 
// Returns:   void
// Qualifier:
//************************************
void AVertCharacter::ActionDropDown()
{
	if (CanMove())
	{
		if (ClimbingComponent->IsClimbingLedge())
		{
			ClimbingComponent->TransitionLedge(ELedgeTransition::DropFromGrabbedLedge);
		}
	}
}

//************************************
// Method:    ActionGrappleShoot
// FullName:  AVertCharacter::ActionGrappleShoot
// Access:    protected 
// Returns:   void
// Qualifier:
//************************************
void AVertCharacter::ActionGrappleShoot()
{
	if (CanGrapple())
	{
 		if (UsingGamepad())
 		{
			if (!mGamepadWantsToGrapple)
			{
				mGamepadWantsToGrapple = true;
			}
 		}
 		else
		{
			ExecuteActionGrappleShoot();
		}
	}
}

//************************************
// Method:    ActionGrappleShootAltDown
// FullName:  AVertCharacter::ActionGrappleShootAltDown
// Access:    protected 
// Returns:   void
// Qualifier:
//************************************
void AVertCharacter::ActionGrappleShootAltDown()
{
	if (!GrapplingComponent->GetHookedPrimitive())
	{
		if (AimArrowComponent && AimArrowComponent->GetUserWidgetObject())
		{
			AimArrowComponent->GetUserWidgetObject()->SetVisibility(ESlateVisibility::Visible);
		}
	}
}

//************************************
// Method:    ActionGrappleShootAltUp
// FullName:  AVertCharacter::ActionGrappleShootAltUp
// Access:    protected 
// Returns:   void
// Qualifier:
//************************************
void AVertCharacter::ActionGrappleShootAltUp()
{
	if (CanGrapple())
	{
		if (!GrapplingComponent->GetHookedPrimitive())
		{
			FVector aimDirection = GetAxisPostisions().GetPlayerLeftThumbstickDirection();
			FVector ActualAimDirection = aimDirection == FVector::ZeroVector ?
				GetController()->GetControlRotation().Vector() :
				GetAxisPostisions().GetPlayerLeftThumbstickDirection();

			if (ActualAimDirection != FVector::ZeroVector && GrapplingComponent->ExecuteGrapple(ActualAimDirection))
			{
				Character_OnGrappleShootExecuted(ActualAimDirection);
			}
		}
		else
		{
			if (GrapplingComponent->StartPulling())
			{
				if (IsClimbing())
				{
					ClimbingComponent->TransitionLedge(ELedgeTransition::DropFromGrabbedLedge);
				}

				Character_OnGrapplePullExecuted(GrapplingComponent->GetLineDirection());
			}
		}
	}

	if (AimArrowComponent && AimArrowComponent->GetUserWidgetObject())
	{
		AimArrowComponent->GetUserWidgetObject()->SetVisibility(ESlateVisibility::Hidden);
	}
}

//************************************
// Method:    ExecuteActionGrappleShoot
// FullName:  AVertCharacter::ExecuteActionGrappleShoot
// Access:    protected 
// Returns:   void
// Qualifier:
//************************************
void AVertCharacter::ExecuteActionGrappleShoot()
{
	if (CanGrapple())
	{
		if (!GrapplingComponent->GetHookedPrimitive())
		{
			FVector aimDirection = UsingGamepad()
				? mGamepadSwipeDirection /*GetAxisPostisions().GetPlayerRightThumbstickDirection()*/
				: GetAxisPostisions().GetPlayerMouseDirection();

			if (aimDirection != FVector::ZeroVector && GrapplingComponent->ExecuteGrapple(aimDirection))
			{
				Character_OnGrappleShootExecuted(aimDirection);
			}
		}
		else
		{
			if (GrapplingComponent->StartPulling())
			{
				if (IsClimbing())
				{
					ClimbingComponent->TransitionLedge(ELedgeTransition::DropFromGrabbedLedge);
				}

				Character_OnGrapplePullExecuted(GrapplingComponent->GetLineDirection());
			}
		}
	}
}

//************************************
// Method:    ActionDash
// FullName:  AVertCharacter::ActionDash
// Access:    protected 
// Returns:   void
// Qualifier:
//************************************
void AVertCharacter::ActionDash()
{
	if (CanDash())
	{
		FVector direction;
		if (DashingComponent->ExecuteDash(direction))
		{
			InteractionComponent->StopAttacking(true);
			Character_OnDashExecuted(direction, false);
		}
		
		UpdateCharacter();
	}	
}

//************************************
// Method:    ActionInteract
// FullName:  AVertCharacter::ActionInteract
// Access:    protected 
// Returns:   void
// Qualifier:
//************************************
void AVertCharacter::ActionInteract()
{
	if (CanInteract())
	{
		if (AInteractive* interactive = InteractionComponent->AttemptInteract())
		{
			Character_OnInteractExecuted(interactive);
		}
	}
}

//************************************
// Method:    ActionAttack
// FullName:  AVertCharacter::ActionAttack
// Access:    protected 
// Returns:   void
// Qualifier:
//************************************
void AVertCharacter::ActionAttack()
{
	if (CanAttack())
	{
		if (!ClimbingComponent->IsClimbingLedge() && InteractionComponent->AttemptAttack())
		{
			Character_OnStartAttackExecuted(GetCurrentWeapon());
		}
	}
}

//************************************
// Method:    UpdateTeamColoursAllMIDs
// FullName:  AVertCharacter::UpdateTeamColoursAllMIDs
// Access:    public 
// Returns:   void
// Qualifier:
//************************************
void AVertCharacter::UpdateTeamColoursAllMIDs()
{
	for (int32 i = 0; i < MeshMIDs.Num(); ++i)
	{
		UpdateTeamColours(MeshMIDs[i]);
	}

	AVertPlayerState* playerState = Cast<AVertPlayerState>(PlayerState);
	if (playerState)
	{
		switch (playerState->PlayerIndex)
		{
		case 0:
			GetMesh()->SetCustomDepthStencilValue(252);
			break;
		case 1:
			GetMesh()->SetCustomDepthStencilValue(253);
			break;
		case 2:
			GetMesh()->SetCustomDepthStencilValue(254);
			break;
		case 3:
			GetMesh()->SetCustomDepthStencilValue(255);
			break;
		}
	}	
}

//************************************
// Method:    UpdateTeamColours
// FullName:  AVertCharacter::UpdateTeamColours
// Access:    protected 
// Returns:   void
// Qualifier:
// Parameter: UMaterialInstanceDynamic * useMIDs
//************************************
void AVertCharacter::UpdateTeamColours(UMaterialInstanceDynamic* useMID)
{
	AVertPlayerState* playerState = Cast<AVertPlayerState>(PlayerState);

	if (playerState != NULL)
	{
		if (useMID)
		{
			useMID->SetVectorParameterValue(TEXT("Primary Colour"), playerState->GetPlayerColours().PrimaryColour);
			useMID->SetVectorParameterValue(TEXT("Secondary Colour"), playerState->GetPlayerColours().SecondaryColour);
			useMID->SetVectorParameterValue(TEXT("Emissive Colour"), playerState->GetPlayerColours().EmissiveColour);
		}

		if (GrapplingComponent)
		{
			GrapplingComponent->SetGrappleColour(playerState->GetPlayerColours().PrimaryColour);
		}

		if (AimArrowComponent)
		{
			AimArrowComponent->SetColour(playerState->GetPlayerColours().PrimaryColour);
		}
	}
}

//************************************
// Method:    OnPickupInteractive
// FullName:  AVertCharacter::OnPickupInteractive
// Access:    protected 
// Returns:   void
// Qualifier:
// Parameter: AInteractive * interactive
// Parameter: bool wasCaught
//************************************
void AVertCharacter::OnPickupInteractiveInternal(AInteractive* interactive, bool wasCaught)
{
	if (ABaseWeapon* weapon = Cast<ABaseWeapon>(interactive))
	{
		weapon->Delegate_OnWeaponAttackFire.Add(mOnWeaponFiredDelegate);
		weapon->Delegate_OnWeaponStateChanged.Add(mOnWeaponStateChangedDelegate);

		mCurrentWeapon = weapon;
		if (InteractionComponent->GetDefaultWeapon())
		{
			InteractionComponent->GetDefaultWeapon()->Delegate_OnWeaponAttackFire.Remove(mOnWeaponFiredDelegate);
			InteractionComponent->GetDefaultWeapon()->Delegate_OnWeaponStateChanged.Remove(mOnWeaponStateChangedDelegate);
		}
	}

	Character_OnPickupNewInteractive(interactive, wasCaught);
}

//************************************
// Method:    OnDropInteractive
// FullName:  AVertCharacter::OnDropInteractive
// Access:    protected 
// Returns:   void
// Qualifier:
// Parameter: AInteractive * interactive
// Parameter: bool wasThrown
//************************************
void AVertCharacter::OnDropInteractiveInternal(AInteractive* interactive, bool wasThrown)
{
	if (ABaseWeapon* weapon = Cast<ABaseWeapon>(interactive))
	{
		weapon->Delegate_OnWeaponAttackFire.Remove(mOnWeaponFiredDelegate);
		weapon->Delegate_OnWeaponStateChanged.Remove(mOnWeaponStateChangedDelegate);

		mCurrentWeapon = InteractionComponent->GetDefaultWeapon();
		if (mCurrentWeapon.IsValid())
		{
			mCurrentWeapon->Delegate_OnWeaponAttackFire.Add(mOnWeaponFiredDelegate);
			mCurrentWeapon->Delegate_OnWeaponStateChanged.Add(mOnWeaponStateChangedDelegate);
		}		
	}

	Character_OnDropCurrentInteractive(interactive, wasThrown);
}

//************************************
// Method:    AttemptToGrabGrappledLedge
// FullName:  AVertCharacter::AttemptToGrabGrappledLedge
// Access:    protected 
// Returns:   void
// Qualifier:
// Parameter: const FHitResult & forwardHit
// Parameter: const FHitResult & downwardHit
//************************************
void AVertCharacter::AttemptToGrabGrappledLedge(const FHitResult& forwardHit, const FHitResult& downwardHit)
{
	if (ClimbingComponent && !ClimbingComponent->IsClimbingLedge() && !ClimbingComponent->IsTransitioning())
	{
		static const FName sLedgeGrabTrace(TEXT("LedgeGrabTrace"));

		FCollisionQueryParams params(sLedgeGrabTrace, false);
		params.bReturnPhysicalMaterial = true;
		params.bTraceAsyncScene = false;
		params.bFindInitialOverlaps = false;
		params.AddIgnoredActor(GetOwner());

		FHitResult correctedForwardHit;
		FVector actorXPlane(GetActorLocation().X, 0, 0);
		FVector ledgeXPlane(downwardHit.ImpactPoint.X, 0, 0);
		FVector forward = (ledgeXPlane - actorXPlane).GetSafeNormal();
		FVector start = GetActorLocation();
		FVector end = start + (forward * FVector(ClimbingComponent->GetForwardRange(), ClimbingComponent->GetForwardRange(), 0.f));

		UWorld* world = GetWorld();
		const bool foundHit = world->SweepSingleByChannel(correctedForwardHit, start, end, FQuat::Identity, ClimbingComponent->GetTraceChannel(), FCollisionShape::MakeSphere(ClimbingComponent->GetTraceRadius()), params);

		if (foundHit)
		{
			ClimbingComponent->GrabLedge(correctedForwardHit, downwardHit);
		}		
	}
}

//************************************
// Method:    GrappleDetached
// FullName:  AVertCharacter::GrappleDetached
// Access:    protected 
// Returns:   void
// Qualifier:
//************************************
void AVertCharacter::GrappleDetached()
{
	
}

//************************************
// Method:    GrappleHooked
// FullName:  AVertCharacter::GrappleHooked
// Access:    protected 
// Returns:   void
// Qualifier:
//************************************
void AVertCharacter::GrappleHooked()
{
	
}

//************************************
// Method:    GrappleReturned
// FullName:  AVertCharacter::GrappleReturned
// Access:    protected 
// Returns:   void
// Qualifier:
//************************************
void AVertCharacter::GrappleReturned()
{
	
}

//************************************
// Method:    PerformLedgeAttack
// FullName:  AVertCharacter::PerformLedgeAttack
// Access:    protected 
// Returns:   void
// Qualifier:
//************************************
void AVertCharacter::PerformLedgeAttack()
{
	if (InteractionComponent->AttemptDashAttack())
	{
		Character_OnStartDashAttackExecuted(GetCurrentWeapon());
	}
	else if (InteractionComponent->AttemptAttack())
	{
		Character_OnStartAttackExecuted(GetCurrentWeapon());
		GetWorldTimerManager().SetTimer(mTimerHandle_AutoAttack, [this](void) {
			StopAttacking();
		}, GetCurrentWeapon()->GetAutoAttackTime(), false);
	}
}

//************************************
// Method:    OnLedgeGrabbed
// FullName:  AVertCharacter::OnLedgeGrabbed
// Access:    protected 
// Returns:   void
// Qualifier:
// Parameter: const FHitResult & forwardHit
// Parameter: const FHitResult & downwardHit
//************************************
void AVertCharacter::OnLedgeGrabbed(const FHitResult& forwardHit, const FHitResult& downwardHit)
{
	//GrapplingComponent->Reset();
	if(GetCurrentWeapon()) 
		GetCurrentWeapon()->Reset();
	Character_OnLedgeGrabbed(forwardHit, downwardHit);
}

//************************************
// Method:    GetCurrentWeapon
// FullName:  AVertCharacter::GetCurrentWeapon
// Access:    public 
// Returns:   ABaseWeapon*
// Qualifier: const
//************************************
ABaseWeapon* AVertCharacter::GetCurrentWeapon() const 
{
	if (InteractionComponent && InteractionComponent->GetHeldWeapon())
	{
		return InteractionComponent->GetHeldWeapon();
	}
	
	return (InteractionComponent && InteractionComponent->GetDefaultWeapon()) ? InteractionComponent->GetDefaultWeapon() : nullptr;
}

//************************************
// Method:    GetDefaultWeapon
// FullName:  AVertCharacter::GetDefaultWeapon
// Access:    public 
// Returns:   ABaseWeapon*
// Qualifier: const
//************************************
ABaseWeapon* AVertCharacter::GetDefaultWeapon() const
{
	if (InteractionComponent && InteractionComponent->GetDefaultWeapon())
	{
		return InteractionComponent->GetDefaultWeapon();
	}

	return nullptr;
}

//************************************
// Method:    GetHeldWeapon
// FullName:  AVertCharacter::GetHeldWeapon
// Access:    public 
// Returns:   ABaseWeapon*
// Qualifier: const
//************************************
ABaseWeapon* AVertCharacter::GetHeldWeapon() const
{
	if (InteractionComponent && InteractionComponent->GetHeldWeapon())
	{
		return InteractionComponent->GetHeldWeapon();
	}

	return nullptr;
}

//************************************
// Method:    OnCharacterHit
// FullName:  AVertCharacter::OnCharacterHit
// Access:    protected 
// Returns:   void
// Qualifier:
// Parameter: UPrimitiveComponent * hitComponent
// Parameter: AActor * otherActor
// Parameter: UPrimitiveComponent * otherComp
// Parameter: FVector normalImpulse
// Parameter: const FHitResult & hit
//************************************
void AVertCharacter::OnCharacterHit(UPrimitiveComponent* hitComponent, AActor* otherActor, UPrimitiveComponent* otherComp, FVector normalImpulse, const FHitResult& hit)
{

}

//************************************
// Method:    RightThumbstickMoveX
// FullName:  AVertCharacter::RightThumbstickMoveX
// Access:    protected 
// Returns:   void
// Qualifier:
// Parameter: float value
//************************************
void AVertCharacter::RightThumbstickMoveX(float value)
{
	mAxisPositions.RightX = value;
}

//************************************
// Method:    RightThumbstickMoveY
// FullName:  AVertCharacter::RightThumbstickMoveY
// Access:    protected 
// Returns:   void
// Qualifier:
// Parameter: float value
//************************************
void AVertCharacter::RightThumbstickMoveY(float value)
{
	mAxisPositions.RightY = value;

	if (mGamepadWantsToGrapple && mGamepadSwipePassesDone < GamepadSwipeRequiredPasses)
	{
		mSwipePasses.Add(GetAxisPostisions().GetPlayerRightThumbstick());
		mGamepadSwipePassesDone += 1;
	}
	else if (mGamepadWantsToGrapple)
	{
		FVector center = FVector::ZeroVector;
		for (auto pass : mSwipePasses)
		{
			center += pass;
		}
		mGamepadSwipeDirection = center.GetSafeNormal();

		mSwipePasses.Empty(GamepadSwipeRequiredPasses);
		mGamepadSwipePassesDone = 0;
		mGamepadWantsToGrapple = false;

		ExecuteActionGrappleShoot();
		mGamepadSwipeDirection = FVector::ZeroVector;
	}
}

//************************************
// Method:    LeftThumbstickMoveY
// FullName:  AVertCharacter::LeftThumbstickMoveY
// Access:    protected 
// Returns:   void
// Qualifier:
// Parameter: float value
//************************************
void AVertCharacter::LeftThumbstickMoveY(float value)
{
	mAxisPositions.LeftY = value;

	if (AimArrowAnchor && AimArrowComponent)
	{
		FRotator AimRotation = GetAxisPostisions().GetPlayerLeftThumbstickDirection() == FVector::ZeroVector ?
			GetController()->GetControlRotation() :
			GetAxisPostisions().GetPlayerLeftThumbstickDirection().Rotation();

		AimArrowAnchor->SetWorldRotation(AimRotation);
		AimArrowComponent->SetAimDirection(AimRotation.Vector().GetSafeNormal());
	}
}

//************************************
// Method:    MouseMove
// FullName:  AVertCharacter::MouseMove
// Access:    protected 
// Returns:   void
// Qualifier:
// Parameter: float value
//************************************
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

//************************************
// Method:    UsingGamepad
// FullName:  AVertCharacter::UsingGamepad
// Access:    public 
// Returns:   const bool
// Qualifier: const
//************************************
const bool AVertCharacter::UsingGamepad() const
{
	if (AVertPlayerController* pc = GetPlayerController())
	{
		return pc->UsingGamepad();
	}

	return true;
}

//************************************
// Method:    RecordRecentHit
// FullName:  AVertCharacter::RecordRecentHit
// Access:    private 
// Returns:   void
// Qualifier:
// Parameter: AController * attacker
//************************************
void AVertCharacter::RecordRecentHit(AController* attacker)
{
	constexpr float cAttackExpiryTime = 10.f;

	APlayerController* PlayerController = Cast<APlayerController>(attacker);
	if (!PlayerController) // Ignore non-player characters, they don't need points.
		return;

	FTimerManager& timerMan = GetWorldTimerManager();
	FTimerDelegate timerDelegate;
	timerDelegate.BindUFunction(this, FName("RecentHitExpired"), attacker);

	if (mRecentHitters.Contains(attacker))
	{
		timerMan.SetTimer(mRecentHitters[attacker], timerDelegate, cAttackExpiryTime, false);
	}
	else
	{
		FTimerHandle timerHandle;
		timerMan.SetTimer(timerHandle, timerDelegate, cAttackExpiryTime, false);
		mRecentHitters.Add(attacker, timerHandle);
	}
}

//************************************
// Method:    GetRecentHitters
// FullName:  AVertCharacter::GetRecentHitters
// Access:    public 
// Returns:   TArray<AController*>
// Qualifier:
//************************************
TArray<AController*> AVertCharacter::GetRecentHitters()
{
	TArray<AController*> assistors;
	for (auto it = mRecentHitters.CreateConstIterator(); it; ++it)
	{
		if (it.Key() && !assistors.Contains(it.Key()))
		{
			assistors.Add(it.Key());
		}
	}
	return assistors;
}

//************************************
// Method:    RecentHitExpired
// FullName:  AVertCharacter::RecentHitExpired
// Access:    protected 
// Returns:   void
// Qualifier:
// Parameter: AController * attacker
//************************************
void AVertCharacter::RecentHitExpired(AController* attacker)
{
	if (attacker && mRecentHitters.Contains(attacker))
	{
		mRecentHitters.Remove(attacker);
	}
}

float AVertCharacter::TakeDamage(float Damage, const FDamageEvent& DamageEvent, class AController* EventInstigator, class AActor* DamageCauser)
{
	float damageTaken = Super::TakeDamage(Damage, DamageEvent, EventInstigator, DamageCauser);

	if (damageTaken > 0)
	{
		APawn* pawnInstigator = nullptr;
		if (EventInstigator)
		{
			pawnInstigator = EventInstigator->GetPawn();
			RecordRecentHit(EventInstigator);
		}
	}

	return damageTaken;
}
