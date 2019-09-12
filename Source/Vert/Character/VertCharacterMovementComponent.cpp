// Fill out your copyright notice in the Description page of Project Settings.

#include "VertCharacterMovementComponent.h"
#include "Vert.h"

DECLARE_LOG_CATEGORY_CLASS(LogVertCharacterMovement, Log, All);

const float VERTICAL_SLOPE_NORMAL_Z = 0.001f; // Slope is vertical if Abs(Normal.Z) <= this threshold. Accounts for precision problems that sometimes angle normals slightly off horizontal for vertical surface.

bool UVertCharacterMovementComponent::DoJump(bool replayingMoves)
{
	if (mCharacterOwner.IsValid() && mCharacterOwner->CanJump())
	{
		// Don't jump if we can't move up/down.
		if (!bConstrainToPlane || FMath::Abs(PlaneConstraintNormal.Z) != 1.f)
		{
			float zVelocity = JumpZVelocity;

			if (mCharacterOwner->CanWallJump())
			{

			}
			else if (mCharacterOwner->GetClimbingComponent()->IsClimbingLedge())
			{

			}

			Velocity.Z = zVelocity;
			SetMovementMode(MOVE_Falling);
			return true;
		}
	}

	return false;
}

//************************************
// Method:    BeginPlay
// FullName:  UVertCharacterMovementComponent::BeginPlay
// Access:    virtual public 
// Returns:   void
// Qualifier:
//************************************
void UVertCharacterMovementComponent::BeginPlay()
{
	Super::BeginPlay();

	mStartingGravityScale = GravityScale;
	mStartingGroundFriction = GroundFriction;
	mStartingAirLateralFriction = FallingLateralFriction;

	if (AVertCharacter* character = Cast<AVertCharacter>(CharacterOwner))
	{
		mCharacterOwner = character;
		mDashingComponent = character->GetDashingComponent();
		mGrapplingComponent = character->GetGrapplingComponent();

		check(mGrapplingComponent.IsValid() && mDashingComponent.IsValid());
	} else { UE_LOG(LogVertCharacterMovement, Error, TEXT("[%s] unable to find AVertCharacter owner.")); }
}

//************************************
// Method:    IsFalling
// FullName:  UVertCharacterMovementComponent::IsFalling
// Access:    virtual public 
// Returns:   bool
// Qualifier: const
//************************************
bool UVertCharacterMovementComponent::IsFalling() const
{
	return (MovementMode == MOVE_Custom && CustomMovementMode == static_cast<uint8>(ECustomMovementMode::MOVE_GrappleFall)) || Super::IsFalling();
}

//************************************
// Method:    IsMovingOnGround
// FullName:  UVertCharacterMovementComponent::IsMovingOnGround
// Access:    virtual public 
// Returns:   bool
// Qualifier: const
//************************************
bool UVertCharacterMovementComponent::IsMovingOnGround() const
{
	return (MovementMode == MOVE_Custom && CustomMovementMode == static_cast<uint8>(ECustomMovementMode::MOVE_GrappleWalk)) || Super::IsMovingOnGround();
}

//************************************
// Method:    SetDashFrictionForTime
// FullName:  UVertCharacterMovementComponent::SetDashFrictionForTime
// Access:    virtual public 
// Returns:   void
// Qualifier:
// Parameter: float newFriction
// Parameter: float time
//************************************
void UVertCharacterMovementComponent::SetDashFrictionForTime(float time, bool disableGravity /*= false*/)
{
	/*FallingLateralFriction = */
	GroundFriction = 0.f;
	if(disableGravity)
		GravityScale = 0.f;

	if (GetWorld()->GetTimerManager().IsTimerActive(mTimerHandle_PostDash))
	{
		GetWorld()->GetTimerManager().ClearTimer(mTimerHandle_PostDash);
		FallingLateralFriction = mStartingAirLateralFriction;
	}

	GetWorld()->GetTimerManager().SetTimer(mTimerHandle_Friction, this, &UVertCharacterMovementComponent::DashFrictionEnded, time, false);
}

//************************************
// Method:    DashFrictionEnded
// FullName:  UVertCharacterMovementComponent::DashFrictionEnded
// Access:    virtual protected 
// Returns:   void
// Qualifier:
//************************************
void UVertCharacterMovementComponent::DashFrictionEnded()
{
	static constexpr float scAirSlowdownFriction = 3.f;
	static constexpr float scAirSlowdownTime = .5f;

	GroundFriction = mStartingGroundFriction;
	GravityScale = mStartingGravityScale;
	FallingLateralFriction = scAirSlowdownFriction;
	GetWorld()->GetTimerManager().SetTimer(mTimerHandle_PostDash, this, &UVertCharacterMovementComponent::DashFrictionCorrectionEnded, scAirSlowdownTime, false);
}

//************************************
// Method:    DashFrictionCorrectionEnded
// FullName:  UVertCharacterMovementComponent::DashFrictionCorrectionEnded
// Access:    virtual protected 
// Returns:   void
// Qualifier:
//************************************
void UVertCharacterMovementComponent::DashFrictionCorrectionEnded()
{
	FallingLateralFriction = mStartingAirLateralFriction;
}

//************************************
// Method:    SetPostLandedPhysics
// FullName:  UVertCharacterMovementComponent::SetPostLandedPhysics
// Access:    virtual public 
// Returns:   void
// Qualifier:
// Parameter: const FHitResult & Hit
//************************************
void UVertCharacterMovementComponent::SetPostLandedPhysics(const FHitResult& Hit)
{
	EMovementMode previousMode = MovementMode;
	uint8 previousCustom = CustomMovementMode;
	Super::SetPostLandedPhysics(Hit);

	if (previousMode == MOVE_Custom && previousCustom == static_cast<uint8>(ECustomMovementMode::MOVE_GrappleFall) && MovementMode == MOVE_Walking)
	{
		SetMovementMode(MOVE_Custom, static_cast<uint8>(ECustomMovementMode::MOVE_GrappleWalk));
	}
}

//************************************
// Method:    PhysCustom
// FullName:  UVertCharacterMovementComponent::PhysCustom
// Access:    virtual protected 
// Returns:   void
// Qualifier:
// Parameter: float deltaTime
// Parameter: int32 Iterations
//************************************
void UVertCharacterMovementComponent::PhysCustom(float deltaTime, int32 Iterations)
{
	Super::PhysCustom(deltaTime, Iterations);

	switch (CustomMovementMode)
	{
	case ECustomMovementMode::MOVE_Climbing:
		PhysClimbing(deltaTime, Iterations);
		break;

	case ECustomMovementMode::MOVE_GrappleWalk:
		PhysGrappling(deltaTime, Iterations);
		break;

	case ECustomMovementMode::MOVE_GrappleFall:
		PhysGrappleFalling(deltaTime, Iterations);
		break;
	}
}

//************************************
// Method:    PhysClimbing
// FullName:  UVertCharacterMovementComponent::PhysClimbing
// Access:    virtual protected 
// Returns:   void
// Qualifier:
// Parameter: float deltaTime
// Parameter: int32 iterations
//************************************
void UVertCharacterMovementComponent::PhysClimbing(float deltaTime, int32 iterations)
{
	UE_LOG(LogVertCharacterMovement, Warning, TEXT("Function UVertCharacterMovementComponent::PhysClimbing is not currently implemented (use MOVE_Flying instead)."));
	SetMovementMode(MOVE_Falling);
}

//************************************
// Method:    PhysGrappling
// FullName:  UVertCharacterMovementComponent::PhysGrappling
// Access:    virtual protected 
// Returns:   void
// Qualifier:
// Parameter: float deltaTime
// Parameter: int32 iterations
//************************************
void UVertCharacterMovementComponent::PhysGrappling(float deltaTime, int32 iterations)
{
	if (deltaTime < MIN_TICK_TIME)
	{
		return;
	}

	if (mGrapplingComponent.IsValid())
	{
		if (deltaTime < MIN_TICK_TIME)
		{
			return;
		}

		if (!CharacterOwner || (!CharacterOwner->Controller && !bRunPhysicsWithNoController && !HasAnimRootMotion() && !CurrentRootMotion.HasOverrideVelocity() && (CharacterOwner->Role != ROLE_SimulatedProxy)))
		{
			Acceleration = FVector::ZeroVector;
			Velocity = FVector::ZeroVector;
			return;
		}

		if (!UpdatedComponent->IsQueryCollisionEnabled())
		{
			SetMovementMode(MOVE_Walking);
			return;
		}

		checkCode(ensureMsgf(!Velocity.ContainsNaN(), TEXT("PhysWalking: Velocity contains NaN before Iteration (%s)\n%s"), *GetPathNameSafe(this), *Velocity.ToString()));

		bJustTeleported = false;
		bool bCheckedFall = false;
		bool bTriedLedgeMove = false;
		float remainingTime = deltaTime;

		// Perform the move
		while ((remainingTime >= MIN_TICK_TIME) && (iterations < MaxSimulationIterations) && CharacterOwner && (CharacterOwner->Controller || bRunPhysicsWithNoController || HasAnimRootMotion() || CurrentRootMotion.HasOverrideVelocity() || (CharacterOwner->Role == ROLE_SimulatedProxy)))
		{
			iterations++;
			bJustTeleported = false;
			const float timeTick = GetSimulationTimeStep(remainingTime, iterations);
			remainingTime -= timeTick;

			// Save current values
			UPrimitiveComponent * const OldBase = GetMovementBase();
			const FVector PreviousBaseLocation = (OldBase != NULL) ? OldBase->GetComponentLocation() : FVector::ZeroVector;
			const FVector OldLocation = UpdatedComponent->GetComponentLocation();
			const FFindFloorResult OldFloor = CurrentFloor;

			RestorePreAdditiveRootMotionVelocity();

			// Ensure velocity is horizontal.
			MaintainHorizontalGroundVelocity();
			const FVector OldVelocity = Velocity;
			Acceleration.Z = 0.f;

			// Apply acceleration
			if (!HasAnimRootMotion() && !CurrentRootMotion.HasOverrideVelocity())
			{
				CalcVelocity(timeTick, GroundFriction, false, GetMaxBrakingDeceleration());
				checkCode(ensureMsgf(!Velocity.ContainsNaN(), TEXT("PhysWalking: Velocity contains NaN after CalcVelocity (%s)\n%s"), *GetPathNameSafe(this), *Velocity.ToString()));
			}

			ApplyRootMotionToVelocity(timeTick);
			checkCode(ensureMsgf(!Velocity.ContainsNaN(), TEXT("PhysWalking: Velocity contains NaN after Root Motion application (%s)\n%s"), *GetPathNameSafe(this), *Velocity.ToString()));

			if (IsFalling())
			{
				// Root motion could have put us into Falling.
				// No movement has taken place this movement tick so we pass on full time/past iteration count
				StartNewPhysics(remainingTime + timeTick, iterations - 1);
				return;
			}

			// Compute move parameters
			const FVector MoveVelocity = Velocity;
			const FVector Delta = timeTick * MoveVelocity;
			const bool bZeroDelta = Delta.IsNearlyZero();
			FStepDownResult StepDownResult;

			if (bZeroDelta)
			{
				remainingTime = 0.f;
			}
			else
			{
				// try to move forward
				MoveAlongFloor(MoveVelocity, timeTick, &StepDownResult);

				if (IsFalling())
				{
					// pawn decided to jump up
					const float DesiredDist = Delta.Size();
					if (DesiredDist > KINDA_SMALL_NUMBER)
					{
						const float ActualDist = (UpdatedComponent->GetComponentLocation() - OldLocation).Size2D();
						remainingTime += timeTick * (1.f - FMath::Min(1.f, ActualDist / DesiredDist));
					}
					StartNewPhysics(remainingTime, iterations);
					return;
				}
				else if (IsSwimming()) //just entered water
				{
					StartSwimming(OldLocation, OldVelocity, timeTick, remainingTime, iterations);
					mGrapplingComponent->Reset();
					return;
				}
			}

			// Update floor.
			// StepUp might have already done it for us.
			if (StepDownResult.bComputedFloor)
			{
				CurrentFloor = StepDownResult.FloorResult;
			}
			else
			{
				FindFloor(UpdatedComponent->GetComponentLocation(), CurrentFloor, bZeroDelta, NULL);
			}

			// check for ledges here
			const bool bCheckLedges = !CanWalkOffLedges();
			if (bCheckLedges && !CurrentFloor.IsWalkableFloor())
			{
				// calculate possible alternate movement
				const FVector GravDir = FVector(0.f, 0.f, -1.f);
				const FVector NewDelta = bTriedLedgeMove ? FVector::ZeroVector : GetLedgeMove(OldLocation, Delta, GravDir);
				if (!NewDelta.IsZero())
				{
					// first revert this move
					RevertMove(OldLocation, OldBase, PreviousBaseLocation, OldFloor, false);

					// avoid repeated ledge moves if the first one fails
					bTriedLedgeMove = true;

					// Try new movement direction
					Velocity = NewDelta / timeTick;
					remainingTime += timeTick;
					continue;
				}
				else
				{
					// see if it is OK to jump
					// @todo collision : only thing that can be problem is that oldbase has world collision on
					bool bMustJump = bZeroDelta || (OldBase == NULL || (!OldBase->IsQueryCollisionEnabled() && MovementBaseUtility::IsDynamicBase(OldBase)));
					if ((bMustJump || !bCheckedFall) && CheckFall(OldFloor, CurrentFloor.HitResult, Delta, OldLocation, remainingTime, timeTick, iterations, bMustJump))
					{
						return;
					}
					bCheckedFall = true;

					// revert this move
					RevertMove(OldLocation, OldBase, PreviousBaseLocation, OldFloor, true);
					remainingTime = 0.f;
					break;
				}
			}
			else
			{
				// Validate the floor check
				if (CurrentFloor.IsWalkableFloor())
				{
					if (ShouldCatchAir(OldFloor, CurrentFloor))
					{
						CharacterOwner->OnWalkingOffLedge(OldFloor.HitResult.ImpactNormal, OldFloor.HitResult.Normal, OldLocation, timeTick);
						if (IsMovingOnGround())
						{
							// If still walking, then fall. If not, assume the user set a different mode they want to keep.
							StartFalling(iterations, remainingTime, timeTick, Delta, OldLocation);
						}
						return;
					}

					AdjustFloorHeight();
					SetBase(CurrentFloor.HitResult.Component.Get(), CurrentFloor.HitResult.BoneName);
				}
				else if (CurrentFloor.HitResult.bStartPenetrating && remainingTime <= 0.f)
				{
					// The floor check failed because it started in penetration
					// We do not want to try to move downward because the downward sweep failed, rather we'd like to try to pop out of the floor.
					FHitResult Hit(CurrentFloor.HitResult);
					Hit.TraceEnd = Hit.TraceStart + FVector(0.f, 0.f, MAX_FLOOR_DIST);
					const FVector RequestedAdjustment = GetPenetrationAdjustment(Hit);
					ResolvePenetration(RequestedAdjustment, Hit, UpdatedComponent->GetComponentQuat());
					bForceNextFloorCheck = true;
				}

				// check if just entered water
				if (IsSwimming())
				{
					mGrapplingComponent->Reset();
					StartSwimming(OldLocation, Velocity, timeTick, remainingTime, iterations);
					return;
				}

				// See if we need to start falling.
				if (!CurrentFloor.IsWalkableFloor() && !CurrentFloor.HitResult.bStartPenetrating)
				{
					const bool bMustJump = bJustTeleported || bZeroDelta || (OldBase == NULL || (!OldBase->IsQueryCollisionEnabled() && MovementBaseUtility::IsDynamicBase(OldBase)));
					if ((bMustJump || !bCheckedFall) && CheckFall(OldFloor, CurrentFloor.HitResult, Delta, OldLocation, remainingTime, timeTick, iterations, bMustJump))
					{
						return;
					}
					bCheckedFall = true;
				}
			}


			// Allow overlap events and such to change physics state and velocity
			if (IsMovingOnGround())
			{
				// Make velocity reflect actual move
				if (!bJustTeleported && !HasAnimRootMotion() && !CurrentRootMotion.HasOverrideVelocity() && timeTick >= MIN_TICK_TIME)
				{
					// TODO-RootMotionSource: Allow this to happen during partial override Velocity, but only set allowed axes?
					Velocity = (UpdatedComponent->GetComponentLocation() - OldLocation) / timeTick;
				}
			}

			// If we didn't move at all this iteration then abort (since future iterations will also be stuck).
			if (UpdatedComponent->GetComponentLocation() == OldLocation)
			{
				remainingTime = 0.f;
				break;
			}

			if (mGrapplingComponent->GetActualLineLengthSqr() > FMath::Square(mGrapplingComponent->GrappleConfig.MaxLineLength))
			{
				StartFalling(iterations, remainingTime, timeTick, Delta, OldLocation);
			}
		}

		if (IsMovingOnGround())
		{
			MaintainHorizontalGroundVelocity();
		}
	}
	else
	{
		UE_LOG(LogVertCharacterMovement, Warning, TEXT("Function UVertCharacterMovementComponent::PhysGrappling is not compatible without a UGrapplingComponent attached to character."));
		SetMovementMode(MOVE_Falling);
	}
}

//************************************
// Method:    PhysGrappleFalling
// FullName:  UVertCharacterMovementComponent::PhysGrappleFalling
// Access:    virtual protected 
// Returns:   void
// Qualifier:
// Parameter: float deltaTime
// Parameter: int32 iterations
//************************************
void UVertCharacterMovementComponent::PhysGrappleFalling(float deltaTime, int32 iterations)
{
	if (mGrapplingComponent.IsValid())
	{
		if (deltaTime < MIN_TICK_TIME)
		{
			return;
		}

		FVector FallAcceleration = GetFallingLateralAcceleration(deltaTime);
		FallAcceleration.Z = 0.f;
		const bool bHasAirControl = (FallAcceleration.SizeSquared2D() > 0.f);

		float remainingTime = deltaTime;
		while ((remainingTime >= MIN_TICK_TIME) && (iterations < MaxSimulationIterations))
		{
			iterations++;
			const float timeTick = GetSimulationTimeStep(remainingTime, iterations);
			remainingTime -= timeTick;

			const FVector OldLocation = UpdatedComponent->GetComponentLocation();
			const FQuat PawnRotation = UpdatedComponent->GetComponentQuat();
			bJustTeleported = false;

			RestorePreAdditiveRootMotionVelocity();

			FVector OldVelocity = Velocity;
			FVector VelocityNoAirControl = Velocity;

			// Apply input
			if (!HasAnimRootMotion() && !CurrentRootMotion.HasOverrideVelocity())
			{
				const float MaxDecel = GetMaxBrakingDeceleration();
				// Compute VelocityNoAirControl
				if (bHasAirControl)
				{
					// Find velocity *without* acceleration.
					TGuardValue<FVector> RestoreAcceleration(Acceleration, FVector::ZeroVector);
					TGuardValue<FVector> RestoreVelocity(Velocity, Velocity);
					Velocity.Z = 0.f;
					CalcVelocity(timeTick, FallingLateralFriction, false, MaxDecel);
					VelocityNoAirControl = FVector(Velocity.X, Velocity.Y, OldVelocity.Z);
				}

				// Compute Velocity
				{
					// Acceleration = FallAcceleration for CalcVelocity(), but we restore it after using it.
					TGuardValue<FVector> RestoreAcceleration(Acceleration, FallAcceleration);
					Velocity.Z = 0.f;
					CalcVelocity(timeTick, FallingLateralFriction, false, MaxDecel);
					Velocity.Z = OldVelocity.Z;
				}

				// Just copy Velocity to VelocityNoAirControl if they are the same (ie no acceleration).
				if (!bHasAirControl)
				{
					VelocityNoAirControl = Velocity;
				}
			}

			// Apply gravity
			const FVector Gravity(0.f, 0.f, GetGravityZ());
			Velocity = NewFallVelocity(Velocity, Gravity, timeTick);
			VelocityNoAirControl = NewFallVelocity(VelocityNoAirControl, Gravity, timeTick);
			const FVector AirControlAccel = (Velocity - VelocityNoAirControl) / timeTick;

			ApplyRootMotionToVelocity(timeTick);

			if (bNotifyApex && CharacterOwner->Controller && (Velocity.Z <= 0.f))
			{
				// Just passed jump apex since now going down
				bNotifyApex = false;
				NotifyJumpApex();
			}


			// Move
			FHitResult Hit(1.f);
			FVector Adjusted = 0.5f*(OldVelocity + Velocity) * timeTick;
			SafeMoveUpdatedComponent(Adjusted, PawnRotation, true, Hit);

			if (!HasValidData())
			{
				return;
			}

			float LastMoveTimeSlice = timeTick;
			float subTimeTickRemaining = timeTick * (1.f - Hit.Time);

			if (IsSwimming()) //just entered water
			{
				remainingTime += subTimeTickRemaining;
				mGrapplingComponent->Reset();
				StartSwimming(OldLocation, OldVelocity, timeTick, remainingTime, iterations);
				return;
			}
			else if (Hit.bBlockingHit)
			{
				if (IsValidLandingSpot(UpdatedComponent->GetComponentLocation(), Hit))
				{
					remainingTime += subTimeTickRemaining;
					ProcessLanded(Hit, remainingTime, iterations);
					return;
				}
				else
				{
					// Compute impact deflection based on final velocity, not integration step.
					// This allows us to compute a new velocity from the deflected vector, and ensures the full gravity effect is included in the slide result.
					Adjusted = Velocity * timeTick;

					// See if we can convert a normally invalid landing spot (based on the hit result) to a usable one.
					if (!Hit.bStartPenetrating && ShouldCheckForValidLandingSpot(timeTick, Adjusted, Hit))
					{
						const FVector PawnLocation = UpdatedComponent->GetComponentLocation();
						FFindFloorResult FloorResult;
						FindFloor(PawnLocation, FloorResult, false);
						if (FloorResult.IsWalkableFloor() && IsValidLandingSpot(PawnLocation, FloorResult.HitResult))
						{
							remainingTime += subTimeTickRemaining;
							ProcessLanded(FloorResult.HitResult, remainingTime, iterations);
							return;
						}
					}

					HandleImpact(Hit, LastMoveTimeSlice, Adjusted);

					// If we've changed physics mode, abort.
					if (!HasValidData() || !IsFalling())
					{
						return;
					}

					// Limit air control based on what we hit.
					// We moved to the impact point using air control, but may want to deflect from there based on a limited air control acceleration.
					if (bHasAirControl)
					{
						const bool bCheckLandingSpot = false; // we already checked above.
						const FVector AirControlDeltaV = LimitAirControl(LastMoveTimeSlice, AirControlAccel, Hit, bCheckLandingSpot) * LastMoveTimeSlice;
						Adjusted = (VelocityNoAirControl + AirControlDeltaV) * LastMoveTimeSlice;
					}

					const FVector OldHitNormal = Hit.Normal;
					const FVector OldHitImpactNormal = Hit.ImpactNormal;
					FVector Delta = ComputeSlideVector(Adjusted, 1.f - Hit.Time, OldHitNormal, Hit);

					// Compute velocity after deflection (only gravity component for RootMotion)
					if (subTimeTickRemaining > KINDA_SMALL_NUMBER && !bJustTeleported)
					{
						const FVector NewVelocity = (Delta / subTimeTickRemaining);
						Velocity = HasAnimRootMotion() && !CurrentRootMotion.HasOverrideVelocity() ? FVector(Velocity.X, Velocity.Y, NewVelocity.Z) : NewVelocity;
					}

					if (subTimeTickRemaining > KINDA_SMALL_NUMBER && (Delta | Adjusted) > 0.f)
					{
						// Move in deflected direction.
						SafeMoveUpdatedComponent(Delta, PawnRotation, true, Hit);

						if (Hit.bBlockingHit)
						{
							// hit second wall
							LastMoveTimeSlice = subTimeTickRemaining;
							subTimeTickRemaining = subTimeTickRemaining * (1.f - Hit.Time);

							if (IsValidLandingSpot(UpdatedComponent->GetComponentLocation(), Hit))
							{
								remainingTime += subTimeTickRemaining;
								ProcessLanded(Hit, remainingTime, iterations);
								return;
							}

							HandleImpact(Hit, LastMoveTimeSlice, Delta);

							// If we've changed physics mode, abort.
							if (!HasValidData() || !IsFalling())
							{
								return;
							}

							// Act as if there was no air control on the last move when computing new deflection.
							if (bHasAirControl && Hit.Normal.Z > VERTICAL_SLOPE_NORMAL_Z)
							{
								const FVector LastMoveNoAirControl = VelocityNoAirControl * LastMoveTimeSlice;
								Delta = ComputeSlideVector(LastMoveNoAirControl, 1.f, OldHitNormal, Hit);
							}

							FVector PreTwoWallDelta = Delta;
							TwoWallAdjust(Delta, Hit, OldHitNormal);

							// Limit air control, but allow a slide along the second wall.
							if (bHasAirControl)
							{
								const bool bCheckLandingSpot = false; // we already checked above.
								const FVector AirControlDeltaV = LimitAirControl(subTimeTickRemaining, AirControlAccel, Hit, bCheckLandingSpot) * subTimeTickRemaining;

								// Only allow if not back in to first wall
								if (FVector::DotProduct(AirControlDeltaV, OldHitNormal) > 0.f)
								{
									Delta += (AirControlDeltaV * subTimeTickRemaining);
								}
							}

							// Compute velocity after deflection (only gravity component for RootMotion)
							if (subTimeTickRemaining > KINDA_SMALL_NUMBER && !bJustTeleported)
							{
								const FVector NewVelocity = (Delta / subTimeTickRemaining);
								Velocity = HasAnimRootMotion() && !CurrentRootMotion.HasOverrideVelocity() ? FVector(Velocity.X, Velocity.Y, NewVelocity.Z) : NewVelocity;
							}

							// bDitch=true means that pawn is straddling two slopes, neither of which he can stand on
							bool bDitch = ((OldHitImpactNormal.Z > 0.f) && (Hit.ImpactNormal.Z > 0.f) && (FMath::Abs(Delta.Z) <= KINDA_SMALL_NUMBER) && ((Hit.ImpactNormal | OldHitImpactNormal) < 0.f));
							SafeMoveUpdatedComponent(Delta, PawnRotation, true, Hit);
							if (Hit.Time == 0.f)
							{
								// if we are stuck then try to side step
								FVector SideDelta = (OldHitNormal + Hit.ImpactNormal).GetSafeNormal2D();
								if (SideDelta.IsNearlyZero())
								{
									SideDelta = FVector(OldHitNormal.Y, -OldHitNormal.X, 0).GetSafeNormal();
								}
								SafeMoveUpdatedComponent(SideDelta, PawnRotation, true, Hit);
							}

							if (bDitch || IsValidLandingSpot(UpdatedComponent->GetComponentLocation(), Hit) || Hit.Time == 0.f)
							{
								remainingTime = 0.f;
								ProcessLanded(Hit, remainingTime, iterations);
								return;
							}
							else if (GetPerchRadiusThreshold() > 0.f && Hit.Time == 1.f && OldHitImpactNormal.Z >= GetWalkableFloorZ())
							{
								// We might be in a virtual 'ditch' within our perch radius. This is rare.
								const FVector PawnLocation = UpdatedComponent->GetComponentLocation();
								const float ZMovedDist = FMath::Abs(PawnLocation.Z - OldLocation.Z);
								const float MovedDist2DSq = (PawnLocation - OldLocation).SizeSquared2D();
								if (ZMovedDist <= 0.2f * timeTick && MovedDist2DSq <= 4.f * timeTick)
								{
									Velocity.X += 0.25f * GetMaxSpeed() * (FMath::FRand() - 0.5f);
									Velocity.Y += 0.25f * GetMaxSpeed() * (FMath::FRand() - 0.5f);
									Velocity.Z = FMath::Max<float>(JumpZVelocity * 0.25f, 1.f);
									Delta = Velocity * timeTick;
									SafeMoveUpdatedComponent(Delta, PawnRotation, true, Hit);
								}
							}
						}
					}
				}
			}

			if (Velocity.SizeSquared2D() <= KINDA_SMALL_NUMBER * 10.f)
			{
				Velocity.X = 0.f;
				Velocity.Y = 0.f;
			}
		}
	}
	else
	{
		UE_LOG(LogVertCharacterMovement, Warning, TEXT("Function UVertCharacterMovementComponent::PhysGrappleFalling is not compatible without a UGrapplingComponent attached to character."));
		SetMovementMode(MOVE_Falling);
	}
}

//************************************
// Method:    CorrectGrapplePosition
// FullName:  UVertCharacterMovementComponent::CorrectGrapplePosition
// Access:    virtual protected 
// Returns:   void
// Qualifier:
//************************************
void UVertCharacterMovementComponent::CorrectGrapplePosition()
{
	if (mGrapplingComponent.IsValid())
	{
		float desiredLength = mGrapplingComponent->GetLineLength();
		if (mGrapplingComponent->GetActualLineLengthSqr() > FMath::Square(desiredLength))
		{
			FVector offset = mGrapplingComponent->RelativeLocation;
			FVector desiredLocation = (-mGrapplingComponent->GetLineDirection()) * desiredLength;
			mCharacterOwner->SetActorLocation(desiredLocation);
		}
	}
}

//************************************
// Method:    SetMovementMode
// FullName:  UVertCharacterMovementComponent::SetMovementMode
// Access:    virtual public 
// Returns:   void
// Qualifier:
// Parameter: EMovementMode NewMovementMode
// Parameter: uint8 NewCustomMode
//************************************
void UVertCharacterMovementComponent::SetMovementMode(EMovementMode NewMovementMode, uint8 NewCustomMode /*= 0*/)
{
 	if (mGrapplingComponent.IsValid() && MovementMode != NewMovementMode && CustomMovementMode != NewCustomMode && NewCustomMode == static_cast<uint8>(ECustomMovementMode::MOVE_GrappleFall))
	{
		mGrapplingComponent->LockDesiredLineLength();
	}

	if (MovementMode == MOVE_Custom 
		&& (CustomMovementMode == static_cast<uint8>(ECustomMovementMode::MOVE_GrappleWalk)
			|| CustomMovementMode == static_cast<uint8>(ECustomMovementMode::MOVE_GrappleFall))
		&& mGrapplingComponent.IsValid()
		&& mGrapplingComponent->GetGrappleState() == EGrappleState::HookDeployed)
	{
		switch (NewMovementMode)
		{
		case MOVE_Falling:
			Super::SetMovementMode(MOVE_Custom, static_cast<uint8>(ECustomMovementMode::MOVE_GrappleFall));
			break;
		case MOVE_Walking:
			Super::SetMovementMode(MOVE_Custom, static_cast<uint8>(ECustomMovementMode::MOVE_GrappleWalk));
			break;
		case MOVE_Swimming: // intentional fall-through
			mGrapplingComponent->Reset();
		default:
			Super::SetMovementMode(NewMovementMode, NewCustomMode);
		}
	}
	else
	{
		Super::SetMovementMode(NewMovementMode, NewCustomMode);
	}	
}

//************************************
// Method:    NewFallVelocity
// FullName:  UVertCharacterMovementComponent::NewFallVelocity
// Access:    virtual public 
// Returns:   FVector
// Qualifier: const
// Parameter: const FVector & InitialVelocity
// Parameter: const FVector & Gravity
// Parameter: float DeltaTime
//************************************
FVector UVertCharacterMovementComponent::NewFallVelocity(const FVector& InitialVelocity, const FVector& Gravity, float DeltaTime) const
{
	// Add a special case for if we're grappling
	FVector GravityDir = Gravity.GetSafeNormal();
	if (MovementMode == MOVE_Custom
		&& CustomMovementMode == static_cast<uint8>(ECustomMovementMode::MOVE_GrappleFall)
		&& mGrapplingComponent.IsValid()
		&& mGrapplingComponent->GetActualLineLengthSqr() > mGrapplingComponent->GrappleConfig.MaxLineLength
		)
	{
		return DetermineVelocityAfterGravityForGrapple(InitialVelocity, Gravity, DeltaTime);
	}

	return Super::NewFallVelocity(InitialVelocity, Gravity, DeltaTime);
}

//************************************
// Method:    DetermineVelocityAfterGravityForGrapple
// FullName:  UVertCharacterMovementComponent::DetermineVelocityAfterGravityForGrapple
// Access:    public 
// Returns:   FVector
// Qualifier: const
// Parameter: const FVector & initialVelocity
// Parameter: const FVector & gravity
// Parameter: float deltaTime
//************************************
FVector UVertCharacterMovementComponent::DetermineVelocityAfterGravityForGrapple(const FVector& initialVelocity, const FVector& gravity, float deltaTime) const
{
	FVector Result = initialVelocity;

	if (!gravity.IsZero() && (bApplyGravityWhileJumping || !(CharacterOwner && CharacterOwner->IsJumpProvidingForce())))
	{
		FVector GravityDir = gravity.GetSafeNormal();
		
		// Apply gravity.
		Result += gravity * deltaTime;
		
		// determine centripetal acceleration
		FVector direction = mGrapplingComponent->GetLineDirection();
		float radius = mGrapplingComponent->GetActualLineLength();
		if (radius > 0) // Check before we go dividing by zero!
		{
			float ac = FMath::Square(Result.Size()) / radius;
			FVector acV = ac * direction;

			// apply centripetal acceleration
			Result += acV * deltaTime;
		}
		else
		{
			UE_LOG(LogVertCharacterMovement, Warning, TEXT("Avoided potential divide by zero in function UVerCharacterMovementComponent::DetermineVelocityAfterGravityForGrapple."));
		}
		
		// project new velocity onto the perpendicular plane
		FVector toHook = mGrapplingComponent->GetVectorToHook();
		FVector plane(toHook.Z, 0, -toHook.X);
		Result = FVector::PointPlaneProject(Result, plane, direction);

		const float TerminalLimit = FMath::Abs(GetPhysicsVolume()->TerminalVelocity);

		// Don't exceed terminal velocity.
		if ((Result | GravityDir) > TerminalLimit)
		{
			Result = FVector::PointPlaneProject(Result, FVector::ZeroVector, GravityDir) + GravityDir * TerminalLimit;
		}
	}

	return Result;
}

//************************************
// Method:    StartFalling
// FullName:  UVertCharacterMovementComponent::StartFalling
// Access:    virtual public 
// Returns:   void
// Qualifier:
// Parameter: int32 Iterations
// Parameter: float remainingTime
// Parameter: float timeTick
// Parameter: const FVector & Delta
// Parameter: const FVector & subLoc
//************************************
void UVertCharacterMovementComponent::StartFalling(int32 Iterations, float remainingTime, float timeTick, const FVector& Delta, const FVector& subLoc)
{
	// start falling 
	const float DesiredDist = Delta.Size();
	const float ActualDist = (UpdatedComponent->GetComponentLocation() - subLoc).Size2D();
	remainingTime = (DesiredDist < KINDA_SMALL_NUMBER)
		? 0.f
		: remainingTime + timeTick * (1.f - FMath::Min(1.f, ActualDist / DesiredDist));

	if (IsMovingOnGround())
	{
		// This is to catch cases where the first frame of PIE is executed, and the
		// level is not yet visible. In those cases, the player will fall out of the
		// world... So, don't set MOVE_Falling straight away.
		if (!GIsEditor || (GetWorld()->HasBegunPlay() && (GetWorld()->GetTimeSeconds() >= 1.f)))
		{
			if (MovementMode == MOVE_Custom && CustomMovementMode == static_cast<uint8>(ECustomMovementMode::MOVE_GrappleWalk))
			{
				SetMovementMode(MOVE_Custom, static_cast<uint8>(ECustomMovementMode::MOVE_GrappleFall));
			}
			else
			{
				SetMovementMode(MOVE_Falling); //default behavior if script didn't change physics
			}
		}
		else
		{
			// Make sure that the floor check code continues processing during this delay.
			bForceNextFloorCheck = true;
		}
	}
	StartNewPhysics(remainingTime, Iterations);
}