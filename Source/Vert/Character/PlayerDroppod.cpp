// Copyright Inside Out Games Limited 2017

#include "PlayerDroppod.h"
#include "Engine/VertPlayerController.h"
#include "Engine/VertCameraManager.h"

// Sets default values
APlayerDroppod::APlayerDroppod()
	: ProjectileMovement(CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement")))
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	ProjectileMovement->SetUpdatedComponent(nullptr);
	ProjectileMovement->bInitialVelocityInLocalSpace = true;
	ProjectileMovement->SetCanEverAffectNavigation(false);
	ProjectileMovement->bAutoActivate = false;

	GetCapsuleComponent()->SetCollisionObjectType(ECC_WorldDynamic);
	GetCapsuleComponent()->SetCanEverAffectNavigation(false);

	GetCharacterMovement()->DefaultLandMovementMode = MOVE_Flying;
	GetCharacterMovement()->bConstrainToPlane = true;
	GetCharacterMovement()->SetPlaneConstraintNormal(FVector(0.0f, -1.0f, 0.0f));

	GetMesh()->SetRenderCustomDepth(true);
}

void APlayerDroppod::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearAllTimersForObject(this);

	Super::EndPlay(EndPlayReason);
}

void APlayerDroppod::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	if (GetMesh())
	{
		// create material instance for setting team colors (3rd person view)
		for (int32 iMat = 0; iMat < GetMesh()->GetNumMaterials(); iMat++)
		{
			MeshMIDs.Add(GetMesh()->CreateAndSetMaterialInstanceDynamic(iMat));
		}
	}

	ProjectileMovement->OnProjectileStop.AddUniqueDynamic(this, &APlayerDroppod::OnPodStopMoving);
	GetCapsuleComponent()->OnComponentBeginOverlap.AddUniqueDynamic(this, &APlayerDroppod::OnPodOverlap);

	mSavedCollisionProfile = GetCapsuleComponent()->GetCollisionResponseToChannels();
}

void APlayerDroppod::BeginPlay()
{
	Super::BeginPlay();

	GetWorldTimerManager().SetTimer(mTimerHandle_AutoDrop, this, &APlayerDroppod::ActionDrop, AimTime, false);
}

// Called to bind functionality to input
void APlayerDroppod::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	
	if (PlayerInputComponent)
	{
		PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &APlayerDroppod::ActionDrop);
		PlayerInputComponent->BindAxis("MoveRight", this, &APlayerDroppod::ActionMoveRight);
	}
}

void APlayerDroppod::ActionMoveRight(float value)
{
	AddMovementInput(FVector(1.f, 0.f, 0.f), value);

	// Set the rotation so that the pod faces his direction of travel.
	if (Controller != nullptr)
	{
		if (value < 0.0f)
		{
			Controller->SetControlRotation(FRotator(0.0, 180.0f, 0.0f));
		}
		else if (value > 0.0f)
		{
			Controller->SetControlRotation(FRotator(0.0f, 0.0f, 0.0f));
		}
	}

	FHitResult ForwardHit, UpwardHit;
	const bool ShouldDescend = FMath::Abs(value) > 0 && TraceForwardForObstacle(ForwardHit) && TraceUpwardsForObstacle(UpwardHit);
	if (ShouldDescend)
	{
		AddMovementInput(FVector::UpVector, -1.f);
	}

	// If we're not colliding with anything and we're also not at the top of the screen, move upwards
	if (!ShouldDescend && !mLaunchedForDeploy && mVertPlayerController.IsValid())
	{
		if (GetActorLocation().Z < mVertPlayerController->GetPodTargetHeight())
		{
			float radius = GetCapsuleComponent()->GetScaledCapsuleRadius();
			float halfHeight = FMath::Max(0.f, GetCapsuleComponent()->GetScaledCapsuleHalfHeight() - radius);
			FVector ActorPosition = GetActorLocation();

			// Trace down to check if we're in blocking geometry.
			FHitResult hit(ForceInit);
			FCollisionQueryParams params;
			params.bFindInitialOverlaps = true;
			params.bTraceComplex = true;
			const FVector start(ActorPosition.X, ActorPosition.Y, ActorPosition.Z - halfHeight);
			const FVector end(ActorPosition.X, ActorPosition.Y, ActorPosition.Z + halfHeight);

			const bool didHit = GetWorld()->SweepSingleByObjectType(hit, start, end, FQuat::Identity, ECC_WorldStatic, FCollisionShape::MakeSphere(radius), params);
			if (!didHit && !hit.bStartPenetrating)
			{
				AddMovementInput(FVector::UpVector, 1.f);
			}
		}
		else // else don't go upwards, and stick within the screen bounds
		{
			SetActorLocation(FVector(GetActorLocation().X, GetActorLocation().Y, mVertPlayerController->GetPodTargetHeight()));
			GetCharacterMovement()->Velocity.Z = 0;
		}
	}

	// Check and constrain the pod to the X bounds of the screen
	if(mVertPlayerController.IsValid())
	{
		float Left, Right;
		mVertPlayerController->GetPodLeftAndRightBounds(Left, Right);
		if (GetActorLocation().X < Left)
		{
			SetActorLocation(FVector(Left, GetActorLocation().Y, GetActorLocation().Z));
		}
		else if (GetActorLocation().X > Right)
		{
			SetActorLocation(FVector(Right, GetActorLocation().Y, GetActorLocation().Z));
		}
	}

	// Check if the pod has left the screen bounds at the bottom, and if it has, emergency eject.
	if (mLaunchedForDeploy && mVertPlayerController.IsValid() && !mEjectedPayload && !GetWorldTimerManager().IsTimerActive(mTimerHandle_SpawnTimer))
	{
		AVertCameraManager* VertCameraMan = Cast<AVertCameraManager>(mVertPlayerController->PlayerCameraManager);
		if (VertCameraMan)
		{
			FVector TopLeft, BottomRight, BottomLeft, TopRight;
			VertCameraMan->GetCurrentPlayerBounds(TopLeft, BottomRight, BottomLeft, TopRight);
			
			if (GetActorLocation().Z <= BottomRight.Z)
			{
				EjectPlayer(true);
			}
		}
	}

	// Dig the pod into the ground.
	if (mMadeImpact)
	{
		SetActorLocation(FMath::VInterpTo(GetActorLocation(), mDigTargetLocation, GetWorld()->GetDeltaSeconds(), DigInterpSpeed));
	}
}

void APlayerDroppod::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	if (AVertPlayerController* VertPC = Cast<AVertPlayerController>(NewController))
	{
		mVertPlayerController = VertPC;
		UpdateTeamColoursAllMIDs();
	}
}

void APlayerDroppod::ActionDrop()
{
	if (!mLaunchedForDeploy)
	{
		if (GetWorldTimerManager().IsTimerActive(mTimerHandle_AutoDrop))
			GetWorldTimerManager().ClearTimer(mTimerHandle_AutoDrop);

		if (ProjectileMovement)
		{
			GetCapsuleComponent()->SetCollisionResponseToAllChannels(ECR_Overlap);
			GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);

			GetCharacterMovement()->StopMovementImmediately();
			GetCharacterMovement()->SetUpdatedComponent(nullptr);

			ProjectileMovement->SetUpdatedComponent(RootComponent);
			ProjectileMovement->Activate();
			ProjectileMovement->Velocity = (-FVector::UpVector) * ProjectileMovement->InitialSpeed;

			OnPodInitVelocity();

			mLaunchedForDeploy = true;
		}
	}
	else if(!mEjectedPayload && !GetWorldTimerManager().IsTimerActive(mTimerHandle_SpawnTimer)) // Deploy early if drop button is pushed again
	{
		FTimerDelegate timerDelegate;
		timerDelegate.BindUFunction(this, FName("EjectPlayer"), true);
		GetWorldTimerManager().SetTimer(mTimerHandle_SpawnTimer, timerDelegate, .5f, false);
	}
}

void APlayerDroppod::UpdateMeshColours(const FLinearColor& newColour)
{
	for (auto id : MeshMIDs)
	{
		id->SetVectorParameterValue(TEXT("Drop_Player_Colour"), newColour);
	}
}

//************************************
// Method:    OnPodHit
// FullName:  APlayerDroppod::OnPodHit
// Access:    protected 
// Returns:   void
// Qualifier:
// Parameter: UPrimitiveComponent * hitComponent
// Parameter: AActor * otherActor
// Parameter: UPrimitiveComponent * otherComp
// Parameter: FVector normalImpulse
// Parameter: const FHitResult & hit
//************************************
void APlayerDroppod::OnPodStopMoving_Implementation(const FHitResult& hit)
{
	if (!mMadeImpact)
	{
		mMadeImpact = true;
		GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Overlap);
		GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
		mDigTargetLocation = GetActorLocation() + ((-hit.ImpactNormal) * DigTargetDepth);

		if (OnImpactSound)
		{
			UAkGameplayStatics::PostEvent(OnImpactSound, this);
		}

		FTimerDelegate timerDelegate;
		timerDelegate.BindUFunction(this, FName("EjectPlayer"), false);
		GetWorldTimerManager().SetTimer(mTimerHandle_SpawnTimer, timerDelegate, .5f, false);
	}
}

//************************************
// Method:    OnPodOverlap
// FullName:  APlayerDroppod::OnPodOverlap
// Access:    protected 
// Returns:   void
// Qualifier:
// Parameter: UPrimitiveComponent * overlappedComponent
// Parameter: AActor * otherActor
// Parameter: UPrimitiveComponent * otherComp
// Parameter: int32 otherBodyIndex
// Parameter: bool fromSweep
// Parameter: const FHitResult & sweepResult
//************************************
void APlayerDroppod::OnPodOverlap_Implementation(UPrimitiveComponent* overlappedComponent, AActor* otherActor, UPrimitiveComponent* otherComp, int32 otherBodyIndex, bool fromSweep, const FHitResult& sweepResult)
{
	if (!mMadeImpact)
	{
		if (otherActor)
		{
			FHitResult hit;
			FCollisionQueryParams params;
			const bool didHit = GetWorld()->SweepSingleByObjectType(hit, GetActorLocation(), otherActor->GetActorLocation(), FQuat::Identity, ECC_Pawn, FCollisionShape::MakeSphere(20.f), params);
			if (didHit)
			{
				FVertPointDamageEvent PointDmg;
				PointDmg.DamageTypeClass = ImpactDamageType;
				PointDmg.HitInfo = hit;
				PointDmg.ShotDirection = ((-hit.ImpactNormal) + FVector::UpVector).GetSafeNormal();
				PointDmg.Damage = Damage;
				PointDmg.Knockback = Knockback;
				PointDmg.KnockbackScaling = KnockbackScaling;

				otherActor->TakeDamage(PointDmg.Damage, PointDmg, mVertPlayerController.IsValid() ? mVertPlayerController.Get() : nullptr, this);
			}
		}
	}
}

//************************************
// Method:    EjectPlayer
// FullName:  APlayerDroppod::EjectPlayer
// Access:    public 
// Returns:   void
// Qualifier:
//************************************
void APlayerDroppod::EjectPlayer(bool Forced)
{
	if (!mEjectedPayload)
	{
		mEjectedPayload = true;

		if (EjectAnimation)
		{
			GetMesh()->PlayAnimation(EjectAnimation, false);
		}

		AVertGameMode* VertGM = GetWorld()->GetAuthGameMode<AVertGameMode>();
		if (VertGM && GetController())
		{
			FVector SpawnLocation(GetActorLocation().X, GetActorLocation().Y, GetActorLocation().Z + GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + DigTargetDepth);
			FTransform SpawnTransform(FQuat::Identity.Rotator(), SpawnLocation);
			
			AVertCharacter* Character = Cast<AVertCharacter>(VertGM->SpawnDefaultPawnAtTransform(GetController(), SpawnTransform));
			if (Character)
			{
				GetController()->Possess(Character);
				Character->GetVertCharacterMovement()->Velocity.Z = Forced ? ForcedLaunchSpeed : NaturalLaunchSpeed;
			}
		}

		StartDespawnTimer();
	}
}

//************************************
// Method:    OnRep_PlayerState
// FullName:  APlayerDroppod::OnRep_PlayerState
// Access:    virtual public 
// Returns:   void
// Qualifier:
//************************************
void APlayerDroppod::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

	// [client] as soon as PlayerState is assigned, set team colors of this pawn for local player
	if (PlayerState != NULL)
	{
		UpdateTeamColoursAllMIDs();
	}
}

//************************************
// Method:    UpdateTeamColoursAllMIDs
// FullName:  APlayerDroppod::UpdateTeamColoursAllMIDs
// Access:    public 
// Returns:   void
// Qualifier:
//************************************
void APlayerDroppod::UpdateTeamColoursAllMIDs()
{
	for (int32 i = 0; i < MeshMIDs.Num(); ++i)
	{
		UpdateTeamColours(MeshMIDs[i]);
	}
}

//************************************
// Method:    UpdateTeamColours
// FullName:  APlayerDroppod::UpdateTeamColours
// Access:    protected 
// Returns:   void
// Qualifier:
// Parameter: UMaterialInstanceDynamic * useMIDs
//************************************
void APlayerDroppod::UpdateTeamColours(UMaterialInstanceDynamic* useMID)
{
	AVertPlayerState* playerState = Cast<AVertPlayerState>(PlayerState);

	if (playerState != NULL)
	{
		if (useMID)
		{
			useMID->SetVectorParameterValue(TEXT("Drop_Player_Colour"), playerState->GetPlayerColours().PrimaryColour);
		}

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
// Method:    StartDespawnTimer
// FullName:  APlayerDroppod::StartDespawnTimer
// Access:    protected 
// Returns:   void
// Qualifier:
//************************************
void APlayerDroppod::StartDespawnTimer()
{
	GetWorldTimerManager().SetTimer(mTimerHandle_Despawn, this, &APlayerDroppod::PrepareForDespawn, DespawnTriggerTime, false);
}

//************************************
// Method:    PrepareForDespawn
// FullName:  APlayerDroppod::PrepareForDespawn
// Access:    protected 
// Returns:   void
// Qualifier:
//************************************
void APlayerDroppod::PrepareForDespawn()
{
	GetWorldTimerManager().SetTimer(mTimerHandle_DespawnFlash, this, &APlayerDroppod::DespawnFlash, FlashSpeed, true);
	GetWorldTimerManager().SetTimer(mTimerHandle_DespawnFinish, this, &APlayerDroppod::DisableAndDestroy, FlashForTimeBeforeDespawning, false);
}

//************************************
// Method:    DespawnFlash
// FullName:  APlayerDroppod::DespawnFlash
// Access:    protected 
// Returns:   void
// Qualifier:
//************************************
void APlayerDroppod::DespawnFlash()
{
	if (GetMesh()->IsVisible())
		GetMesh()->SetVisibility(false, true);
	else
		GetMesh()->SetVisibility(true, true);
}

//************************************
// Method:    CancelDespawn
// FullName:  APlayerDroppod::CancelDespawn
// Access:    protected 
// Returns:   void
// Qualifier:
//************************************
void APlayerDroppod::CancelDespawn()
{
	if (GetWorldTimerManager().IsTimerActive(mTimerHandle_Despawn))
		GetWorldTimerManager().ClearTimer(mTimerHandle_Despawn);

	if (GetWorldTimerManager().IsTimerActive(mTimerHandle_DespawnFlash))
		GetWorldTimerManager().ClearTimer(mTimerHandle_DespawnFlash);

	if (GetWorldTimerManager().IsTimerActive(mTimerHandle_DespawnFinish))
		GetWorldTimerManager().ClearTimer(mTimerHandle_DespawnFinish);

	GetMesh()->SetVisibility(true, true);
}

//************************************
// Method:    DisableAndDestroy
// FullName:  APlayerDroppod::DisableAndDestroy
// Access:    protected 
// Returns:   void
// Qualifier:
//************************************
void APlayerDroppod::DisableAndDestroy()
{
	if (DespawnFX)
	{
		UParticleSystemComponent* psc = UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), DespawnFX, GetActorTransform());
	}

	Destroy();
}

//************************************
// Method:    TraceForForwardLedge
// FullName:  APlayerDroppod::TraceForwardForObstacle
// Access:    private 
// Returns:   bool
// Qualifier:
// Parameter: FHitResult & hit
//************************************
bool APlayerDroppod::TraceForwardForObstacle(FHitResult& hit)
{
	FVector forward = Controller ? Controller->GetControlRotation().Vector() : FRotationMatrix(GetActorRotation()).GetScaledAxis(EAxis::X);

	FVector start = GetActorLocation();
	FVector end = start + (forward * FVector(ObstacleTracing.ForwardRange, ObstacleTracing.ForwardRange, 0.f));

	return PerformObstacleTrace(start, end, hit);
}

//************************************
// Method:    TraceForUpwardLedge
// FullName:  APlayerDroppod::TraceUpwardsForObstacle
// Access:    private 
// Returns:   bool
// Qualifier:
// Parameter: FHitResult & hit
//************************************
bool APlayerDroppod::TraceUpwardsForObstacle(FHitResult& hit)
{
	FVector forward = Controller ? Controller->GetControlRotation().Vector() : FRotationMatrix(GetActorRotation()).GetScaledAxis(EAxis::X);

	FVector start = FVector(GetActorLocation().X, GetActorLocation().Y, GetActorLocation().Z - ObstacleTracing.UpwardTraceZStartOffset) + (forward * ObstacleTracing.UpwardTraceForwardOffset);
	FVector end = FVector(start.X, start.Y, GetActorLocation().Z);

	return PerformObstacleTrace(start, end, hit);
}

//************************************
// Method:    PerformLedgeTrace
// FullName:  APlayerDroppod::PerformObstacleTrace
// Access:    private 
// Returns:   bool
// Qualifier:
// Parameter: const FVector & start
// Parameter: const FVector & end
// Parameter: FHitResult & hit
//************************************
bool APlayerDroppod::PerformObstacleTrace(const FVector& start, const FVector& end, FHitResult& hit)
{
	static const FName scObstacleTrace(TEXT("PodObstacleTrace"));

	FCollisionQueryParams params(scObstacleTrace, true);
	params.bReturnPhysicalMaterial = true;
	params.bTraceAsyncScene = false;
	params.bFindInitialOverlaps = true;
	params.AddIgnoredActor(this);

	UWorld* world = GetWorld();
	const bool foundHit = world->SweepSingleByChannel(hit, start, end, FQuat::Identity, ObstacleTracing.TraceChannel, FCollisionShape::MakeSphere(ObstacleTracing.TraceRadius), params);

#if ENABLE_DRAW_DEBUG
	if (ObstacleTracing.ShowDebug)
	{
		if (foundHit && hit.bBlockingHit)
		{
			// Red up to the blocking hit, green thereafter
			UVertUtilities::DrawDebugSweptSphere(world, start, hit.Location, ObstacleTracing.TraceRadius, FColor::Red, false);
			UVertUtilities::DrawDebugSweptSphere(world, hit.Location, end, ObstacleTracing.TraceRadius, FColor::Green, false);
			DrawDebugPoint(world, hit.ImpactPoint, 16.f, FColor::Red, false);
		}
		else
		{
			// no hit means all red
			UVertUtilities::DrawDebugSweptSphere(world, start, end, ObstacleTracing.TraceRadius, FColor::Red, false);
		}
	}
#endif

	return foundHit && !hit.bStartPenetrating; // if the trace started in penetration then it's too high to navigate
}