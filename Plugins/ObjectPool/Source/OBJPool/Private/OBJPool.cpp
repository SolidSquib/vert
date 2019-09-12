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

#include "OBJPool.h"
#include "OBJPoolShared.h"

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

UPoolSettings::UPoolSettings(const FObjectInitializer& OBJ) : Super(OBJ) {
	ReinitializeInstances = true;
	InstantiateOnDemand = false;
	NeverFailDeferredSpawn = false;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Object Pool Component Functions

UObjectPool::UObjectPool(const FObjectInitializer& OBJ) : Super(OBJ) {
	TemplateClass = APooledActor::StaticClass();
	bWantsInitializeComponent = true;
	SetComponentTickEnabled(false);
	SetActive(true,false);
	AutoInitialize = true;
}

void UObjectPool::BeginPlay() {
	Super::BeginPlay();
	//
	if (AutoInitialize) {InitializeObjectPool();}
}

void UObjectPool::EndPlay(const EEndPlayReason::Type EndPlayReason) {
	EmptyObjectPool();
	//
	Super::EndPlay(EndPlayReason);
}

void UObjectPool::InitializeObjectPool() {
	if (TemplateClass==NULL) {
		LOG_OBJ(true,5.f,FColor::Red,GetNameSafe(this)+FString(" : ")+FString("Invalid Template Class!"));
	return;} EmptyObjectPool();
	//
	FTransform SpawnTransform(FRotator::ZeroRotator,FVector::ZeroVector,FVector(1.f,1.f,1.f));
	for (int32 I=0; I<PoolSize; I++) {
		const auto &ACT = GetWorld()->SpawnActorDeferred<APooledActor>(TemplateClass,SpawnTransform,this->GetOwner(),GetOwner()->GetInstigator(),ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
		if (ACT) {
			ACT->OwningPool = const_cast<UObjectPool*>(this);
			ACT->FinishSpawning(SpawnTransform);
	ACT->Initialize();}}
	//
	FlushObjectPool();
}

void UObjectPool::FlushObjectPool() {
	if (!Pool.IsValidIndex(0)) {return;}
	//
	for (int32 ID = Pool.Num()-1; ID>=0; --ID) {
		if (!Pool[ID]->IsValidLowLevelFast()||Pool[ID]->IsPendingKill()) {Pool.RemoveAt(ID,1,false);}
	}
}

void UObjectPool::EmptyObjectPool() {
	if (!Pool.IsValidIndex(0)) {return;}
	//
	for (int32 ID = Pool.Num()-1; ID>=0; --ID) {
		if (Pool[ID]->IsValidLowLevelFast()) {Pool[ID]->Destroy();}
	} Pool.Empty();
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

APooledActor* UObjectPool::BeginDeferredSpawnFromPool(const UObject* WorldContextObject, UObjectPool* ObjectPool, const FPoolSpawnOptions &SpawnOptions, const FTransform& SpawnTransform, ESpawnActorCollisionHandlingMethod CollisionHandlingOverride, AActor* Owner, const bool Reconstruct, bool &SpawnSuccessful) {
	if (!ObjectPool->IsValidLowLevelFast()||ObjectPool->IsPendingKill()) {return nullptr;}
	const auto &Settings = GetMutableDefault<UPoolSettings>();
	//
	if (ObjectPool->Pool.Num()==0) {ObjectPool->InitializeObjectPool();}
	APooledActor* DeferredSpawn = ObjectPool->GetInactiveObject();
	//
	if ((DeferredSpawn==nullptr) && Settings->InstantiateOnDemand) {
		DeferredSpawn = ObjectPool->GetWorld()->SpawnActorDeferred<APooledActor>(ObjectPool->TemplateClass,SpawnTransform,Owner,ObjectPool->GetOwner()->GetInstigator(),CollisionHandlingOverride);
		if (DeferredSpawn) {DeferredSpawn->OwningPool = ObjectPool; DeferredSpawn->FinishSpawning(SpawnTransform);}
	} else if (DeferredSpawn==nullptr && Settings->NeverFailDeferredSpawn) {
		DeferredSpawn = ObjectPool->GetSpawnedObject();
		if (DeferredSpawn!=nullptr) {DeferredSpawn->ReturnToPool();}
	} ObjectPool->FlushObjectPool();
	//
	if (DeferredSpawn!=nullptr) {
		const APawn* Instigator = Cast<APawn>(WorldContextObject);
		if (Instigator==nullptr) { if (const AActor* ContextActor = Cast<AActor>(WorldContextObject)) {Instigator = ContextActor->Instigator;} }
		if (Instigator!=nullptr) {DeferredSpawn->Instigator = const_cast<APawn*>(Instigator);}
		if (Owner==nullptr) {DeferredSpawn->SetOwner(ObjectPool->GetOwner());}
		//
		DeferredSpawn->OwningPool = ObjectPool;
		DeferredSpawn->Initialize();
		DeferredSpawn->SpawnFromPool(Reconstruct,SpawnOptions,SpawnTransform);
		SpawnSuccessful = DeferredSpawn->Spawned;
	} else {
		SpawnSuccessful = false;
		DeferredSpawn = ObjectPool->GetSpawnedObject();
	} return DeferredSpawn;
}

APooledActor* UObjectPool::FinishDeferredSpawnFromPool(APooledActor* Actor, const FTransform& SpawnTransform) {
	if (Actor->IsValidLowLevelFast()&&!Actor->IsActorInitialized()&&!Actor->IsPendingKill()) {
		Actor->FinishSpawning(SpawnTransform);
	} return Actor;
}

DEPRECATED(4.15,"Please use the new ''Spawn Actor from Object-Pool ...'' instead.")
void UObjectPool::PullActor(APooledActor *& Instance, ESpawnActorCollisionHandlingMethod CollisionHandlingOverride, const FPoolSpawnOptions &SpawnOptions, const FTransform &SpawnTransform, APawn* Instigator, AActor* Owner, FOBJPoolSpawning &Spawning) {
	if (TemplateClass==NULL) {LOG_OBJ(true,5.f,FColor::Red,GetNameSafe(this)+FString(" : ")+FString("Invalid Template Class!")); return;}
	if (!IsValidLowLevelFast()||IsPendingKill()) {return;}
	//
	Spawning = FOBJPoolSpawning::SpawnFailed;
	const auto &Settings = GetMutableDefault<UPoolSettings>();
	//
	if (Pool.Num()==0) {InitializeObjectPool();}
	Instance = GetInactiveObject();
	//
	if ((Instance==nullptr) && Settings->InstantiateOnDemand) {
		Instance = GetWorld()->SpawnActorDeferred<APooledActor>(TemplateClass,SpawnTransform,Owner,GetOwner()->GetInstigator(),CollisionHandlingOverride);
		if (Instance) {Instance->OwningPool = const_cast<UObjectPool*>(this); Instance->FinishSpawning(SpawnTransform);}
	} else if (Instance==nullptr && Settings->NeverFailDeferredSpawn) {
		Instance = GetSpawnedObject();
		if (Instance!=nullptr) {Instance->ReturnToPool();}
	} FlushObjectPool();
	//
	if (Instance!=nullptr) {
		if (Instigator==nullptr) {Instigator = GetOwner()->Instigator;}
		if (Instigator!=nullptr) {Instance->Instigator = Instigator;}
		if (Owner==nullptr) {Instance->SetOwner(GetOwner());}
		//
		Instance->OwningPool = const_cast<UObjectPool*>(this);
		Instance->Initialize();
		Instance->SpawnFromPool(false,SpawnOptions,SpawnTransform);
	Spawning = FOBJPoolSpawning::SpawnSuccessful;}
}

void UObjectPool::ReturnActor(APooledActor* PooledActor) {
	if (!GetOwner()->IsValidLowLevelFast()||TemplateClass==NULL) {
		LOG_OBJ(true,5.f,FColor::Red,GetNameSafe(this)+FString(" : ")+FString("Invalid Template Class!"));
	return;}
	//
	if (!PooledActor->IsValidLowLevelFast()||PooledActor->IsPendingKill()) {return;}
	//
	if (PooledActor->GetClass()->IsChildOf(TemplateClass->GetDefaultObject()->GetClass())) {
		if (!Pool.Contains(PooledActor)) {
			PooledActor->SetActorLocation(GetOwner()->GetActorLocation());
			Pool.Add(PooledActor);
		}
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void UObjectPool::GetObjectsFromPool(TArray<APooledActor*> &Spawned, TArray<APooledActor*> &Inactive) {
	for (const auto &OBJ : Pool) {
		if (!OBJ->IsValidLowLevelFast()||OBJ->IsPendingKill()) {continue;}
	if (OBJ->Spawned) {Spawned.Add(OBJ);} else {Inactive.Add(OBJ);}}
}

void UObjectPool::GetSpawnedObjects(TArray<APooledActor*>& Spawned) {
	for (const auto &OBJ : Pool) {
		if (!OBJ->IsValidLowLevelFast()||OBJ->IsPendingKill()) {continue;}
	if (OBJ->Spawned) {Spawned.Add(OBJ);}}
}

void UObjectPool::GetInactiveObjects(TArray<APooledActor*> &Inactive) {
	for (const auto &OBJ : Pool) {
		if (!OBJ->IsValidLowLevelFast()||OBJ->IsPendingKill()) {continue;}
	if (!OBJ->Spawned) {Inactive.Add(OBJ);}}
}

APooledActor* UObjectPool::GetSpawnedObject() const {
	for (int32 ID = Pool.Num()-1; ID>=0; --ID) {
		if (!Pool[ID]->IsValidLowLevelFast()||Pool[ID]->IsPendingKill()) {continue;}
	if (Pool[ID]->Spawned) {return Pool[ID];}}
	//
	return nullptr;
}

APooledActor* UObjectPool::GetInactiveObject() const {
	for (int32 ID = Pool.Num()-1; ID>=0; --ID) {
		if (!Pool[ID]->IsValidLowLevelFast()||Pool[ID]->IsPendingKill()) {continue;}
	if (!Pool[ID]->Spawned) {return Pool[ID];}}
	//
	return nullptr;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Pawn Pool Component Functions

UPawnPool::UPawnPool(const FObjectInitializer& OBJ) : Super(OBJ) {
	TemplateClass = APooledPawn::StaticClass();
	bWantsInitializeComponent = true;
	SetComponentTickEnabled(false);
	SetActive(true,false);
	AutoInitialize = true;
}

void UPawnPool::BeginPlay() {
	Super::BeginPlay();
	//
	if (AutoInitialize) {InitializeObjectPool();}
}

void UPawnPool::EndPlay(const EEndPlayReason::Type EndPlayReason) {
	EmptyObjectPool();
	//
	Super::EndPlay(EndPlayReason);
}

void UPawnPool::InitializeObjectPool() {
	if (TemplateClass==NULL) {
		LOG_OBJ(true,5.f,FColor::Red,GetNameSafe(this)+FString(" : ")+FString("Invalid Template Class!"));
	return;} EmptyObjectPool();
	//
	FTransform SpawnTransform(FRotator::ZeroRotator,FVector::ZeroVector,FVector(1.f,1.f,1.f));
	for (int32 I=0; I<PoolSize; I++) {
		const auto &ACT = GetWorld()->SpawnActorDeferred<APooledPawn>(TemplateClass,SpawnTransform,this->GetOwner(),GetOwner()->GetInstigator(),ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
		if (ACT) {
			ACT->OwningPool = const_cast<UPawnPool*>(this);
			ACT->FinishSpawning(SpawnTransform);
	ACT->Initialize();}}
}

void UPawnPool::FlushObjectPool() {
	if (!Pool.IsValidIndex(0)) {return;}
	//
	for (int32 ID = Pool.Num()-1; ID>=0; --ID) {
		if (!Pool[ID]->IsValidLowLevelFast()||Pool[ID]->IsPendingKill()) {Pool.RemoveAt(ID,1,false);}
	}
}

void UPawnPool::EmptyObjectPool() {
	if (!Pool.IsValidIndex(0)) {return;}
	//
	for (int32 ID = Pool.Num()-1; ID>=0; --ID) {
		if (Pool[ID]->IsValidLowLevelFast()) {Pool[ID]->Destroy();}
	} Pool.Empty();
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

APooledPawn* UPawnPool::BeginDeferredSpawnFromPool(const UObject* WorldContextObject, UPawnPool* ObjectPool, const FPoolSpawnOptions &SpawnOptions, const FTransform& SpawnTransform, ESpawnActorCollisionHandlingMethod CollisionHandlingOverride, AActor* Owner, const bool Reconstruct, bool &SpawnSuccessful) {
	if (!ObjectPool->IsValidLowLevelFast()||ObjectPool->IsPendingKill()) {return nullptr;}
	const auto &Settings = GetMutableDefault<UPoolSettings>();
	//
	if (ObjectPool->Pool.Num()==0) {ObjectPool->InitializeObjectPool();}
	APooledPawn* DeferredSpawn = ObjectPool->GetInactiveObject();
	//
	if ((DeferredSpawn==nullptr) && Settings->InstantiateOnDemand) {
		DeferredSpawn = ObjectPool->GetWorld()->SpawnActorDeferred<APooledPawn>(ObjectPool->TemplateClass,SpawnTransform,Owner,ObjectPool->GetOwner()->GetInstigator(),CollisionHandlingOverride);
		if (DeferredSpawn) {DeferredSpawn->OwningPool = ObjectPool; DeferredSpawn->FinishSpawning(SpawnTransform);}
	} else if (DeferredSpawn==nullptr && Settings->NeverFailDeferredSpawn) {
		DeferredSpawn = ObjectPool->GetSpawnedObject();
		if (DeferredSpawn!=nullptr) {DeferredSpawn->ReturnToPool();}
	} ObjectPool->FlushObjectPool();
	//
	if (DeferredSpawn!=nullptr) {
		const APawn* Instigator = Cast<APawn>(WorldContextObject);
		if (Instigator==nullptr) { if (const AActor* ContextActor = Cast<AActor>(WorldContextObject)) {Instigator = ContextActor->Instigator;} }
		if (Instigator!=nullptr) {DeferredSpawn->Instigator = const_cast<APawn*>(Instigator);}
		if (Owner==nullptr) {DeferredSpawn->SetOwner(ObjectPool->GetOwner());}
		//
		DeferredSpawn->OwningPool = ObjectPool;
		DeferredSpawn->Initialize();
		DeferredSpawn->SpawnFromPool(Reconstruct,SpawnOptions,SpawnTransform);
		SpawnSuccessful = DeferredSpawn->Spawned;
	} else {
		SpawnSuccessful = false;
		DeferredSpawn = ObjectPool->GetSpawnedObject();
	} return DeferredSpawn;
}

APooledPawn* UPawnPool::FinishDeferredSpawnFromPool(APooledPawn* Pawn, const FTransform& SpawnTransform) {
	if (Pawn->IsValidLowLevelFast()&&!Pawn->IsActorInitialized()&&!Pawn->IsPendingKill()) {
		Pawn->FinishSpawning(SpawnTransform);
	} return Pawn;
}

DEPRECATED(4.15,"Please use the new ''Spawn Pawn from Object-Pool ...'' function instead.")
void UPawnPool::PullActor(APooledPawn *& Instance, ESpawnActorCollisionHandlingMethod CollisionHandlingOverride, const FPoolSpawnOptions &SpawnOptions, const FTransform &SpawnTransform, APawn* Instigator, AActor* Owner, FOBJPoolSpawning &Spawning) {
	if (TemplateClass==NULL) {LOG_OBJ(true,5.f,FColor::Red,GetNameSafe(this)+FString(" : ")+FString("Invalid Template Class!")); return;}
	if (!IsValidLowLevelFast()||IsPendingKill()) {return;}
	//
	Spawning = FOBJPoolSpawning::SpawnFailed;
	const auto &Settings = GetMutableDefault<UPoolSettings>();
	//
	if (Pool.Num()==0) {InitializeObjectPool();}
	Instance = GetInactiveObject();
	//
	if ((Instance==nullptr) && Settings->InstantiateOnDemand) {
		Instance = GetWorld()->SpawnActorDeferred<APooledPawn>(TemplateClass,SpawnTransform,Owner,GetOwner()->GetInstigator(),CollisionHandlingOverride);
		if (Instance) {Instance->OwningPool = const_cast<UPawnPool*>(this); Instance->FinishSpawning(SpawnTransform);}
	} else if (Instance==nullptr && Settings->NeverFailDeferredSpawn) {
		Instance = GetSpawnedObject();
		if (Instance!=nullptr) {Instance->ReturnToPool();}
	} FlushObjectPool();
	//
	if (Instance!=nullptr) {
		if (Instigator==nullptr) {Instigator = GetOwner()->Instigator;}
		if (Instigator!=nullptr) {Instance->Instigator = Instigator;}
		if (Owner==nullptr) {Instance->SetOwner(GetOwner());}
		//
		Instance->OwningPool = const_cast<UPawnPool*>(this);
		Instance->Initialize();
		Instance->SpawnFromPool(false,SpawnOptions,SpawnTransform);
	Spawning = FOBJPoolSpawning::SpawnSuccessful;}
}

void UPawnPool::ReturnActor(APooledPawn* PooledActor) {
	if (!GetOwner()->IsValidLowLevelFast()||TemplateClass==NULL) {
		LOG_OBJ(true,5.f,FColor::Red,GetNameSafe(this)+FString(" : ")+FString("Invalid Template Class!"));
	return;}
	//
	if (!PooledActor->IsValidLowLevelFast()||PooledActor->IsPendingKill()) {return;}
	//
	if (PooledActor->GetClass()->IsChildOf(TemplateClass->GetDefaultObject()->GetClass())) {
		if (!Pool.Contains(PooledActor)) {
			PooledActor->SetActorLocation(GetOwner()->GetActorLocation());
			Pool.Add(PooledActor);
		}
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void UPawnPool::GetObjectsFromPool(TArray<APooledPawn*> &Spawned, TArray<APooledPawn*> &Inactive) {
	for (const auto &OBJ : Pool) {
		if (!OBJ->IsValidLowLevelFast()||OBJ->IsPendingKill()) {continue;}
	if (OBJ->Spawned) {Spawned.Add(OBJ);} else {Inactive.Add(OBJ);}}
}

void UPawnPool::GetSpawnedObjects(TArray<APooledPawn*>& Spawned) {
	for (const auto &OBJ : Pool) {
		if (!OBJ->IsValidLowLevelFast()||OBJ->IsPendingKill()) {continue;}
	if (OBJ->Spawned) {Spawned.Add(OBJ);}}
}

void UPawnPool::GetInactiveObjects(TArray<APooledPawn*> &Inactive) {
	for (const auto &OBJ : Pool) {
		if (!OBJ->IsValidLowLevelFast()||OBJ->IsPendingKill()) {continue;}
	if (!OBJ->Spawned) {Inactive.Add(OBJ);}}
}

APooledPawn* UPawnPool::GetSpawnedObject() const {
	for (int32 ID = Pool.Num()-1; ID>=0; --ID) {
		if (!Pool[ID]->IsValidLowLevelFast()||Pool[ID]->IsPendingKill()) {continue;}
	if (Pool[ID]->Spawned) {return Pool[ID];}}
	//
	return nullptr;
}

APooledPawn* UPawnPool::GetInactiveObject() const {
	for (int32 ID = Pool.Num()-1; ID>=0; --ID) {
		if (!Pool[ID]->IsValidLowLevelFast()||Pool[ID]->IsPendingKill()) {continue;}
	if (!Pool[ID]->Spawned) {return Pool[ID];}}
	//
	return nullptr;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Character Pool Component Functions

UCharacterPool::UCharacterPool(const FObjectInitializer& OBJ) : Super(OBJ) {
	TemplateClass = APooledCharacter::StaticClass();
	bWantsInitializeComponent = true;
	SetComponentTickEnabled(false);
	SetActive(true,false);
	AutoInitialize = true;
}

void UCharacterPool::BeginPlay() {
	Super::BeginPlay();
	//
	if (AutoInitialize) {InitializeObjectPool();}
}

void UCharacterPool::EndPlay(const EEndPlayReason::Type EndPlayReason) {
	EmptyObjectPool();
	//
	Super::EndPlay(EndPlayReason);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void UCharacterPool::InitializeObjectPool() {
	if (TemplateClass==NULL) {
		LOG_OBJ(true,5.f,FColor::Red,GetNameSafe(this)+FString(" : ")+FString("Invalid Template Class!"));
	return;} EmptyObjectPool();
	//
	FTransform SpawnTransform(FRotator::ZeroRotator,FVector::ZeroVector,FVector(1.f,1.f,1.f));
	for (int32 I=0; I<PoolSize; I++) {
		const auto &ACT = GetWorld()->SpawnActorDeferred<APooledCharacter>(TemplateClass,SpawnTransform,this->GetOwner(),GetOwner()->GetInstigator(),ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
		if (ACT) {
			ACT->OwningPool = const_cast<UCharacterPool*>(this);
			ACT->FinishSpawning(SpawnTransform);
	ACT->Initialize();}}
}

void UCharacterPool::FlushObjectPool() {
	if (!Pool.IsValidIndex(0)) {return;}
	//
	for (int32 ID = Pool.Num()-1; ID>=0; --ID) {
		if (!Pool[ID]->IsValidLowLevelFast()||Pool[ID]->IsPendingKill()) {Pool.RemoveAt(ID,1,false);}
	}
}

void UCharacterPool::EmptyObjectPool() {
	if (!Pool.IsValidIndex(0)) {return;}
	//
	for (int32 ID = Pool.Num()-1; ID>=0; --ID) {
		if (Pool[ID]->IsValidLowLevelFast()) {Pool[ID]->Destroy();}
	} Pool.Empty();
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

APooledCharacter* UCharacterPool::BeginDeferredSpawnFromPool(const UObject* WorldContextObject, UCharacterPool* ObjectPool, const FPoolSpawnOptions &SpawnOptions, const FTransform& SpawnTransform, ESpawnActorCollisionHandlingMethod CollisionHandlingOverride, AActor* Owner, const bool Reconstruct, bool &SpawnSuccessful) {
	if (!ObjectPool->IsValidLowLevelFast()||ObjectPool->IsPendingKill()) {return nullptr;}
	const auto &Settings = GetMutableDefault<UPoolSettings>();
	//
	if (ObjectPool->Pool.Num()==0) {ObjectPool->InitializeObjectPool();}
	APooledCharacter* DeferredSpawn = ObjectPool->GetInactiveObject();
	//
	if ((DeferredSpawn==nullptr) && Settings->InstantiateOnDemand) {
		DeferredSpawn = ObjectPool->GetWorld()->SpawnActorDeferred<APooledCharacter>(ObjectPool->TemplateClass,SpawnTransform,Owner,ObjectPool->GetOwner()->GetInstigator(),CollisionHandlingOverride);
		if (DeferredSpawn) {DeferredSpawn->OwningPool = ObjectPool; DeferredSpawn->FinishSpawning(SpawnTransform);}
	} else if (DeferredSpawn==nullptr && Settings->NeverFailDeferredSpawn) {
		DeferredSpawn = ObjectPool->GetSpawnedObject();
		if (DeferredSpawn!=nullptr) {DeferredSpawn->ReturnToPool();}
	} ObjectPool->FlushObjectPool();
	//
	if (DeferredSpawn!=nullptr) {
		const APawn* Instigator = Cast<APawn>(WorldContextObject);
		if (Instigator==nullptr) { if (const AActor* ContextActor = Cast<AActor>(WorldContextObject)) {Instigator = ContextActor->Instigator;} }
		if (Instigator!=nullptr) {DeferredSpawn->Instigator = const_cast<APawn*>(Instigator);}
		if (Owner==nullptr) {DeferredSpawn->SetOwner(ObjectPool->GetOwner());}
		//
		DeferredSpawn->OwningPool = ObjectPool;
		DeferredSpawn->Initialize();
		DeferredSpawn->SpawnFromPool(Reconstruct,SpawnOptions,SpawnTransform);
		SpawnSuccessful = DeferredSpawn->Spawned;
	} else {
		SpawnSuccessful = false;
		DeferredSpawn = ObjectPool->GetSpawnedObject();
	} return DeferredSpawn;
}

APooledCharacter* UCharacterPool::FinishDeferredSpawnFromPool(APooledCharacter* Character, const FTransform& SpawnTransform) {
	if (Character->IsValidLowLevelFast()&&!Character->IsActorInitialized()&&!Character->IsPendingKill()) {
		Character->FinishSpawning(SpawnTransform);
	} return Character;
}

DEPRECATED(4.15,"Please use the new ''Spawn Character from Object-Pool ...'' function instead.")
void UCharacterPool::PullActor(APooledCharacter *& Instance, ESpawnActorCollisionHandlingMethod CollisionHandlingOverride, const FPoolSpawnOptions &SpawnOptions, const FTransform &SpawnTransform, APawn* Instigator, AActor* Owner, FOBJPoolSpawning &Spawning) {
	if (TemplateClass==NULL) {LOG_OBJ(true,5.f,FColor::Red,GetNameSafe(this)+FString(" : ")+FString("Invalid Template Class!")); return;}
	if (!IsValidLowLevelFast()||IsPendingKill()) {return;}
	//
	Spawning = FOBJPoolSpawning::SpawnFailed;
	const auto &Settings = GetMutableDefault<UPoolSettings>();
	//
	if (Pool.Num()==0) {InitializeObjectPool();}
	Instance = GetInactiveObject();
	//
	if ((Instance==nullptr) && Settings->InstantiateOnDemand) {
		Instance = GetWorld()->SpawnActorDeferred<APooledCharacter>(TemplateClass,SpawnTransform,Owner,GetOwner()->GetInstigator(),CollisionHandlingOverride);
		if (Instance) {Instance->OwningPool = const_cast<UCharacterPool*>(this); Instance->FinishSpawning(SpawnTransform);}
	} else if (Instance==nullptr && Settings->NeverFailDeferredSpawn) {
		Instance = GetSpawnedObject();
		if (Instance!=nullptr) {Instance->ReturnToPool();}
	} FlushObjectPool();
	//
	if (Instance!=nullptr) {
		if (Instigator==nullptr) {Instigator = GetOwner()->Instigator;}
		if (Instigator!=nullptr) {Instance->Instigator = Instigator;}
		if (Owner==nullptr) {Instance->SetOwner(GetOwner());}
		//
		Instance->OwningPool = const_cast<UCharacterPool*>(this);
		Instance->Initialize();
		Instance->SpawnFromPool(false,SpawnOptions,SpawnTransform);
	Spawning = FOBJPoolSpawning::SpawnSuccessful;}
}

void UCharacterPool::ReturnActor(APooledCharacter* PooledActor) {
	if (!GetOwner()->IsValidLowLevelFast()||TemplateClass==NULL) {
		LOG_OBJ(true,5.f,FColor::Red,GetNameSafe(this)+FString(" : ")+FString("Invalid Template Class!"));
	return;}
	//
	if (!PooledActor->IsValidLowLevelFast()||PooledActor->IsPendingKill()) {return;}
	//
	if (PooledActor->GetClass()->IsChildOf(TemplateClass->GetDefaultObject()->GetClass())) {
		if (!Pool.Contains(PooledActor)) {
			PooledActor->SetActorLocation(GetOwner()->GetActorLocation());
			Pool.Add(PooledActor);
		}
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void UCharacterPool::GetObjectsFromPool(TArray<APooledCharacter*> &Spawned, TArray<APooledCharacter*> &Inactive) {
	for (const auto &OBJ : Pool) {
		if (!OBJ->IsValidLowLevelFast()||OBJ->IsPendingKill()) {continue;}
	if (OBJ->Spawned) {Spawned.Add(OBJ);} else {Inactive.Add(OBJ);}}
}

void UCharacterPool::GetSpawnedObjects(TArray<APooledCharacter*>& Spawned) {
	for (const auto &OBJ : Pool) {
		if (!OBJ->IsValidLowLevelFast()||OBJ->IsPendingKill()) {continue;}
	if (OBJ->Spawned) {Spawned.Add(OBJ);}}
}

void UCharacterPool::GetInactiveObjects(TArray<APooledCharacter*> &Inactive) {
	for (const auto &OBJ : Pool) {
		if (!OBJ->IsValidLowLevelFast()||OBJ->IsPendingKill()) {continue;}
	if (!OBJ->Spawned) {Inactive.Add(OBJ);}}
}

APooledCharacter* UCharacterPool::GetSpawnedObject() const {
	for (int32 ID = Pool.Num()-1; ID>=0; --ID) {
		if (!Pool[ID]->IsValidLowLevelFast()||Pool[ID]->IsPendingKill()) {continue;}
	if (Pool[ID]->Spawned) {return Pool[ID];}}
	//
	return nullptr;
}

APooledCharacter* UCharacterPool::GetInactiveObject() const {
	for (int32 ID = Pool.Num()-1; ID>=0; --ID) {
		if (!Pool[ID]->IsValidLowLevelFast()||Pool[ID]->IsPendingKill()) {continue;}
	if (!Pool[ID]->Spawned) {return Pool[ID];}}
	//
	return nullptr;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void LOG_OBJ(const bool Debug,const float Duration, const FColor Color, const FString Message) {
	if (!Debug) return; UE_LOG(LogTemp,Warning,TEXT("{Pool}:: %s"),*Message); if (!GEngine) {return;}
	GEngine->AddOnScreenDebugMessage(-1,Duration,Color,FString::Printf(TEXT("{Pool}:: %s"),*Message));
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////