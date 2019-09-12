// Copyright Inside Out Games Ltd. 2017

#include "GrapplingComponent.h"
#include "CableComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "VertCharacterBase.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystemComponent.h"
#include "AkAudio/Classes/AkGameplayStatics.h"

DECLARE_LOG_CATEGORY_CLASS(LogGrapplingComponent, Log, All);

UGrapplingComponent::UGrapplingComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
}

// Called when the game starts
void UGrapplingComponent::BeginPlay()
{
	Super::BeginPlay();

	mCharacterOwner = Cast<AVertCharacterBase>(GetOwner());
	if (!mCharacterOwner.IsValid())
		UE_LOG(LogGrapplingComponent, Warning, TEXT("Owner [%s] of GrapplingComponent is not an AVertCharacterBase, component functionality will be limited."), *GetOwner()->GetName());

	mRemainingGrapples = GrappleConfig.MaxGrapples;
	mRechargeTimer.BindAlarm(this, TEXT("OnGrappleRechargeTimerFinished"));

	if (ACharacter* owner = Cast<ACharacter>(GetOwner()))
	{
		// Previously from AGrappleHook
		// Use a sphere as a simple collision representation
		SphereComponent = NewObject<USphereComponent>(owner, TEXT("HookCollision"));
		SphereComponent->InitSphereRadius(10.f);
		SphereComponent->BodyInstance.SetCollisionProfileName("GrappleHook");
		SphereComponent->OnComponentHit.AddDynamic(this, &UGrapplingComponent::OnHit);
		SphereComponent->OnComponentBeginOverlap.AddDynamic(this, &UGrapplingComponent::OnBeginOverlap);
		SphereComponent->SetCollisionObjectType(ECC_Grappler);
		SphereComponent->SetCollisionResponseToChannel(ECC_Grappler, ECR_Ignore);
		SphereComponent->SetCollisionResponseToChannel(ECC_GrappleTrace, ECR_Block);
		SphereComponent->SetWalkableSlopeOverride(FWalkableSlopeOverride(WalkableSlope_Unwalkable, 0.f));
		SphereComponent->CanCharacterStepUpOn = ECB_No;
		SphereComponent->SetupAttachment(this);
		SphereComponent->RegisterComponent();

		MeshComponent = NewObject<USkeletalMeshComponent>(owner, TEXT("HookMesh"));
		MeshComponent->SetupAttachment(SphereComponent);
		MeshComponent->RegisterComponent();

		if (GrappleBeamFX && !GrappleBeamPSC)
		{
			GrappleBeamPSC = UGameplayStatics::SpawnEmitterAttached(GrappleBeamFX, this, NAME_None);
			if (GrappleBeamPSC)
			{
				GrappleBeamPSC->SetVectorParameter("BeamColour", FVector(0, 0.5f, 1.f));
				GrappleBeamPSC->SetVisibility(false);
			}
		}
	}
	else
	{
		UE_LOG(LogGrapplingComponent, Error, TEXT("Unable to initialize GrapplingComponent, owner [%s] is not a Character"), *GetOwner()->GetName());
	}
}

// Called every frame
void UGrapplingComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (GrappleBeamPSC && GrappleBeamPSC->IsVisible())
	{
		GrappleBeamPSC->SetVectorParameter("BeamEnd", SphereComponent->GetComponentLocation());
	}

	UpdateRechargeState();
	mRechargeTimer.TickTimer(DeltaTime);

	switch (mGrappleState)
	{
	case EGrappleState::HookReturning:
		TickReel(DeltaTime);
		break;

	case EGrappleState::HookLaunching:
	{
		if (IsLineDepleted(DeltaTime))
			StartReeling();
	}		
		break;

	case EGrappleState::HookDeployedAndReturning:
		TickHookDeployedAndReturning(DeltaTime);
		break;

	case EGrappleState::HookDeployed:
		TickHookDeployed(DeltaTime);
		break;
	}
}

//************************************
// Method:    SetGrappleColour
// FullName:  UGrapplingComponent::SetGrappleColour
// Access:    public 
// Returns:   void
// Qualifier:
// Parameter: const FLinearColor & newColour
//************************************
void UGrapplingComponent::SetGrappleColour(const FLinearColor& newColour)
{
	if (GrappleBeamPSC)
	{
		GrappleBeamPSC->SetVectorParameter(TEXT("BeamColour"), FVector(newColour.R, newColour.G, newColour.B));
	}
}

void UGrapplingComponent::DetachProjectile()
{
	if (AnchorActor && !AnchorActor->IsPendingKill())
	{
		AnchorActor->Destroy();
		AnchorActor = nullptr;
	}

	SphereComponent->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
	AnchorActor = UGameplayStatics::BeginDeferredActorSpawnFromClass(this, AActor::StaticClass(), SphereComponent->GetComponentTransform(), ESpawnActorCollisionHandlingMethod::AlwaysSpawn, GetOwner());
	if (AnchorActor)
	{
		UGameplayStatics::FinishSpawningActor(AnchorActor, SphereComponent->GetComponentTransform());
		RecreateProjectileVelocity(AnchorActor);
	}
}

//************************************
// Method:    ReattachProjectile
// FullName:  UGrapplingComponent::ReattachProjectile
// Access:    protected 
// Returns:   void
// Qualifier:
//************************************
void UGrapplingComponent::ReattachProjectile()
{
	SphereComponent->AttachToComponent(this, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	SphereComponent->SetRelativeLocation(FVector::ZeroVector);
	if (ProjectileComponent && !ProjectileComponent->IsPendingKill())
	{
		ProjectileComponent->DestroyComponent();
		ProjectileComponent = nullptr;
	}
}

//************************************
// Method:    RecreateProjectileVelocity
// FullName:  UGrapplingComponent::RecreateProjectileVelocity
// Access:    protected 
// Returns:   UProjectileMovementComponent*
// Qualifier:
// Parameter: AActor * owner
//************************************
UProjectileMovementComponent* UGrapplingComponent::RecreateProjectileVelocity(AActor* owner)
{
	if (ProjectileComponent && !ProjectileComponent->IsPendingKill())
	{
		ProjectileComponent->DestroyComponent();
		ProjectileComponent = nullptr;
	}		

	ProjectileComponent = NewObject<UProjectileMovementComponent>(owner, TEXT("HookMovement"));
	ProjectileComponent->UpdatedComponent = SphereComponent;
	ProjectileComponent->InitialSpeed = 0.f;
	ProjectileComponent->ProjectileGravityScale = 0.f;
	ProjectileComponent->MaxSpeed = 3000.f;
	ProjectileComponent->bRotationFollowsVelocity = true;
	ProjectileComponent->bShouldBounce = false;
	ProjectileComponent->bInitialVelocityInLocalSpace = false;
	ProjectileComponent->RegisterComponent();

	return ProjectileComponent;
}

//************************************
// Method:    DisableAndDestroyAnchor
// FullName:  UGrapplingComponent::DisableAndDestroyAnchor
// Access:    protected 
// Returns:   void
// Qualifier:
//************************************
void UGrapplingComponent::DisableAndDestroyAnchor()
{
	if (ProjectileComponent && !ProjectileComponent->IsPendingKill())
	{
		ProjectileComponent->StopMovementImmediately();
	}

	if (AnchorActor && !AnchorActor->IsPendingKill())
	{
		AnchorActor->Destroy();
		AnchorActor = nullptr;
		ProjectileComponent = nullptr;
	}
}

//************************************
// Method:    ExecuteGrapple
// FullName:  UGrapplingComponent::ExecuteGrapple
// Access:    public 
// Returns:   bool
// Qualifier:
// Parameter: FVector & aimDirection
//************************************
bool UGrapplingComponent::ExecuteGrapple(FVector& aimDirection)
{
	if (mGrappleState == EGrappleState::HookSheathed && mRemainingGrapples > 0 || GrappleConfig.MaxGrapples == -1)
	{
		aimDirection = UVertUtilities::LimitAimTrajectory(GrappleConfig.AimFreedom, aimDirection);

		if (SnapGrappleAim)
		{
			aimDirection = FindBestGrappleLocation(aimDirection);
		}

		mGrappleState = EGrappleState::HookLaunching;
		DetachProjectile();

		ActivateHookCollision();
		if(ProjectileComponent)
			ProjectileComponent->Velocity = aimDirection * GrappleConfig.LaunchSpeed;

		if (GrappleBeamPSC)
		{
			GrappleBeamPSC->SetVectorParameter("BeamEnd", SphereComponent->GetComponentLocation());
			GrappleBeamPSC->SetVisibility(true);
		}

		OnFired.Broadcast();
		mRemainingGrapples = FMath::Max(0, mRemainingGrapples - 1);

		if (FireSound)
		{
			UAkGameplayStatics::PostEvent(FireSound, GetOwner(), false);
		}

		return true;
	}

	return false;
}

//************************************
// Method:    FindBestGrappleLocation
// FullName:  UGrapplingComponent::FindBestGrappleLocation
// Access:    private 
// Returns:   FVector
// Qualifier:
// Parameter: const FVector & projectedAim
//************************************
FVector UGrapplingComponent::FindBestGrappleLocation(const FVector& projectedAim)
{
	static FName sGrappleLedgeTraceTag = FName(TEXT("GrappleLedgeTrace"));
	FVector traceStart = GetComponentLocation();	
	FVector traceEnd = traceStart + (projectedAim*GrappleConfig.MaxLineLength);

	// Perform trace to retrieve hit info
	FCollisionQueryParams params(sGrappleLedgeTraceTag, false, GetOwner());
	params.bTraceAsyncScene = false;
	params.bReturnPhysicalMaterial = false;

	TArray<FHitResult> hits;
	const bool didHit = GetWorld()->SweepMultiByChannel(hits, traceStart, traceEnd, FQuat::Identity, TraceChannel, FCollisionShape::MakeSphere(TraceRadius), params);

	if (ShowTraceDebug)
	{
		UE_LOG(LogGrapplingComponent, Log, TEXT("---------------- Hit results ----------------"));
		int32 index = 0;
		for (auto hit : hits)
		{
			UE_LOG(LogGrapplingComponent, Log, TEXT("hit #%i:"), index);
			UE_LOG(LogGrapplingComponent, Log, TEXT(" - Hit Actor: %s"), hit.Actor.IsValid() ? *hit.Actor->GetName() : TEXT("NULL"));
			UE_LOG(LogGrapplingComponent, Log, TEXT(" - Hit Component: %s"), *hit.Component->GetName());
			UE_LOG(LogGrapplingComponent, Log, TEXT(" - Impact Point: %f, %f, %f"), hit.ImpactPoint.X, hit.ImpactPoint.Y, hit.ImpactPoint.Z);
			UE_LOG(LogGrapplingComponent, Log, TEXT(" - Location: %f, %f, %f"), hit.Location.X, hit.Location.Y, hit.Location.Z);
			index += 1;
		}
#if ENABLE_DRAW_DEBUG
		UVertUtilities::DrawDebugSphereTraceMulti(GetWorld(), traceStart, traceEnd, TraceRadius, EDrawDebugTrace::ForDuration, didHit, hits, FLinearColor::Green, FLinearColor::Red, 5.f);
#endif
	}

	if (didHit || hits.Num() > 0)
	{
		float bestMatch = 0;
		int32 bestIndex = 0;
		bool foundMatch = false;
		FGrappledLedgeData bestLedgeData;
		FVector bestDirection = FVector::ZeroVector;
		for (int32 i = 0; i < hits.Num(); ++i)
		{
			// Check if this potential hooking point is blocked by geometry
			FHitResult blocker;
			FCollisionQueryParams blockParams(sGrappleLedgeTraceTag, false, GetOwner());
			const bool blocking = GetWorld()->LineTraceSingleByChannel(blocker, traceStart, hits[i].ImpactPoint, ECC_Visibility, blockParams);

			if (FMath::Square(GrappleConfig.MaxLineLength) < (hits[i].ImpactPoint - GetComponentLocation()).SizeSquared())
				continue;

			// if nothing is blocking, find the angle between here and the initial aim direction.
			if (!blocking)
			{
				FGrappledLedgeData ledgeData;
				FVector thisDirection = hits[i].ImpactPoint - GetComponentLocation();
				if (thisDirection == FVector::ZeroVector)
					continue;

				thisDirection = thisDirection.GetSafeNormal();
				ledgeData.ForwardHit = hits[i];

				TArray<AActor*> ignoreActors;
				TArray<UPrimitiveComponent*> ignoreComponents;
				FindIgnoreActorsAndComponents(hits, ignoreActors, ignoreComponents, hits[i].Actor.Get(), hits[i].Component.Get());
				ledgeData.IsLedge = FindLedgeFromTraceResults(ledgeData, thisDirection, hits[i], ignoreActors, ignoreComponents);

				if (!ledgeData.IsLedge)
				{
					thisDirection = hits[i].Component->GetComponentLocation() - GetComponentLocation();
					thisDirection = thisDirection.GetSafeNormal();
				}

				float cos = FVector::DotProduct(thisDirection, projectedAim);
				float rads = FMath::Acos(cos);
				float angle = FMath::RadiansToDegrees(rads);

				if (!foundMatch || angle < bestMatch)
				{
					bestMatch = angle;
					bestIndex = i;
					bestDirection = thisDirection;
					foundMatch = true;
					bestLedgeData = ledgeData;
				}
			}

#if ENABLE_DRAW_DEBUG
			if (ShowTraceDebug)
			{
				UWorld* world = GetWorld();
				DrawDebugLine(world, traceStart, hits[i].ImpactPoint, (blocking) ? FColor::Purple : FColor::Orange, false, 5.f, 100, 10.f);
			}
#endif
		}

		if (ShowTraceDebug)
		{
#if ENABLE_DRAW_DEBUG
			UWorld* world = GetWorld();
			DrawDebugLine(world, traceStart, traceEnd, FColor::Blue, false, 5.f);
#endif
			if (foundMatch)
			{
				UE_LOG(LogGrapplingComponent, Log, TEXT("Match found, actor [%s] with component [%s]."), (hits[bestIndex].Actor.IsValid()) ? *hits[bestIndex].Actor->GetName() : TEXT("NULL"), *hits[bestIndex].Component->GetName());
			}
			else
			{
				UE_LOG(LogGrapplingComponent, Log, TEXT("No appropriate match found, returning original direction."));
			}
		}
		
		if (foundMatch)
		{
			mLedgeData = bestLedgeData;
			return bestDirection;
		}
	}

	mLedgeData.IsLedge = false; // reset data if no ledge was hit
	return projectedAim;
}

//************************************
// Method:    FindIgnoreActorsAndComponents
// FullName:  UGrapplingComponent::FindIgnoreActorsAndComponents
// Access:    private 
// Returns:   void
// Qualifier:
// Parameter: const TArray<FHitResult> & hitResults
// Parameter: TArray<AActor * > & outActors
// Parameter: TArray<UPrimitiveComponent * > & outComponents
// Parameter: const AActor * excludeActor
// Parameter: const UPrimitiveComponent * excludeComponent
//************************************
void UGrapplingComponent::FindIgnoreActorsAndComponents(const TArray<FHitResult>& hitResults, TArray<AActor*>& outActors, TArray<UPrimitiveComponent*>& outComponents, const AActor* excludeActor /*= nullptr*/, const UPrimitiveComponent* excludeComponent /*= nullptr*/)
{
	for (auto hit : hitResults)
	{
		if (hit.Actor.IsValid() && hit.Actor != excludeActor)
		{
			outActors.Add(hit.Actor.Get());
		}
		if (hit.Component.IsValid() && hit.Component != excludeComponent)
		{
			outComponents.Add(hit.Component.Get());
		}
	}
}

//************************************
// Method:    FindLedgeFromTraceResults
// FullName:  UGrapplingComponent::FindLedgeFromTraceResults
// Access:    private 
// Returns:   bool
// Qualifier:
// Parameter: FVector & projectedAim
// Parameter: const FHitResult & hit
//************************************
bool UGrapplingComponent::FindLedgeFromTraceResults(FGrappledLedgeData& outLedgeData, FVector& outProjectedAim, const FHitResult& hit, const TArray<AActor*>& ignoreActors, const TArray<UPrimitiveComponent*>& ignoreComponents)
{
	static const FName sGrappleLedgeTrace(TEXT("GrappleLedgeTrace"));
	static constexpr float scTraceRadius = 20.f;
	static constexpr float scGeoTraceRadius = 10.f;
	static constexpr float scDownwardTraceZOffset = 150.f;

	//if (FMath::Abs(FVector::DotProduct(hit.ImpactNormal, FVector::UpVector)) > 0.55f)
	//	return false;

	FVector traceStart(hit.ImpactPoint.X, hit.ImpactPoint.Y, hit.ImpactPoint.Z + scDownwardTraceZOffset);
	FVector traceEnd = hit.ImpactPoint;

	FCollisionQueryParams params(sGrappleLedgeTrace, false);
	params.bReturnPhysicalMaterial = false;
	params.bTraceAsyncScene = false;
	params.bFindInitialOverlaps = false;
	params.AddIgnoredActors(ignoreActors);
	params.AddIgnoredComponents(ignoreComponents);
	params.AddIgnoredActor(GetOwner());

	UWorld* world = GetWorld();
	FHitResult downwardHit;
	FHitResult geometryHit;
	const bool foundHit = world->SweepSingleByChannel(downwardHit, traceStart, traceEnd, FQuat::Identity, ECC_LedgeTracer, FCollisionShape::MakeSphere(scTraceRadius), params);

	if (foundHit)
	{
		FCollisionQueryParams geometryParams(sGrappleLedgeTrace, false, GetOwner());
		FVector geometryTraceEnd(downwardHit.ImpactPoint.X, downwardHit.ImpactPoint.Y, downwardHit.ImpactPoint.Z - scDownwardTraceZOffset);
		const bool foundGeometryHit = world->SweepSingleByChannel(geometryHit, downwardHit.ImpactPoint, geometryTraceEnd, FQuat::Identity, ECC_Visibility, FCollisionShape::MakeSphere(scGeoTraceRadius), geometryParams);

#if ENABLE_DRAW_DEBUG
		if (ShowTraceDebug)
		{
			if (foundGeometryHit && geometryHit.bBlockingHit)
			{
				// Red up to the blocking hit, green thereafter
				UVertUtilities::DrawDebugSweptSphere(world, downwardHit.ImpactPoint, geometryHit.Location, scGeoTraceRadius, FColor::Orange, false, 5.f);
				UVertUtilities::DrawDebugSweptSphere(world, geometryHit.Location, geometryTraceEnd, scGeoTraceRadius, FColor::Emerald, false, 5.f);
				DrawDebugPoint(world, downwardHit.ImpactPoint, 16.f, FColor::Red, false);
			}
			else
			{
				// no hit means all red
				UVertUtilities::DrawDebugSweptSphere(world, downwardHit.ImpactPoint, geometryTraceEnd, scGeoTraceRadius, FColor::Orange, false, 5.f);
			}
		}
#endif
		static constexpr float scLedgeVerticalModifier = 5.f;

		FVector& actualImpact = (foundGeometryHit) ? geometryHit.ImpactPoint : downwardHit.ImpactPoint;
		actualImpact.Z -= scLedgeVerticalModifier;
		outProjectedAim = actualImpact - GetComponentLocation();
		outProjectedAim = outProjectedAim.GetSafeNormal();
		outLedgeData.IsLedge = true;
		outLedgeData.DownwardHit = /*(foundGeometryHit) ? geometryHit : */downwardHit;
	}

#if ENABLE_DRAW_DEBUG
	if (ShowTraceDebug)
	{
		if (foundHit && downwardHit.bBlockingHit)
		{
			// Red up to the blocking hit, green thereafter
			UVertUtilities::DrawDebugSweptSphere(world, traceStart, downwardHit.Location, scTraceRadius, FColor::Red, false, 5.f);
			UVertUtilities::DrawDebugSweptSphere(world, downwardHit.Location, traceEnd, scTraceRadius, FColor::Green, false, 5.f);
			DrawDebugPoint(world, downwardHit.ImpactPoint, 16.f, FColor::Red, false);
		}
		else
		{
			// no hit means all red
			UVertUtilities::DrawDebugSweptSphere(world, traceStart, traceEnd, scTraceRadius, FColor::Red, false, 5.f);
		}
	}
#endif

	return foundHit;
}

//************************************
// Method:    OnLanded
// FullName:  UGrapplingComponent::OnLanded
// Access:    public 
// Returns:   void
// Qualifier:
//************************************
void UGrapplingComponent::OnLanded()
{
	if (GrappleConfig.RecieveChargeOnGroundOnly)
	{
		mRemainingGrapples += mRechargeTimer.PopAlarmBacklog();
	}
}

//************************************
// Method:    UpdateRechargeSate
// FullName:  UGrapplingComponent::UpdateRechargeSate
// Access:    private 
// Returns:   void
// Qualifier:
//************************************
void UGrapplingComponent::UpdateRechargeState()
{
	if (mCharacterOwner.IsValid() && mRemainingGrapples < GrappleConfig.MaxGrapples && mRechargeTimer.GetAlarmBacklog() < (GrappleConfig.MaxGrapples - mRemainingGrapples))
	{
		(mCharacterOwner->CanComponentRecharge(GrappleConfig.RechargeMode))
			? mRechargeTimer.Start()
			: mRechargeTimer.Stop();
	}
}

//************************************
// Method:    Reset
// FullName:  UGrapplingComponent::Reset
// Access:    public 
// Returns:   bool
// Qualifier:
//************************************
bool UGrapplingComponent::Reset()
{
	if (mGrappleState == EGrappleState::HookDeployed || mGrappleState == EGrappleState::HookDeployedAndReturning)
	{
		mLedgeData.IsLedge = false;
		DetatchHook();
		return true;
	}

	return false;
}

//************************************
// Method:    StartPulling
// FullName:  UGrapplingComponent::StartPulling
// Access:    public 
// Returns:   bool
// Qualifier:
//************************************
bool UGrapplingComponent::StartPulling()
{
	if (mGrappleState == EGrappleState::HookDeployed)
	{
		mGrappleState = EGrappleState::HookDeployedAndReturning;
		return true;
	}
	else if (mGrappleState == EGrappleState::HookDeployedAndReturning)
	{
		Reset();
	}
	UE_LOG(LogGrapplingComponent, Warning, TEXT("Could not start pulling, not currently hooked."));

	return false;
}

//************************************
// Method:    OnGrappleRechargeTimerFinished_Implementation
// FullName:  UGrapplingComponent::OnGrappleRechargeTimerFinished_Implementation
// Access:    public 
// Returns:   void
// Qualifier:
//************************************
void UGrapplingComponent::OnGrappleRechargeTimerFinished_Implementation()
{
	if (!GrappleConfig.RecieveChargeOnGroundOnly || (!mCharacterOwner.IsValid() || mCharacterOwner->IsGrounded()))
	{
		mRemainingGrapples += mRechargeTimer.PopAlarmBacklog();
		if (!mCharacterOwner.IsValid())
			UE_LOG(LogGrapplingComponent, Warning, TEXT("Component has not AVertCharacterBase parent, grapple recharge may be innaccurate."));
	}
	mRechargeTimer.Reset();
}

//************************************
// Method:    ActivateHookCollision
// FullName:  UGrapplingComponent::ActivateHookCollision
// Access:    private 
// Returns:   void
// Qualifier:
//************************************
void UGrapplingComponent::ActivateHookCollision()
{
	SphereComponent->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
	SphereComponent->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
	SphereComponent->SetCollisionResponseToChannel(ECC_PhysicsBody, ECR_Block);
	SphereComponent->SetCollisionResponseToChannel(ECC_Vehicle, ECR_Block);
	SphereComponent->SetCollisionResponseToChannel(ECC_Destructible, ECR_Block);
	SphereComponent->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
}

//************************************
// Method:    DeactivateHookCollision
// FullName:  UGrapplingComponent::DeactivateHookCollision
// Access:    private 
// Returns:   void
// Qualifier:
//************************************
void UGrapplingComponent::DeactivateHookCollision()
{
	SphereComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
}

//************************************
// Method:    StartReeling
// FullName:  UGrapplingComponent::StartReeling
// Access:    private 
// Returns:   void
// Qualifier:
//************************************
void UGrapplingComponent::StartReeling()
{
	if (mGrappleState != EGrappleState::HookReturning && mGrappleState != EGrappleState::HookSheathed)
	{
		if (GrappleConfig.SkipReel)
		{
			SheatheHook();
		}
		else
		{
			mGrappleState = EGrappleState::HookReturning;
			DeactivateHookCollision();
			OnHookBreak.Broadcast();
		}		
	}
}

//************************************
// Method:    TickReel
// FullName:  UGrapplingComponent::TickReel
// Access:    private 
// Returns:   void
// Qualifier:
// Parameter: float DeltaSeconds
//************************************
void UGrapplingComponent::TickReel(float DeltaSeconds)
{
	FVector diff = GetComponentLocation() - SphereComponent->GetComponentLocation();
	FVector direction = diff.GetSafeNormal();
	float distance = DeltaSeconds * GrappleConfig.ReelSpeed;
	float sizeSquared = diff.SizeSquared();

	if (FMath::Square(distance) >= sizeSquared)
		SheatheHook();
	else if (ProjectileComponent)
		ProjectileComponent->Velocity = direction * GrappleConfig.ReelSpeed;
}

//************************************
// Method:    TickHookDeployed
// FullName:  UGrapplingComponent::TickHookDeployed
// Access:    private 
// Returns:   void
// Qualifier:
// Parameter: float DeltaSeconds
//************************************
void UGrapplingComponent::TickHookDeployed(float DeltaSeconds)
{	
	if (UsePhysicsForPull || UsePhysicsForSwing)
	{
		FVector difference = GetComponentLocation() - SphereComponent->GetComponentLocation();
		FVector direction = difference.GetSafeNormal();
		float actualLength = difference.Size();

		if ((PhysicsGrappleConfig.StringContraint && actualLength > mDistanceFromLauncher) || !PhysicsGrappleConfig.StringContraint)
		{
			if (mGrappleState == EGrappleState::HookDeployed && mCharacterOwner.IsValid())
			{
				FVector dampingVelocity = PhysicsGrappleConfig.LineDampingCoefficient * mCharacterOwner->GetVelocity();
				FVector springVelocity = PhysicsGrappleConfig.LineSpringCoefficient * (direction*(mDistanceFromLauncher - actualLength));
				FVector launchVelocity = springVelocity - dampingVelocity;
							
				mCharacterOwner->LaunchCharacter(launchVelocity, false, false);
			} else if (!mCharacterOwner.IsValid()) { UE_LOG(LogGrapplingComponent, Warning, TEXT("Cannot pull character associated with [%s] because it is invalid."), *GetName()); }
		}
	}

	CheckForGrappleBreak();
}

//************************************
// Method:    TickHookDeployedAndReturning
// FullName:  UGrapplingComponent::TickHookDeployedAndReturning
// Access:    private 
// Returns:   void
// Qualifier:
// Parameter: float DeltaSeconds
//************************************
void UGrapplingComponent::TickHookDeployedAndReturning(float DeltaSeconds)
{
	if (!UsePhysicsForPull)
	{
		FVector diff = SphereComponent->GetComponentLocation() - GetComponentLocation();
		FVector direction = diff.GetSafeNormal();

		float distance = DeltaSeconds * GrappleConfig.PullSpeed;
		FVector newLocation = diff - (distance * direction);

		if (newLocation.SizeSquared() <= FMath::Square(GrappleConfig.LineCutLength))
			DetatchHook();
		else
		{
			if (mCharacterOwner.IsValid())
			{
				mCharacterOwner->LaunchCharacter(GrappleConfig.PullSpeed * direction, true, true);
			}
			mDistanceFromLauncher -= (DeltaSeconds * GrappleConfig.PullSpeed);
		}

		CheckForGrappleBreak();
	}
	else
	{
		float distance = DeltaSeconds * GrappleConfig.PullSpeed;
		float newLength = mDistanceFromLauncher - distance;

		if (newLength <= GrappleConfig.LineCutLength)
		{
			Reset();
		}			
		else
		{
			mDistanceFromLauncher -= (DeltaSeconds * GrappleConfig.PullSpeed);
			TickHookDeployed(DeltaSeconds);
		}
	}
}

//************************************
// Method:    CheckForGrappleBreak
// FullName:  UGrapplingComponent::CheckForGrappleBreak
// Access:    private 
// Returns:   void
// Qualifier:
//************************************
void UGrapplingComponent::CheckForGrappleBreak()
{
	static const FName scGrappleBreakCheck(TEXT("GrappleBreakCheck"));

	FCollisionQueryParams params(scGrappleBreakCheck, false, GetOwner());
	FHitResult hit(ForceInit);
	const bool traceHit = GetWorld()->LineTraceSingleByChannel(hit, GetComponentLocation(), SphereComponent->GetComponentLocation(), ECC_Visibility, params);

	if (traceHit && (FMath::Square(hit.Distance) < ((GetComponentLocation() - SphereComponent->GetComponentLocation()).SizeSquared() - FMath::Square(GrappleConfig.BreakLeeway))))
	{
		if (GrappleConfig.EverCheckForBreak && 
			(!GrappleConfig.CheckForBreakOnlyIfPulling || mGrappleState == EGrappleState::HookDeployedAndReturning))
		{
			UE_LOG(LogGrapplingComponent, Log, TEXT("Grapple broke, geometry in the way."));
			Reset();
		}
		else
		{
			mGeometryBlockingGrab = true;
		}
	}
	else
	{
		mGeometryBlockingGrab = false;
	}
}

//************************************
// Method:    SheatheHook
// FullName:  UGrapplingComponent::SheatheHook
// Access:    private 
// Returns:   void
// Qualifier:
//************************************
void UGrapplingComponent::SheatheHook()
{
	mGrappleState = EGrappleState::HookSheathed;

	DisableAndDestroyAnchor();
	ReattachProjectile();

	GrappleBeamPSC->SetVisibility(false);

	if (SheathSound)
	{
		UAkGameplayStatics::PostEvent(SheathSound, GetOwner(), false);
	}

	OnReturned.Broadcast();
	mDistanceFromLauncher = 0.f;
}

//************************************
// Method:    DeployHook
// FullName:  UGrapplingComponent::DeployHook
// Access:    private 
// Returns:   void
// Qualifier:
// Parameter: AActor * OtherActor
// Parameter: UPrimitiveComponent * OtherComp
// Parameter: const FHitResult & hit
// Parameter: bool attachToTarget
//************************************
void UGrapplingComponent::DeployHook(AActor* OtherActor, UPrimitiveComponent* OtherComp, const FHitResult& hit, bool attachToTarget /*= true*/)
{
	mGrappleState = EGrappleState::HookDeployed;
	DisableAndDestroyAnchor();

	if (attachToTarget && OtherComp)
	{
		SphereComponent->AttachToComponent(OtherComp, FAttachmentTransformRules::KeepWorldTransform);
		UE_LOG(LogGrapplingComponent, Log, TEXT("Hook [%s] attached to object [%s]."), *GetName(), OtherActor ? *OtherActor->GetName() : *OtherComp->GetName());
	}

	mHookAttachment.Actor = OtherActor;
	mHookAttachment.Component = OtherComp;

	FVector diff = GetComponentLocation() - SphereComponent->GetComponentLocation();
	mDistanceFromLauncher = diff.Size();
		
	if (mCharacterOwner.IsValid())
	{
		if (mCharacterOwner->IsGrounded())
		{
			mCharacterOwner->GetCharacterMovement()->SetMovementMode(MOVE_Custom, static_cast<uint8>(ECustomMovementMode::MOVE_GrappleWalk));
		}
		else
		{
			mCharacterOwner->GetCharacterMovement()->SetMovementMode(MOVE_Custom, static_cast<uint8>(ECustomMovementMode::MOVE_GrappleFall));
		}
	}

	if (LatchSound)
	{
		UAkGameplayStatics::PostEvent(LatchSound, GetOwner(), false);
	}

	OnHooked.Broadcast();
}

//************************************
// Method:    DetatchHook
// FullName:  UGrapplingComponent::DetatchHook
// Access:    private 
// Returns:   void
// Qualifier:
//************************************
void UGrapplingComponent::DetatchHook()
{
	if (mGrappleState == EGrappleState::HookDeployed || mGrappleState == EGrappleState::HookDeployedAndReturning)
	{
		if (mHookAttachment.Actor.IsValid())
		{
			DetachProjectile();
		}

		mHookAttachment.Actor = nullptr;
		mHookAttachment.Component = nullptr;

		SphereComponent->Activate();

		OnHookReleased.Broadcast();
		StartReeling();
		if (mLedgeData.IsLedge && !mGeometryBlockingGrab)
		{
			OnGrappleToLedgeTransition.Broadcast(mLedgeData.ForwardHit, mLedgeData.DownwardHit);
		}		

		if (mCharacterOwner.IsValid() && mCharacterOwner->GetCharacterMovement()->MovementMode != MOVE_Flying)
		{
			mCharacterOwner->GetCharacterMovement()->SetMovementMode(MOVE_Falling);
		}

		if (UnlatchSound)
		{
			UAkGameplayStatics::PostEvent(UnlatchSound, GetOwner(), false);
		}
	}
}

//************************************
// Method:    IsLineDepleted
// FullName:  UGrapplingComponent::IsLineDepleted
// Access:    private 
// Returns:   bool
// Qualifier:
// Parameter: float DeltaSeconds
//************************************
bool UGrapplingComponent::IsLineDepleted(float DeltaSeconds)
{
	FVector diff = GetComponentLocation() - SphereComponent->GetComponentLocation();
	return diff.SizeSquared() >= FMath::Square(GrappleConfig.MaxLineLength);
}

//************************************
// Method:    OnHit_Implementation
// FullName:  UGrapplingComponent::OnHit_Implementation
// Access:    public 
// Returns:   void
// Qualifier:
// Parameter: class UPrimitiveComponent * HitComp
// Parameter: AActor * OtherActor
// Parameter: UPrimitiveComponent * OtherComp
// Parameter: FVector NormalImpulse
// Parameter: const FHitResult & Hit
//************************************
void UGrapplingComponent::OnHit_Implementation(class UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (mGrappleState == EGrappleState::HookLaunching)
	{
		DeployHook(OtherActor, OtherComp, Hit);
		SphereComponent->Deactivate();
	}
}

//************************************
// Method:    OnBeginOverlap_Implementation
// FullName:  UGrapplingComponent::OnBeginOverlap_Implementation
// Access:    public 
// Returns:   void
// Qualifier:
// Parameter: UPrimitiveComponent * overlappedComp
// Parameter: AActor * otherActor
// Parameter: UPrimitiveComponent * otherComp
// Parameter: int32 otherBodyIndex
// Parameter: bool fromSweep
// Parameter: const FHitResult & sweepResult
//************************************
void UGrapplingComponent::OnBeginOverlap_Implementation(UPrimitiveComponent* overlappedComp, AActor* otherActor, UPrimitiveComponent* otherComp, int32 otherBodyIndex, bool fromSweep, const FHitResult& sweepResult)
{
	// Possibly used for pulling players / weapons
}