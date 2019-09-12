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

#pragma once
#include "PooledActor.h"
#include "PooledPawn.h"
#include "PooledCharacter.h"
#include "PooledProjectile.h"
#include "PoolSpawnOptions.h"
#include "OBJPool.generated.h"

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Default Pool Settings

UCLASS(ClassGroup = Synaptech, Category = "Synaptech", config = Game)
class OBJPOOL_API UPoolSettings : public UObject {
	GENERATED_UCLASS_BODY()
public:
	/// If enabled, every time the object is Pooled by the Pool, it's OnPoolBeginPlay() and OnPoolEndPlay() events will be executed.
	UPROPERTY(Category = "General Settings", config, EditAnywhere, BlueprintReadOnly)
	bool ReinitializeInstances;
	//
	/// If enabled, a new object will be created by the Pool to fulfill the request from owner when there's no instances available;
	/// If disabled, nothing is instantiated if pool is empty and instead, resulting object returns NULL for the request out of bounds.
	UPROPERTY(Category = "General Settings", config, EditAnywhere, BlueprintReadOnly)
	bool InstantiateOnDemand;
	//
	/// If enabled, the Pool will guarantee that the 'Spawn Deferred' function will never fail with a null pointer.
	/// If there's no inactive Actor available then the function will point to the last spawned Actor instead.
	UPROPERTY(Category = "General Settings", config, EditAnywhere, BlueprintReadOnly)
	bool NeverFailDeferredSpawn;
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Template Pool Enumerators

UENUM(BlueprintType, meta = (DisplayName = "Validation", Keywords = "Spawn Validation"))
enum class FOBJPoolSpawning : uint8 {
	SpawnSuccessful,
	SpawnFailed
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Object Pool Component:: Stores Pooled Objects for Owner

/// {S} Pool Data Component;
/// Builds and holds pointers to the data stored in memory to be used by Object-Pool Queries.
UCLASS(ClassGroup = Synaptech, Category = "Performance", Blueprintable, BlueprintType, hideCategories = ("Activation", "Collision", "Sockets", "Variable"), meta = (BlueprintSpawnableComponent, DisplayName = "{S} Object-Pool Component", ShortTooltip = "{S} Pool Data Class. Check documentation for more information."))
class OBJPOOL_API UObjectPool : public UActorComponent {
	GENERATED_UCLASS_BODY()
	//
	friend class APooledActor;
protected:
	/// Empties the Pool; Respawns all of its members based on 'Pool Size' allocation.
	UFUNCTION(Category = "Object Pool", BlueprintCallable, meta = (DisplayName = "Initialize Object Pool", Keywords = "initialize pool"))
	virtual void InitializeObjectPool();
	/// The Memory Pool.
	UPROPERTY()
	TArray<APooledActor*> Pool;
public:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void FlushObjectPool();
	//
	//
	/// The Actor Class needed to populate this Object-Pool.
	UPROPERTY(Category = "Object Pool", EditAnywhere)
	TSubclassOf<APooledActor> TemplateClass;
	//
	/// How many instances you want to store pre-loaded.
	UPROPERTY(Category = "Object Pool", EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "1"))
	int32 PoolSize = 1;
	//
	/// If checked, this will make this Pool Component to automatically load Object Instances when the Game starts.
	/// Initialization is still going to happen, independently, when a Pull request is received and the Pool is empty.
	UPROPERTY(Category = "Object Pool", EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "1"))
	bool AutoInitialize;
	//
	//
	/// Spawns a Pooled Actor from this Object-Pool.
	UFUNCTION(Category = "Object Pool", BlueprintCallable, meta = (DeprecatedFunction, DeprecationMessage = "Please use the new ''Spawn Actor from Object-Pool ...'' function instead.", DisplayName = "Spawn Actor from Pool", Keywords = "spawn actor pool", ExpandEnumAsExecs = "Spawning"))
	void PullActor(APooledActor *& Instance, ESpawnActorCollisionHandlingMethod CollisionHandlingOverride, const FPoolSpawnOptions &SpawnOptions, const FTransform &SpawnTransform, APawn* Instigator, AActor* Owner, FOBJPoolSpawning &Spawning);
	//
	UFUNCTION()
	void ReturnActor(APooledActor* PooledActor);
	//
	/// Will destroy all Pooled Actors stored by this Object-Pool.
	/// Note: A Pool Component, when destroyed, will also destroy all of its Pooled objects; Spawned or not.
	UFUNCTION(Category = "Object Pool", BlueprintCallable, meta = (DisplayName = "Empty Object Pool", Keywords = "empty delete pool"))
	void EmptyObjectPool();
	//
	//
	/// Returns the Container Array of Pooled Objects.
	/// WARNING: Danger! Do not manually Add or Remove members from this!
	UFUNCTION(Category = "Object Pool", BlueprintCallable, meta = (DisplayName = "Get Object Array", Keywords = "get pool objects"))
	TArray<APooledActor*> GetObjectArray() const {return Pool;}
	//
	/// Fast-Query this Pool and returns all of its Spawned or Inactive Actors as separate lists.
	UFUNCTION(Category = "Object Pool", BlueprintCallable, meta = (DisplayName = "Get Objects From Pool", Keywords = "get pool objects"))
	void GetObjectsFromPool(TArray<APooledActor*> &Spawned, TArray<APooledActor*> &Inactive);
	//
	/// Fast-Query this Pool and returns all of its Spawned Actors list.
	UFUNCTION(Category = "Object Pool", BlueprintCallable, meta = (DisplayName = "Get Spawned Objects", Keywords = "get spawned objects"))
	void GetSpawnedObjects(TArray<APooledActor*> &Spawned);
	//
	/// Fast-Query this Pool and returns all of its Inactive Actors list.
	UFUNCTION(Category = "Object Pool", BlueprintCallable, meta = (DisplayName = "Get Inactive Objects", Keywords = "get inactive objects"))
	void GetInactiveObjects(TArray<APooledActor*> &Inactive);
	//
	/// Fast-Query this Pool and returns the first Spawned Actor found on top of the Pool.
	UFUNCTION(Category = "Object Pool", BlueprintCallable, meta = (DisplayName = "Get Spawned Object", Keywords = "get pool spawned object"))
	APooledActor* GetSpawnedObject() const;
	//
	/// Fast-Query this Pool and returns the first Inactive Actor found on top of the Pool.
	UFUNCTION(Category = "Object Pool", BlueprintCallable, meta = (DisplayName = "Get Inactive Object", Keywords = "get pool inactive object"))
	APooledActor* GetInactiveObject() const;
	//
	//
	/// Spawns an Actor from Pool, manually running its Construction Scripts if a full reset is needed.
	UFUNCTION(Category = "Object Pool", BlueprintCallable, meta = (WorldContext = "WorldContextObject", UnsafeDuringActorConstruction = "true", BlueprintInternalUseOnly = "true"))
	static APooledActor* BeginDeferredSpawnFromPool(const UObject* WorldContextObject, UObjectPool* ObjectPool, const FPoolSpawnOptions &SpawnOptions, const FTransform &SpawnTransform, ESpawnActorCollisionHandlingMethod CollisionHandlingOverride, AActor* Owner, const bool Reconstruct, bool &SpawnSuccessful);
	//
	/// Finishes Deferred Spawning an Actor from Pool.
	UFUNCTION(Category = "Object Pool", BlueprintCallable, meta = (WorldContext = "WorldContextObject", UnsafeDuringActorConstruction = "true", BlueprintInternalUseOnly = "true"))
	static APooledActor* FinishDeferredSpawnFromPool(APooledActor* Actor, const FTransform &SpawnTransform);
	//
	//
	/// Gets this Pool's Objects, Type-casting.
	/// Returns Empty Array if Cast fails.
	template <typename T>
	FORCEINLINE TArray<T*> GetObjectsFromPool() const {
		TArray<T*> TC;
		for (const auto &OBJ : Pool) {
			if (!OBJ->IsValidLowLevelFast()||OBJ->IsPendingKill()) {continue;}
			T* O = Cast<T>(*&OBJ); if (O) {TC.Add(O);}
	} return TC;}
	//
	/// Gets this Pool's Spawned Objects, Type-casting.
	/// Returns Empty Array if Cast fails.
	template <typename T>
	FORCEINLINE TArray<T*> GetSpawnedObjects() const {
		TArray<T*> TC;
		for (const auto &OBJ : Pool) {
			if (!OBJ->IsValidLowLevelFast()||OBJ->IsPendingKill()) {continue;}
			T* O = Cast<T>(*&OBJ); if (O&&O->Spawned) {TC.Add(O);}
	} return TC;}
	//
	/// Gets this Pool's Inactive Objects, Type-casting.
	/// Returns Empty Array if Cast fails.
	template <typename T>
	FORCEINLINE TArray<T*> GetInactiveObjects() const {
		TArray<T*> TC;
		for (const auto &OBJ : Pool) {
			if (!OBJ->IsValidLowLevelFast()||OBJ->IsPendingKill()) {continue;}
			T* O = Cast<T>(*&OBJ); if (O&&(!O->Spawned)) {TC.Add(O);}
	} return TC;}
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Pawn Pool Component:: Stores Pooled Pawn Actors for Owner

/// {S} Pool Data Component;
/// Builds and holds pointers to the data stored in memory to be used by Object-Pool Queries.
UCLASS(ClassGroup = Synaptech, Category = "Performance", Blueprintable, BlueprintType, hideCategories = ("Activation", "Collision", "Sockets", "Variable"), meta = (BlueprintSpawnableComponent, DisplayName = "{S} Pawn-Pool Component", ShortTooltip = "{S} Pool Data Class. Check documentation for more information."))
class OBJPOOL_API UPawnPool : public UActorComponent {
	GENERATED_UCLASS_BODY()
	//
	friend class APooledPawn;
protected:
	/// Empties the Pool; Respawns all of its members based on 'Pool Size' allocation.
	UFUNCTION(Category = "Object Pool", BlueprintCallable, meta = (DisplayName = "Initialize Object Pool", Keywords = "initialize pool"))
	virtual void InitializeObjectPool();
	/// The Memory Pool.
	UPROPERTY()
	TArray<APooledPawn*> Pool;
public:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void FlushObjectPool();
	//
	//
	/// The Actor Class needed to populate this Object-Pool.
	UPROPERTY(Category = "Object Pool", EditAnywhere)
	TSubclassOf<APooledPawn> TemplateClass;
	//
	/// How many instances you want to store pre-loaded.
	UPROPERTY(Category = "Object Pool", EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "1"))
	int32 PoolSize = 1;
	//
	/// If checked, this will make this Pool Component to automatically load Object Instances when the Game starts.
	/// Initialization is still going to happen, independently, when a Pull request is received and the Pool is empty.
	UPROPERTY(Category = "Object Pool", EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "1"))
	bool AutoInitialize;
	//
	//
	/// Spawns a Pooled Actor from this Object-Pool.
	UFUNCTION(Category = "Object Pool", BlueprintCallable, meta = (DeprecatedFunction, DeprecationMessage = "Please use the new ''Spawn Pawn from Object-Pool ...'' function instead.", DisplayName = "Spawn Actor from Pool", Keywords = "spawn actor pool", ExpandEnumAsExecs = "Spawning"))
	void PullActor(APooledPawn *& Instance, ESpawnActorCollisionHandlingMethod CollisionHandlingOverride, const FPoolSpawnOptions &SpawnOptions, const FTransform &SpawnTransform, APawn* Instigator, AActor* Owner, FOBJPoolSpawning &Spawning);
	//
	UFUNCTION()
	void ReturnActor(APooledPawn* PooledActor);
	//
	/// Will destroy all Pooled Actors stored by this Object-Pool.
	/// Note: A Pool Component, when destroyed, will also destroy all of its Pooled objects; Spawned or not.
	UFUNCTION(Category = "Object Pool", BlueprintCallable, meta = (DisplayName = "Empty Object Pool", Keywords = "empty delete pool"))
	void EmptyObjectPool();
	//
	//
	/// Returns the Container Array of Pooled Objects.
	/// WARNING: Danger! Do not manually Add or Remove members from this!
	UFUNCTION(Category = "Object Pool", BlueprintCallable, meta = (DisplayName = "Get Object Array", Keywords = "get pool objects"))
	TArray<APooledPawn*> GetObjectArray() const {return Pool;}
	//
	/// Fast-Query this Pool and returns all of its Spawned or Inactive Actors as separate lists.
	UFUNCTION(Category = "Object Pool", BlueprintCallable, meta = (DisplayName = "Get Objects From Pool", Keywords = "get pool objects"))
	void GetObjectsFromPool(TArray<APooledPawn*> &Spawned, TArray<APooledPawn*> &Inactive);
	//
	/// Fast-Query this Pool and returns all of its Spawned Actors list.
	UFUNCTION(Category = "Object Pool", BlueprintCallable, meta = (DisplayName = "Get Spawned Objects", Keywords = "get spawned objects"))
	void GetSpawnedObjects(TArray<APooledPawn*> &Spawned);
	//
	/// Fast-Query this Pool and returns all of its Inactive Actors list.
	UFUNCTION(Category = "Object Pool", BlueprintCallable, meta = (DisplayName = "Get Inactive Objects", Keywords = "get inactive objects"))
	void GetInactiveObjects(TArray<APooledPawn*> &Inactive);
	//
	/// Fast-Query this Pool and returns the first Spawned Actor found on top of the Pool.
	UFUNCTION(Category = "Object Pool", BlueprintCallable, meta = (DisplayName = "Get Spawned Object", Keywords = "get pool spawned object"))
	APooledPawn* GetSpawnedObject() const;
	//
	/// Fast-Query this Pool and returns the first Inactive Actor found on top of the Pool.
	UFUNCTION(Category = "Object Pool", BlueprintCallable, meta = (DisplayName = "Get Inactive Object", Keywords = "get pool inactive object"))
	APooledPawn* GetInactiveObject() const;
	//
	//
	/// Spawns an Actor from Pool, manually running its Construction Scripts if a full reset is needed.
	UFUNCTION(Category = "Object Pool", BlueprintCallable, meta = (WorldContext = "WorldContextObject", UnsafeDuringActorConstruction = "true", BlueprintInternalUseOnly = "true"))
	static APooledPawn* BeginDeferredSpawnFromPool(const UObject* WorldContextObject, UPawnPool* ObjectPool, const FPoolSpawnOptions &SpawnOptions, const FTransform &SpawnTransform, ESpawnActorCollisionHandlingMethod CollisionHandlingOverride, AActor* Owner, const bool Reconstruct, bool &SpawnSuccessful);
	//
	/// Finishes Deferred Spawning an Actor from Pool.
	UFUNCTION(Category = "Object Pool", BlueprintCallable, meta = (WorldContext = "WorldContextObject", UnsafeDuringActorConstruction = "true", BlueprintInternalUseOnly = "true"))
	static APooledPawn* FinishDeferredSpawnFromPool(APooledPawn* Pawn, const FTransform &SpawnTransform);
	//
	//
	/// Gets this Pool's Objects, Type-casting.
	/// Returns Empty Array if Cast fails.
	template <typename T>
	FORCEINLINE TArray<T*> GetObjectsFromPool() const {
		TArray<T*> TC;
		for (const auto &OBJ : Pool) {
			if (!OBJ->IsValidLowLevelFast()||OBJ->IsPendingKill()) {continue;}
			T* O = Cast<T>(*&OBJ); if (O) {TC.Add(O);}
	} return TC;}
	//
	/// Gets this Pool's Spawned Objects, Type-casting.
	/// Returns Empty Array if Cast fails.
	template <typename T>
	FORCEINLINE TArray<T*> GetSpawnedObjects() const {
		TArray<T*> TC;
		for (const auto &OBJ : Pool) {
			if (!OBJ->IsValidLowLevelFast()||OBJ->IsPendingKill()) {continue;}
			T* O = Cast<T>(*&OBJ); if (O&&O->Spawned) {TC.Add(O);}
	} return TC;}
	//
	/// Gets this Pool's Inactive Objects, Type-casting.
	/// Returns Empty Array if Cast fails.
	template <typename T>
	FORCEINLINE TArray<T*> GetInactiveObjects() const {
		TArray<T*> TC;
		for (const auto &OBJ : Pool) {
			if (!OBJ->IsValidLowLevelFast()||OBJ->IsPendingKill()) {continue;}
			T* O = Cast<T>(*&OBJ); if (O&&(!O->Spawned)) {TC.Add(O);}
	} return TC;}
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Character Pool Component:: Stores Pooled Character Actors for Owner

/// {S} Pool Data Component;
/// Builds and holds pointers to the data stored in memory to be used by Object-Pool Queries.
UCLASS(ClassGroup = Synaptech, Category = "Performance", Blueprintable, BlueprintType, hideCategories = ("Activation", "Collision", "Sockets", "Variable"), meta = (BlueprintSpawnableComponent, DisplayName = "{S} Character-Pool Component", ShortTooltip = "{S} Pool Data Class. Check documentation for more information."))
class OBJPOOL_API UCharacterPool : public UActorComponent {
	GENERATED_UCLASS_BODY()
	//
	friend class APooledCharacter;
protected:
	/// Empties the Pool; Respawns all of its members based on 'Pool Size' allocation.
	UFUNCTION(Category = "Object Pool", BlueprintCallable, meta = (DisplayName = "Initialize Object Pool", Keywords = "initialize pool"))
	virtual void InitializeObjectPool();
	/// The Memory Pool.
	UPROPERTY()
	TArray<APooledCharacter*> Pool;
public:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void FlushObjectPool();
	//
	//
	/// The Actor Class needed to populate this Object-Pool.
	UPROPERTY(Category = "Object Pool", EditAnywhere)
	TSubclassOf<APooledCharacter> TemplateClass;
	//
	/// How many instances you want to store pre-loaded.
	UPROPERTY(Category = "Object Pool", EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "1"))
	int32 PoolSize = 1;
	//
	/// If checked, this will make this Pool Component to automatically load Object Instances when the Game starts.
	/// Initialization is still going to happen, independently, when a Pull request is received and the Pool is empty.
	UPROPERTY(Category = "Object Pool", EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "1"))
	bool AutoInitialize;
	//
	//
	/// Spawns a Pooled Actor from this Object-Pool.
	UFUNCTION(Category = "Object Pool", BlueprintCallable, meta = (DeprecatedFunction, DeprecationMessage = "Please use the new ''Spawn Character from Object-Pool ...'' function instead.", DisplayName = "Spawn Actor from Pool", Keywords = "spawn actor pool", ExpandEnumAsExecs = "Spawning"))
	void PullActor(APooledCharacter *& Instance, ESpawnActorCollisionHandlingMethod CollisionHandlingOverride, const FPoolSpawnOptions &SpawnOptions, const FTransform &SpawnTransform, APawn* Instigator, AActor* Owner, FOBJPoolSpawning &Spawning);
	//
	UFUNCTION()
	void ReturnActor(APooledCharacter* PooledActor);
	//
	/// Will destroy all Pooled Actors stored by this Object-Pool.
	/// Note: A Pool Component, when destroyed, will also destroy all of its Pooled objects; Spawned or not.
	UFUNCTION(Category = "Object Pool", BlueprintCallable, meta = (DisplayName = "Empty Object Pool", Keywords = "empty delete pool"))
	void EmptyObjectPool();
	//
	//
	/// Returns the Container Array of Pooled Objects.
	/// WARNING: Danger! Do not manually Add or Remove members from this!
	UFUNCTION(Category = "Object Pool", BlueprintCallable, meta = (DisplayName = "Get Object Array", Keywords = "get pool objects"))
	TArray<APooledCharacter*> GetObjectArray() const {return Pool;}
	//
	/// Fast-Query this Pool and returns all of its Spawned or Inactive Actors as separate lists.
	UFUNCTION(Category = "Object Pool", BlueprintCallable, meta = (DisplayName = "Get Objects From Pool", Keywords = "get pool objects"))
	void GetObjectsFromPool(TArray<APooledCharacter*> &Spawned, TArray<APooledCharacter*> &Inactive);
	//
	/// Fast-Query this Pool and returns all of its Spawned Actors list.
	UFUNCTION(Category = "Object Pool", BlueprintCallable, meta = (DisplayName = "Get Spawned Objects", Keywords = "get spawned objects"))
	void GetSpawnedObjects(TArray<APooledCharacter*> &Spawned);
	//
	/// Fast-Query this Pool and returns all of its Inactive Actors list.
	UFUNCTION(Category = "Object Pool", BlueprintCallable, meta = (DisplayName = "Get Inactive Objects", Keywords = "get inactive objects"))
	void GetInactiveObjects(TArray<APooledCharacter*> &Inactive);
	//
	/// Fast-Query this Pool and returns the first Spawned Actor found on top of the Pool.
	UFUNCTION(Category = "Object Pool", BlueprintCallable, meta = (DisplayName = "Get Spawned Object", Keywords = "get pool spawned object"))
	APooledCharacter* GetSpawnedObject() const;
	//
	/// Fast-Query this Pool and returns the first Inactive Actor found on top of the Pool.
	UFUNCTION(Category = "Object Pool", BlueprintCallable, meta = (DisplayName = "Get Inactive Object", Keywords = "get pool inactive object"))
	APooledCharacter* GetInactiveObject() const;
	//
	//
	/// Spawns an Actor from Pool, manually running its Construction Scripts if a full reset is needed.
	UFUNCTION(Category = "Object Pool", BlueprintCallable, meta = (WorldContext = "WorldContextObject", UnsafeDuringActorConstruction = "true", BlueprintInternalUseOnly = "true"))
	static APooledCharacter* BeginDeferredSpawnFromPool(const UObject* WorldContextObject, UCharacterPool* ObjectPool, const FPoolSpawnOptions &SpawnOptions, const FTransform &SpawnTransform, ESpawnActorCollisionHandlingMethod CollisionHandlingOverride, AActor* Owner, const bool Reconstruct, bool &SpawnSuccessful);
	//
	/// Finishes Deferred Spawning an Actor from Pool.
	UFUNCTION(Category = "Object Pool", BlueprintCallable, meta = (WorldContext = "WorldContextObject", UnsafeDuringActorConstruction = "true", BlueprintInternalUseOnly = "true"))
	static APooledCharacter* FinishDeferredSpawnFromPool(APooledCharacter* Character, const FTransform &SpawnTransform);
	//
	//
	/// Gets this Pool's Objects, Type-casting.
	/// Returns Empty Array if Cast fails.
	template <typename T>
	FORCEINLINE TArray<T*> GetObjectsFromPool() const {
		TArray<T*> TC;
		for (const auto &OBJ : Pool) {
			if (!OBJ->IsValidLowLevelFast()||OBJ->IsPendingKill()) {continue;}
			T* O = Cast<T>(*&OBJ); if (O) {TC.Add(O);}
	} return TC;}
	//
	/// Gets this Pool's Spawned Objects, Type-casting.
	/// Returns Empty Array if Cast fails.
	template <typename T>
	FORCEINLINE TArray<T*> GetSpawnedObjects() const {
		TArray<T*> TC;
		for (const auto &OBJ : Pool) {
			if (!OBJ->IsValidLowLevelFast()||OBJ->IsPendingKill()) {continue;}
			T* O = Cast<T>(*&OBJ); if (O&&O->Spawned) {TC.Add(O);}
	} return TC;}
	//
	/// Gets this Pool's Inactive Objects, Type-casting.
	/// Returns Empty Array if Cast fails.
	template <typename T>
	FORCEINLINE TArray<T*> GetInactiveObjects() const {
		TArray<T*> TC;
		for (const auto &OBJ : Pool) {
			if (!OBJ->IsValidLowLevelFast()||OBJ->IsPendingKill()) {continue;}
			T* O = Cast<T>(*&OBJ); if (O&&(!O->Spawned)) {TC.Add(O);}
	} return TC;}
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void LOG_OBJ(const bool Debug, const float Duration, const FColor Color, const FString Message);

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////