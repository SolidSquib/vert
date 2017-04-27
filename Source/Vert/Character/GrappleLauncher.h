// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameFramework/Actor.h"
//#include "Character/GrappleHook.h"
#include "GrappleLauncher.generated.h"

UCLASS()
class VERT_API AGrappleLauncher : public AActor
{
	GENERATED_BODY()
	
public:
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Grappling")
	class UCableComponent* Cable;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Grappling")
	TSubclassOf<class AGrappleHook> HookClass;

public:
	// Sets default values for this component's properties
	AGrappleLauncher();

	bool FireGrapple(const FVector& shootDirection, bool wasGamepadTriggered = false);
	bool FireGrapple(const FVector2D& shootDirection, bool wasGamepadTriggered = false);
	bool StartPulling() const;
	void ResetGrapple();

	virtual void BeginPlay() override;

	FORCEINLINE const TWeakObjectPtr<class AGrappleHook>& GetGrappleHook() const { return mGrappleHook; }

	UFUNCTION(BlueprintCallable)
	class AVertCharacter* GetCharacterOwner() const;

protected:
	TWeakObjectPtr<class AGrappleHook> mGrappleHook = nullptr;
	TWeakObjectPtr<class AVertCharacter> mCharacterOwner = nullptr;
	TWeakObjectPtr<class UGrapplingComponent> mGrapplingComponent = nullptr;
};

// Declare all of the hook stuff now

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FHookEventDelegate);

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
	HookLaunching,
	HookDeployed,
	HookDeployedAndReturning,
	Latched,
	HookReturning,
	HookSheathed
};

UCLASS()
class VERT_API AGrappleHook : public AActor
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable, Category = "Grappling")
	FHookEventDelegate OnHooked;

	UPROPERTY(BlueprintAssignable, Category = "Grappling")
	FHookEventDelegate OnFired;

	UPROPERTY(BlueprintAssignable, Category = "Grappling")
	FHookEventDelegate OnReturned;

	UPROPERTY(BlueprintAssignable, Category = "Grappling")
	FHookEventDelegate OnHookReleased;

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
	bool StartPulling();
	void ResetHook();
	
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	FORCEINLINE UPaperFlipbookComponent* GetSprite() const { return Sprite; }
	FORCEINLINE EGrappleState GetGrappleState() const { return mGrappleState; }
	FORCEINLINE float GetCurrentDistanceFromLauncher() const { return mDistanceFromLauncher; }

	UFUNCTION(BlueprintNativeEvent)
	void OnHit(class UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	UFUNCTION(BlueprintNativeEvent)
	void OnBeginOverlap(class UPrimitiveComponent* overlappedComp, AActor* otherActor, UPrimitiveComponent* otherComp, int32 otherBodyIndex, bool fromSweep, const FHitResult& sweepResult);

	UFUNCTION(BlueprintCallable, Category = "Grappling")
	FORCEINLINE AActor* GetHookedActor() const { return mHookAttachment.Actor.Get(); }

private:
	void StartReeling();
	void TickReel(float DeltaSeconds);
	void TickHookDeployed(float DeltaSeconds);
	void TickHookDeployedAndReturning(float DeltaSeconds);
	void SheatheHook();
	void DeployHook(AActor* OtherActor, UPrimitiveComponent* OtherComp, const FHitResult& hit, bool attachToTarget = true);
	void DetatchHook();
	void ActivateHookCollision();
	void DeactivateHookCollision();
	bool IsLineDepleted(float DeltaSeconds);

protected:
	TWeakObjectPtr<class AVertCharacter> mCharacterOwner = nullptr;
	TWeakObjectPtr<class UGrapplingComponent> mGrapplingComponent = nullptr;
	TWeakObjectPtr<class AGrappleLauncher> mLauncherOwner = nullptr;

	FHookedActorInfo mHookAttachment;
	EGrappleState mGrappleState = EGrappleState::HookSheathed;
	bool mIsActive = false;
	float mDistanceFromLauncher = 0.f;
};