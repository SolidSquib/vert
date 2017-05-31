﻿// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

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
	SideViewCameraComponent->ProjectionMode = ECameraProjectionMode::Perspective;
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
	GetMesh()->SetIsReplicated(true);
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
	
	if (HealthComponent)
	{
		FScriptDelegate onDeath;
		onDeath.BindUFunction(this, TEXT("OnNotifyDeath"));
		HealthComponent->OnDeath.Add(onDeath);
	}

	if (InteractionComponent)
	{
		FScriptDelegate onPickup;
		onPickup.BindUFunction(this, TEXT("OnPickupInteractive"));
		InteractionComponent->Delegate_OnPickupInteractive.Add(onPickup);

		FScriptDelegate onDrop;
		onDrop.BindUFunction(this, TEXT("OnDropInteractive"));
		InteractionComponent->Delegate_OnDropInteractive.Add(onDrop);
	}

	if (DashingComponent)
	{
		FScriptDelegate onDashEnd;
		onDashEnd.BindUFunction(this, TEXT("Character_OnDashEnded"));
		DashingComponent->OnDashEnd.Add(onDashEnd);
	}

	mOnWeaponStateChangedDelegate.BindUFunction(this, TEXT("Character_OnWeaponStateChangeExecuted"));
	mOnWeaponFiredWithRecoilDelegate.BindUFunction(this, TEXT("Character_OnWeaponFiredWithRecoilExecuted"));

	if (UWorld* world = GetWorld())
	{
		if (AGameModeBase* gameMode = world->GetAuthGameMode())
		{
			if (AVertGameMode* vertGameMode = Cast<AVertGameMode>(gameMode))
			{
				vertGameMode->RegisterPlayerPawn(this);
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
					vertGameMode->UnregisterPlayerPawn(this);
				}
			}
		}
	}

	GetWorld()->GetTimerManager().ClearAllTimersForObject(this);
}

void AVertCharacter::Landed(const FHitResult& Hit)
{
	GrapplingComponent->OnLanded();
	DashingComponent->OnLanded();

	Super::Landed(Hit);
}

float AVertCharacter::TakeDamage(float Damage, const FDamageEvent& DamageEvent, class AController* EventInstigator, class AActor* DamageCauser)
{
	if (AVertPlayerController* pc = Cast<AVertPlayerController>(Controller))
	{
		if (pc->HasGodMode())
		{
			return 0.f;
		}
	}

	UE_LOG(LogVertCharacter, Log, TEXT("%s recieved damage from %s"), *GetName(), *DamageCauser->GetName());

	APawn* pawnInstigator = EventInstigator ? EventInstigator->GetPawn() : nullptr;
	return HealthComponent->DealDamage(Super::TakeDamage(Damage, DamageEvent, EventInstigator, DamageCauser), DamageEvent, pawnInstigator, DamageCauser);
}

bool AVertCharacter::CanDie(float KillingDamage, const FDamageEvent& DamageEvent, AController* Killer, AActor* DamageCauser) const
{
	if (mIsDying										// already dying
		|| IsPendingKill()								// already destroyed
		|| Role != ROLE_Authority						// not authority
		|| GetWorld()->GetAuthGameMode<AVertGameMode>() == NULL
		|| GetWorld()->GetAuthGameMode<AVertGameMode>()->GetMatchState() == MatchState::LeavingMap)	// level transition occurring
	{
		return false;
	}

	return true;
}

void AVertCharacter::OnDeath(float killingDamage, const FDamageEvent& damageEvent, APawn* pawnInstigator, AActor* damageCauser)
{
	if (mIsDying)
	{
		return;
	}

	bReplicateMovement = false;
	bTearOff = true;
	mIsDying = true;

	if (Role == ROLE_Authority)
	{
		HealthComponent->ReplicateHit(killingDamage, damageEvent, pawnInstigator, damageCauser, true);

		// play the force feedback effect on the client player controller
		APlayerController* PC = Cast<APlayerController>(Controller);
		if (PC && damageEvent.DamageTypeClass)
		{
			UVertDamageType *damageType = Cast<UVertDamageType>(damageEvent.DamageTypeClass->GetDefaultObject());
			if (damageType && damageType->KilledForceFeedback)
			{
				PC->ClientPlayForceFeedback(damageType->KilledForceFeedback, false, "Damage");
			}
		}
	}

	DetachFromControllerPendingDestroy();
	GetWorld()->GetAuthGameMode<AVertGameMode>()->UnregisterPlayerPawn(this);

	if (GetMesh())
	{
		static FName CollisionProfileName(TEXT("Ragdoll"));
		GetMesh()->SetCollisionProfileName(CollisionProfileName);
	}
	SetActorEnableCollision(true);
}

void AVertCharacter::Suicide()
{
	KilledBy(this);
}

void AVertCharacter::KilledBy(class APawn* EventInstigator)
{
	if (Role == ROLE_Authority && !mIsDying)
	{
		AController* Killer = NULL;
		if (EventInstigator != NULL)
		{
			Killer = EventInstigator->Controller;
			LastHitBy = NULL;
		}

		Die(HealthComponent->GetCurrentDamageModifier(), FDamageEvent(UDamageType::StaticClass()), Killer, NULL);
	}
}

bool AVertCharacter::Die(float KillingDamage, const FDamageEvent& DamageEvent, class AController* Killer, class AActor* DamageCauser)
{
	if (!CanDie(KillingDamage, DamageEvent, Killer, DamageCauser))
		return false;

	// if this is an environmental death then refer to the previous killer so that they receive credit (knocked into lava pits, etc)
	UDamageType const* const DamageType = DamageEvent.DamageTypeClass ? DamageEvent.DamageTypeClass->GetDefaultObject<UDamageType>() : GetDefault<UDamageType>();
	Killer = GetDamageInstigator(Killer, *DamageType);

	AController* const KilledPlayer = (Controller != NULL) ? Controller : Cast<AController>(GetOwner());
	// GetWorld()->GetAuthGameMode<AVertGameMode>()->Killed(Killer, KilledPlayer, this, DamageType);

	NetUpdateFrequency = GetDefault<AVertGameMode>()->NetUpdateFrequency;
	GetCharacterMovement()->ForceReplicationUpdate();

	OnDeath(KillingDamage, DamageEvent, Killer ? Killer->GetPawn() : NULL, DamageCauser);
	return true;
}

void AVertCharacter::FellOutOfWorld(const class UDamageType& dmgType)
{
	Die(HealthComponent->GetCurrentDamageModifier(), FDamageEvent(dmgType.GetClass()), NULL, NULL);
}

void AVertCharacter::ApplyDamageMomentum(float DamageTaken, const FDamageEvent& DamageEvent, APawn* PawnInstigator, AActor* DamageCauser)
{
	Super::ApplyDamageMomentum(DamageTaken, DamageEvent, PawnInstigator, DamageCauser);
}

void AVertCharacter::StopAttacking()
{
	InteractionComponent->StopAttacking();
	Character_OnStopAttackExecuted();
}

bool AVertCharacter::CanFire() const
{
	return HealthComponent->IsAlive();
}

bool AVertCharacter::CanReload() const
{
	return true;
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
	PlayerInputComponent->BindAction("Attack", IE_Pressed, this, &AVertCharacter::ActionAttack);
	PlayerInputComponent->BindAction("Attack", IE_Released, this, &AVertCharacter::StopAttacking);

	PlayerInputComponent->BindAxis("MoveRight", this, &AVertCharacter::ActionMoveRight);
	PlayerInputComponent->BindAxis("LeftThumbstickMoveY", this, &AVertCharacter::LeftThumbstickMoveY);
	PlayerInputComponent->BindAxis("RightThumbstickMoveX", this, &AVertCharacter::RightThumbstickMoveX);
	PlayerInputComponent->BindAxis("RightThumbstickMoveY", this, &AVertCharacter::RightThumbstickMoveY);
	PlayerInputComponent->BindAxis("MouseMove", this, &AVertCharacter::MouseMove);
}

void AVertCharacter::ActionMoveRight(float Value)
{
	mAxisPositions.LeftX = Value;

	AddMovementInput(FVector(1.f, 0.f, 0.f), Value);
}

void AVertCharacter::ActionJump()
{
	Jump();
	Character_OnJumpExecuted();
}

void AVertCharacter::ActionGrappleShoot()
{
	switch (UsingGamepad())
	{
	case true:
		if (!mGamepadOnStandby)
		{
			mGamepadOnStandby = true;
			GetWorld()->GetTimerManager().SetTimer(mTimerHandle, this, &AVertCharacter::ExecuteActionGrappleShoot, 0.01f, false);
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
	if (!GrapplingComponent->GetHookedPrimitive())
	{
		if (GrapplingComponent->ExecuteGrapple(UsingGamepad() ? GetAxisPostisions().GetPlayerRightThumbstickDirection() : GetAxisPostisions().GetPlayerMouseDirection()))
		{
			Character_OnGrappleShootExecuted(GrapplingComponent->GetLineDirection());
		}
	}
	else
	{
		if (GrapplingComponent->StartPulling())
		{
			Character_OnGrapplePullExecuted(GrapplingComponent->GetLineDirection());
		}
	}
}

void AVertCharacter::ActionDash()
{
	FVector direction;

	if (GrapplingComponent->IsGrappleDeployed())
	{
		if (DashingComponent->ExecuteGrappleDash(direction))
		{
			Character_OnDashExecuted(direction, true);
		}
	}
	else
	{
		if (DashingComponent->ExecuteGroundDash(direction))
		{
			Character_OnDashExecuted(direction, false);
		}
	}
}

void AVertCharacter::ActionInteract()
{
	if (AInteractive* interactive = InteractionComponent->AttemptInteract())
	{
		Character_OnInteractExecuted(interactive);
	}
}

void AVertCharacter::ActionAttack()
{
	if (InteractionComponent->AttemptAttack())
	{
		Character_OnStartAttackExecuted(GetWeapon());
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
}
#endif

bool AVertCharacter::CanComponentRecharge(ERechargeRule rule)
{
	if (GrapplingComponent)
	{
		AActor* hookedActor = GrapplingComponent->GetHookedActor();

		return IsGrounded() 
			|| rule == ERechargeRule::OnRechargeTimer 
			|| (rule == ERechargeRule::OnContactGroundOrLatchedAnywhere 
				&& GrapplingComponent->GetGrappleState() == EGrappleState::HookDeployed);
	}
	else
		UE_LOG(LogVertCharacter, Error, TEXT("Hook associated with character [%s] is not valid."), *GetOwner()->GetName());

	return false;
}

bool AVertCharacter::IsMoving()
{
	FVector velocity = GetVelocity();
	return velocity.SizeSquared() > KINDA_SMALL_NUMBER;
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

void AVertCharacter::OnPickupInteractive(AInteractive* interactive, bool wasCaught)
{
	if (ABaseWeapon* weapon = Cast<ABaseWeapon>(interactive))
	{
		weapon->Delegate_OnWeaponFiredWithRecoil.Add(mOnWeaponFiredWithRecoilDelegate);
		weapon->Delegate_OnWeaponStateChanged.Add(mOnWeaponStateChangedDelegate);
	}

	Character_OnPickupNewInteractive(interactive, wasCaught);
}

void AVertCharacter::OnDropInteractive(AInteractive* interactive, bool wasThrown)
{
	if (ABaseWeapon* weapon = Cast<ABaseWeapon>(interactive))
	{
		weapon->Delegate_OnWeaponFiredWithRecoil.Remove(mOnWeaponFiredWithRecoilDelegate);
		weapon->Delegate_OnWeaponStateChanged.Remove(mOnWeaponStateChangedDelegate);
	}

	Character_OnDropCurrentInteractive(interactive, wasThrown);
}

void AVertCharacter::OnNotifyDeath_Implementation(const FTakeHitInfo& lastHit)
{

}

void AVertCharacter::SetRagdollPhysics()
{
	bool bInRagdoll = false;

	if (IsPendingKill())
	{
		bInRagdoll = false;
	}
	else if (!GetMesh() || !GetMesh()->GetPhysicsAsset())
	{
		bInRagdoll = false;
	}
	else
	{
		// initialize physics/etc
		GetMesh()->SetSimulatePhysics(true);
		GetMesh()->WakeAllRigidBodies();
		GetMesh()->bBlendPhysics = true;

		bInRagdoll = true;
	}

	GetCharacterMovement()->StopMovementImmediately();
	GetCharacterMovement()->DisableMovement();
	GetCharacterMovement()->SetComponentTickEnabled(false);

	if (!bInRagdoll)
	{
		// hide and set short lifespan
		TurnOff();
		SetActorHiddenInGame(true);
		SetLifeSpan(1.0f);
	}
	else
	{
		SetLifeSpan(10.0f);
	}
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