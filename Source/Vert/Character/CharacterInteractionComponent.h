// Copyright Inside Out Games Ltd. 2017

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Interactives/Interactive.h"
#include "Weapons/BaseWeapon.h"
#include "CharacterInteractionComponent.generated.h"

UENUM()
enum class EInteractionState : uint8
{
	Free,
	HoldingItem
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnPickupInteractiveDelegate, AInteractive*, interactive, bool, wasCaught);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnDropInteractiveDelegate, AInteractive*, interactive, bool, wasThrown);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class VERT_API UCharacterInteractionComponent : public UActorComponent
{
	GENERATED_BODY()
		
public:
	UPROPERTY(BlueprintAssignable)
	FOnPickupInteractiveDelegate Delegate_OnPickupInteractive;

	UPROPERTY(BlueprintAssignable)
	FOnDropInteractiveDelegate Delegate_OnDropInteractive;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Audio")
	class UAkAudioEvent* ThrowSound = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interact|Trace")
	FVector LocalSphereTraceOffset = FVector::ZeroVector;

	// default weapon to use when no level weapon is picked up
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Weapons")
	TSubclassOf<ABaseWeapon> DefaultWeaponClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interact|Trace")
	float TraceRadius = 50.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interact|Trace")
	bool FindFirstInteractive = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interact|Trace", meta = (EditCondition = "!FindFirstInteractive"))
	bool FavourEnvironmentInteractives = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interact|Debug")
	bool ShowDebug = false;

public:	
	// Sets default values for this component's properties
	UCharacterInteractionComponent();

	class AInteractive* AttemptInteract();
	bool HoldInteractive(AInteractive* interactive, const FVector& localOffset = FVector::ZeroVector, bool forceDrop = false);
	void DropInteractive();
	void ThrowInteractive(UPrimitiveComponent* body, const FVector& impulse, const FVector& radialImpulse);
	bool AttemptAttack();
	bool AttemptDashAttack();
	void StopAttacking(bool forced = false);
	bool WantsToAttack() const { return mWantsToAttack; }

	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	FORCEINLINE AInteractive* GetHeldInteractive() const { return mHeldInteractive; }
	FORCEINLINE ABaseWeapon* GetHeldWeapon() const { return mHeldWeapon; }
	FORCEINLINE ABaseWeapon* GetDefaultWeapon() const { return mDefaultWeapon.IsValid() ? mDefaultWeapon.Get() : nullptr; }

	UFUNCTION(BlueprintCallable, Category = "Interact|Character")
	FORCEINLINE AVertCharacter* GetCharacterOwner() const { return mCharacterOwner.Get(); }

protected:
	// Called when the game starts
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type endPlayReason);

private:
	class AInteractive* TraceForSingleInteractive();
	class TArray<AInteractive*> TraceForMultipleInteractives();
	void DrawDebug();

private:
	TWeakObjectPtr<AVertCharacter> mCharacterOwner = nullptr;
	TWeakObjectPtr<ABaseWeapon> mDefaultWeapon = nullptr;
	AInteractive* mHeldInteractive = nullptr;
	ABaseWeapon* mHeldWeapon = nullptr;
	EInteractionState mInteractionState = EInteractionState::Free;
	bool mWantsToAttack = false;
};
