// Copyright Inside Out Games Ltd. 2017

#pragma once

#include "Components/ActorComponent.h"
#include "Interactives/Interactive.h"
#include "Weapons/WeaponPickup.h"
#include "CharacterInteractionComponent.generated.h"

UENUM()
enum class EInteractionState : uint8
{
	Free,
	HoldingItem
};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class VERT_API UCharacterInteractionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interact|Trace")
	FVector LocalSphereTraceOffset = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interact|Trace")
	float TraceRange = 50.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interact|Trace")
	bool FindFirstInteractive = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interact|Trace", meta = (EditCondition = "!FindFirstInteractive"))
	bool FavourEnvironmentInteractives = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interact|Debug")
	bool ShowDebug = false;

public:	
	// Sets default values for this component's properties
	UCharacterInteractionComponent();

	class IInteractive* AttemptInteract();
	bool HoldInteractive(IInteractive* interactive, const FVector& localOffset = FVector::ZeroVector, bool forceDrop = false);
	void DropInteractive();
	void ForceDropInteractive(FVector force, float radialForce);
	bool AttemptAttack();
	void StopAttacking();

	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	FORCEINLINE IInteractive* GetHeldInteractive() const { return mHeldInteractive; }
	FORCEINLINE IWeaponPickup* GetHeldWeapon() const { return mHeldWeapon; }

	UFUNCTION(BlueprintCallable, Category = "Interact|Character")
	FORCEINLINE AVertCharacter* GetCharacterOwner() const { return mCharacterOwner.Get(); }

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

private:
	class IInteractive* TraceForSingleInteractive();
	class TArray<IInteractive*> TraceForMultipleInteractives();
	void DrawDebug();

private:
	TWeakObjectPtr<AVertCharacter> mCharacterOwner = nullptr;
	IInteractive* mHeldInteractive = nullptr;
	IWeaponPickup* mHeldWeapon = nullptr;
	EInteractionState mInteractionState = EInteractionState::Free;
};
