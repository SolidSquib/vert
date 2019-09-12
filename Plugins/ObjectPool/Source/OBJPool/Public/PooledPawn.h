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

#include "Runtime/Engine/Classes/GameFramework/Pawn.h"
#include "Runtime/Engine/Public/TimerManager.h"
#include "PooledPawn.generated.h"

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class UPawnPool;

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FPWP_PoolEvent);

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Pooled Object:: UObject Template

/// {S} Pooled Object Template;
/// Pawn type capable of being spawned as member of a Pool and automatically managed by the Pool Component.
UCLASS(ClassGroup = Synaptech, Category = "Performance", Blueprintable, BlueprintType, meta = (DisplayName = "{S} Pool Pawn", ShortTooltip = "{S} Pool Pawn Class. Check documentation for more information."))
class OBJPOOL_API APooledPawn : public APawn {
	GENERATED_UCLASS_BODY()
	//
	friend class UPawnPool;
private:
	UPROPERTY()
	UPawnPool* OwningPool;
	//
	UPROPERTY()
	bool Spawned;
	//
	UPROPERTY()
	FTimerHandle LifeSpanHandle;
protected:
	/// Finishes Spawning this Pawn, after deferred spawning from the Object-Pool.
	virtual void FinishSpawnFromPool(const bool Reconstruct, const FTransform &Transform);
public:
	virtual void Initialize();
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	//
	/// How long this Pawn lives after Spawn from Pool. 0 = Forever.
	UPROPERTY(Category = "Object Pool", EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0"))
	float LifeSpanPool;
	//
	//
	UFUNCTION()
	void SpawnFromPool(const bool Reconstruct, const FPoolSpawnOptions &SpawnOptions, const FTransform &SpawnTransform);
	//
	/// Returns this Pooled Pawn to its owning Object-Pool, disabling it.
	UFUNCTION(Category = "Object Pool", BlueprintCallable, meta = (DisplayName = "Return to Pool", Keywords = "return character pool"))
	void ReturnToPool();
	//
	/// Returns reference to the Pool Component owner of this Pawn.
	UFUNCTION(Category = "Object Pool", BlueprintCallable, meta = (DisplayName = "Get Owning Pool", Keywords = "get owner pool"))
	UPawnPool* GetOwningPool() const;
	//
	//
	/// Event called every time this Pawn is spawned from the Pool.
	UPROPERTY(Category = "Object Pool", BlueprintAssignable)
	FOBJP_PoolEvent OnPoolBeginPlay;
	//
	/// Event called every time this Pawn is sent back to the Pool.
	UPROPERTY(Category = "Object Pool", BlueprintAssignable)
	FOBJP_PoolEvent OnPoolEndPlay;
	//
	/// Event called every time this Pawn is spawned from the Pool.
	UFUNCTION(Category = "Object Pool", BlueprintImplementableEvent, meta = (DisplayName = "On Pool Begin Play", Keywords = "pool begin play event"))
	void EVENT_OnPoolBeginPlay();
	//
	/// Event called every time this Pawn is sent back to the Pool.
	UFUNCTION(Category = "Object Pool", BlueprintImplementableEvent, meta = (DisplayName = "On Pool End Play", Keywords = "pool end play event"))
	void EVENT_OnPoolEndPlay();
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////