/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////
///			Copyright 2017 (C) Bruno Xavier B. Leite
//////////////////////////////////////////////////////////////
/*
	BY EXECUTING, READING, EDITING, COPYING OR KEEPING FILES FROM THIS SOFTWARE SOURCE CODE,
	YOU AGREE TO THE FOLLOWING TERMS IN ADDITION TO EPIC GAMES MARKETPLACE EULA:
	- YOU HAVE READ AND AGREE TO EPIC GAMES TERMS: https://publish.unrealengine.com/faq
	- YOU AGREE DEVELOPER RESERVES ALL RIGHTS TO THE SOFTWARE PROVIDED, GRANTED BY LAW.
	- YOU AGREE YOU'LL NOT CREATE OR PUBLISH DERIVATIVE SOFTWARE TO THE MARKETPLACE.
	- YOU AGREE DEVELOPER WILL NOT PROVIDE SOFTWARE OUTSIDE MARKETPLACE ENVIRONMENT.
	- YOU AGREE DEVELOPER WILL NOT PROVIDE PAID OR EXCLUSIVE SUPPORT SERVICES.
	- YOU AGREE DEVELOPER PROVIDED SUPPORT CHANNELS, ARE UNDER HIS SOLE DISCRETION.
*/
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "PooledProjectile.h"
#include "OBJPoolShared.h"

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

UPooledProjectile::UPooledProjectile(const FObjectInitializer& OBJ) : Super(OBJ) {
	Movement = nullptr;
	//
	Direction = FVector::ZeroVector;
	InitialSpeed = 3000.0f;
	MaxSpeed = 3000.0f;
	//
	Friction = 0.2f;
	Bounciness = 0.6f;
	ForceSubStepping = false;
	ProjectileGravityScale = 1.f;
	MaxSimulationIterations = 8;
	UpdateOnlyIfRendered = false;
	MaxSimulationTimeStep = 0.05f;
	InitialVelocityInLocalSpace = true;
	BounceAngleAffectsFriction = false;
	HomingAccelerationMagnitude = 0.f;
	BounceVelocityStopSimulatingThreshold = 5.f;
}

void UPooledProjectile::PostLoad() {
	Super::PostLoad();
	//
	Owner = Cast<APooledActor>(GetOwner()); if (!Owner) {return;}
	//
	Owner->OnPoolBeginPlay.AddDynamic(this,&UPooledProjectile::Shoot);
	Owner->OnPoolEndPlay.AddDynamic(this,&UPooledProjectile::Break);
	//
	Owner->SetLifeSpan(0.f);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void UPooledProjectile::Shoot_Implementation() {
	if (!Primitive) {
		Owner = Cast<APooledActor>(GetOwner());
		Primitive = Cast<UPrimitiveComponent>(Owner->GetRootComponent());
		//
		if (!Owner||!Owner->OwningPool||!Primitive) {
			UE_LOG(LogTemp,Error,TEXT("{Pool}:: %s"),TEXT("Pooled Projectile Components are meant to be used by ''Pooled Actor'' Class... but casting have failed or Owning Pool Component is invalid!"));
	return;}}
	//
	if (Direction==FVector::ZeroVector) {Direction = FVector::ForwardVector;}
	//
	if (Movement) {
		Movement->StopSimulating(FHitResult::FHitResult());
		Movement->DestroyComponent();
	Movement = nullptr;}
	//
	Movement = NewObject<UProjectileMovementComponent>(Owner);
	if (Movement) {
		Movement->BounceVelocityStopSimulatingThreshold = BounceVelocityStopSimulatingThreshold;
		Movement->HomingAccelerationMagnitude = HomingAccelerationMagnitude;
		Movement->bBounceAngleAffectsFriction = BounceAngleAffectsFriction;
		Movement->bInitialVelocityInLocalSpace = InitialVelocityInLocalSpace;
		Movement->bRotationFollowsVelocity = RotationFollowsVelocity;
		Movement->MaxSimulationIterations = MaxSimulationIterations;
		Movement->MaxSimulationTimeStep = MaxSimulationTimeStep;
		Movement->bUpdateOnlyIfRendered = UpdateOnlyIfRendered;
		Movement->ProjectileGravityScale = ProjectileGravityScale;
		Movement->bIsHomingProjectile = bIsHomingProjectile;
		Movement->bForceSubStepping = ForceSubStepping;
		Movement->bShouldBounce = ShouldBounce;
		Movement->InitialSpeed = InitialSpeed;
		Movement->Bounciness = Bounciness;
		Movement->MaxSpeed = MaxSpeed;
		Movement->Velocity = Direction;
		Movement->Friction = Friction;
	Movement->RegisterComponent();}
}

void UPooledProjectile::Break_Implementation() {
	if ((!Owner||!Primitive||!Movement)||Owner->IsPendingKill()||!Owner->IsValidLowLevel()) {return;}
	//
	Primitive->SetPhysicsLinearVelocity(FVector::ZeroVector);
	if (Movement) {
		Movement->StopSimulating(FHitResult::FHitResult());
		Movement->DestroyComponent();
	}
}

UProjectileMovementComponent* UPooledProjectile::GetMovementComponent() const {
	return Movement;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////