// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameFramework/Actor.h"
#include "GrappleHook.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnHookedDelegate);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnFiredDelegate);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnReturnedDelegate);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLatchChangeDelegate, AGrappleHook*, hook);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnPullDelegate, AGrappleHook*, hook, const FVector&, pullDirection, const float, lineLength);

USTRUCT()
struct FHookedActorInfo
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = Hooked)
	TWeakObjectPtr<AActor> Actor;

	UPROPERTY(BlueprintReadOnly, Category = Hooked)
	TWeakObjectPtr<UPrimitiveComponent> Component;

	UPROPERTY(BlueprintReadOnly, Category = Hooked)
	FVector LocalOffset;

	FHookedActorInfo()
	{
		Actor = nullptr;
		Component = nullptr;
		LocalOffset = FVector::ZeroVector;
	}
};

UENUM()
enum class EGrappleState : uint8
{
	None,
	Launching,
	Hooked,
	Latched,
	Reeling,
	Sheathed
};

UCLASS()
class VERT_API AGrappleHook : public AActor
{
	GENERATED_BODY()
	
public:
	UPROPERTY(BlueprintAssignable, Category = "Grappling")
	FOnHookedDelegate OnHooked;

	UPROPERTY(BlueprintAssignable, Category = "Grappling")
	FOnFiredDelegate OnFired;

	UPROPERTY(BlueprintAssignable, Category = "Grappling")
	FOnReturnedDelegate OnReturned;

	UPROPERTY(BlueprintAssignable, Category = "Grappling")
	FOnPullDelegate OnPull;

	UPROPERTY(BlueprintAssignable, Category = "Grappling")
	FOnLatchChangeDelegate OnLatched;

	UPROPERTY(BlueprintAssignable, Category = "Grappling")
	FOnLatchChangeDelegate OnUnLatched;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Sprite")
	class UPaperFlipbookComponent* Sprite;

	UPROPERTY(VisibleDefaultsOnly)
	class USphereComponent* SphereCollider;

	UPROPERTY(VisibleAnywhere, Category = Projectile)
	class UProjectileMovementComponent* ProjectileMovement;

	UPROPERTY(EditDefaultsOnly, Category = Projectile)
	class UParticleSystem* ImpactParticleTemplate;

	UPROPERTY(EditDefaultsOnly, Category = Sound)
	class USoundBase* ImpactSound;

public:
	// Sets default values for this actor's properties
	AGrappleHook();

	void Launch(const FVector& fireDirection);
	void Reel();
	void Sheathe();
	void Hook(AActor* OtherActor, UPrimitiveComponent* OtherComp, const FHitResult& hit, bool attachToTarget = true);
	void UnHook();
	void Pull();
	class AGrappleLauncher* GetOwnerAsGrappleLauncher() const;
	FVector GetHookVelocity() const;
	bool GetProjectileMovementComponentIsActive() const;

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	FORCEINLINE UPaperFlipbookComponent* GetSprite() const { return Sprite; }
	FORCEINLINE bool GetIsActive() const { return mIsActive; }
	FORCEINLINE EGrappleState GetGrappleState() const { return mGrappleState; }

	UFUNCTION()
	void OnHit(class UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	UFUNCTION()
	void OnBeginOverlap(class UPrimitiveComponent* overlappedComp, AActor* otherActor, UPrimitiveComponent* otherComp, int32 otherBodyIndex, bool fromSweep, const FHitResult& sweepResult);

	UFUNCTION(BlueprintCallable, Category = "Grappling")
	FORCEINLINE AActor* GetHookedActor() const { return mHookAttachment.Actor.Get(); }

private:
	void ActivateHookCollision();
	void DeactivateHookCollision();

protected:
	TWeakObjectPtr<class AVertCharacter> mCharacterOwner = nullptr;
	TWeakObjectPtr<class UGrapplingComponent> mGrapplingComponent = nullptr;
	TWeakObjectPtr<class AGrappleLauncher> mLauncherOwner = nullptr;

	FHookedActorInfo mHookAttachment;
	EGrappleState mGrappleState = EGrappleState::Sheathed;
	bool mIsActive = false;
};
