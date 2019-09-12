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

#include "PooledActor.h"
#include "OBJPoolShared.h"
#include "Runtime/Engine/Classes/Particles/ParticleSystemComponent.h"

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

APooledActor::APooledActor(const FObjectInitializer &OBJ) : Super(OBJ) {
	LifeSpanPool = 0.f;
}

void APooledActor::Initialize() {
	const auto &Settings = GetMutableDefault<UPoolSettings>();
	//
	Spawned = false;
	SetActorHiddenInGame(true);
	SetActorEnableCollision(false);
	SetActorTickEnabled(false);
	SetLifeSpan(0.f);
	//
	TInlineComponentArray<UActorComponent*> Components;
	GetComponents(Components);
	//
	for (const auto &C : Components) {
		const auto &P = Cast<UPrimitiveComponent>(C);
		if (P) {
			P->SetSimulatePhysics(false);
			P->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			P->SetPhysicsAngularVelocity(FVector::ZeroVector);
			P->SetPhysicsLinearVelocity(FVector::ZeroVector);
			P->SetComponentTickEnabled(false);
			P->SetVisibility(false,true);
		} C->Deactivate();}
	//
	if (OwningPool && OwningPool->IsValidLowLevel()) {
		OwningPool->ReturnActor(const_cast<APooledActor*>(this));
	}
}

void APooledActor::BeginPlay() {
	if (LifeSpanPool>0.0001f) {
		const FTimerDelegate Timer = FTimerDelegate::CreateUObject(this,&APooledActor::ReturnToPool);
		GetWorld()->GetTimerManager().SetTimer(LifeSpanHandle,Timer,LifeSpanPool,false);
	} Super::BeginPlay();
}

void APooledActor::EndPlay(const EEndPlayReason::Type EndPlayReason) {
	OnPoolBeginPlay.Clear();
	OnPoolEndPlay.Clear();
	//
	Super::EndPlay(EndPlayReason);
}

void APooledActor::FellOutOfWorld(const class UDamageType& dmgType)
{
	// Attempt to return to pool when killed by a KillZVolume
	ReturnToPool();
}

void APooledActor::SpawnFromPool(const bool Reconstruct, const FPoolSpawnOptions &SpawnOptions, const FTransform &SpawnTransform) {
	if (!OwningPool||!OwningPool->IsValidLowLevel()) {
		UE_LOG(LogTemp,Warning,TEXT("{Pool}:: %s"),TEXT("Actor trying to spawn from Pool, but Owning Pool Component is invalid!"));
	return;} Spawned = true;
	//
	const auto &Settings = GetMutableDefault<UPoolSettings>();
	//
	TInlineComponentArray<UActorComponent*> Components;
	GetComponents(Components);
	//
	for (const auto &C : Components) {
		const auto &P = Cast<UPrimitiveComponent>(C);
		if (P) {
			P->SetCollisionEnabled((TEnumAsByte<ECollisionEnabled::Type>)(uint8)SpawnOptions.CollisionType);
			P->SetComponentTickEnabled(SpawnOptions.ActorTickEnabled);
			P->SetSimulatePhysics(SpawnOptions.SimulatePhysics);
			P->SetVisibility(true,true);
	} C->Activate(true);}
	//
	if (Settings->ReinitializeInstances) {
		SetActorLocationAndRotation(SpawnTransform.GetLocation(),SpawnTransform.GetRotation(),false,nullptr,ETeleportType::TeleportPhysics);
		SetActorScale3D(SpawnTransform.GetScale3D());
		SetActorTickEnabled(SpawnOptions.ActorTickEnabled);
		SetActorEnableCollision(SpawnOptions.EnableCollision);
		//
		FinishSpawnFromPool(Reconstruct,SpawnTransform);
		if (!Spawned) {return;}
		//
		if (LifeSpanPool>0.0001f) {
			const FTimerDelegate Timer = FTimerDelegate::CreateUObject(this,&APooledActor::ReturnToPool);
			GetWorld()->GetTimerManager().SetTimer(LifeSpanHandle,Timer,LifeSpanPool,false);
	} EVENT_OnPoolBeginPlay(); OnPoolBeginPlay.Broadcast();}
	//
	SetActorHiddenInGame(false);
	//
	for (const auto &C : Components) {
		const auto &P = Cast<UParticleSystemComponent>(C);
		if (P) {
			P->SetVisibility(true,true);
			P->Activate(true);
		P->ActivateSystem(true);
	}}
}

void APooledActor::FinishSpawnFromPool(const bool Reconstruct, const FTransform &Transform) {
	FTransform FinalRootComponentTransform = (RootComponent ? RootComponent->ComponentToWorld : Transform);
	//
	FinalRootComponentTransform.GetLocation().DiagnosticCheckNaN(TEXT("APooledActor::FinishSpawning: FinalRootComponentTransform.GetLocation()"));
	FinalRootComponentTransform.GetRotation().DiagnosticCheckNaN(TEXT("APooledActor::FinishSpawning: FinalRootComponentTransform.GetRotation()"));
	//
	if (GetWorld()) {
		FVector AdjustedLocation; FRotator AdjustedRotation;
		switch (SpawnCollisionHandlingMethod) {
			case ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn:
				AdjustedLocation = GetActorLocation();
				AdjustedRotation = GetActorRotation();
				if (GetWorld()->FindTeleportSpot(this,AdjustedLocation,AdjustedRotation)) {
					SetActorLocationAndRotation(AdjustedLocation,AdjustedRotation,false,nullptr,ETeleportType::TeleportPhysics);
			} break;
			//
			case ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding:	
				AdjustedLocation = GetActorLocation();
				AdjustedRotation = GetActorRotation();
				if (GetWorld()->FindTeleportSpot(this,AdjustedLocation,AdjustedRotation)) {
					SetActorLocationAndRotation(AdjustedLocation,AdjustedRotation,false,nullptr,ETeleportType::TeleportPhysics);
				} else {
					UE_LOG(LogTemp,Warning,TEXT("Spawn Actor from Pool: failed because of collision at the spawn location [%s] for [%s]"),*AdjustedLocation.ToString(),*GetClass()->GetName());
					Initialize();
				return;}
			break;
			//
			case ESpawnActorCollisionHandlingMethod::DontSpawnIfColliding:
				if (GetWorld()->EncroachingBlockingGeometry(this,GetActorLocation(),GetActorRotation())) {
					UE_LOG(LogTemp,Warning,TEXT("Spawn Actor from Pool: failed because of collision at the spawn location [%s] for [%s]"),*GetActorLocation().ToString(),*GetClass()->GetName());
					Initialize();
				return;}
			break;
			//
			case ESpawnActorCollisionHandlingMethod::Undefined:
			case ESpawnActorCollisionHandlingMethod::AlwaysSpawn:
	default: break;}}
	//
	if (Reconstruct) {
		ResetPropertiesForConstruction();
		RerunConstructionScripts();
	}
}

void APooledActor::ReturnToPool() {
	const auto &Settings = GetMutableDefault<UPoolSettings>();
	//
	SetActorEnableCollision(false);
	SetActorHiddenInGame(true);
	SetActorTickEnabled(false);
	//
	TInlineComponentArray<UActorComponent*> Components;
	GetComponents(Components);
	//
	for (const auto &C : Components) {
		const auto &P = Cast<UPrimitiveComponent>(C);
		if (P) {
			P->SetSimulatePhysics(false);
			P->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			P->SetPhysicsAngularVelocity(FVector::ZeroVector);
			P->SetPhysicsLinearVelocity(FVector::ZeroVector);
			P->SetComponentTickEnabled(false);
			P->SetVisibility(false,true);
		} C->Deactivate();
		//
		const auto &PRC = Cast<UParticleSystemComponent>(C);
	if (PRC) {PRC->Activate(true);}}
	//
	Spawned = false;
	if (OwningPool && OwningPool->IsValidLowLevel()) {
		if (Settings->ReinitializeInstances) {
			GetWorld()->GetTimerManager().ClearTimer(LifeSpanHandle);
			OnPoolEndPlay.Broadcast();
			EVENT_OnPoolEndPlay();
		} OwningPool->ReturnActor(const_cast<APooledActor*>(this));
	} else if (!IsPendingKill()) {Destroy();}
}

UObjectPool* APooledActor::GetOwningPool() const {
	return OwningPool;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////