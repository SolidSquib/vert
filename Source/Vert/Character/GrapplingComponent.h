// Copyright Inside Out Games Ltd. 2017

#pragma once

#include "Components/SceneComponent.h"
#include "Components/SphereComponent.h"
#include "Engine/VertGlobals.h"
#include "GrapplingComponent.generated.h"

USTRUCT(BlueprintType)
struct FGrappleConfig
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Config)
	FName GrappleAttachSocket;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config")
	bool SkipReel;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config")
	bool EverCheckForBreak;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config", meta = (EditCondition = "EverCheckForBreak"))
	bool CheckForBreakOnlyIfPulling;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Config)
	int32 MaxGrapples;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Config)
	ERechargeRule RechargeMode;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Config)
	bool RecieveChargeOnGroundOnly;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Config)
	EAimFreedom AimFreedom;

	/* Set this to true to make the grapple pull the character as soon as it contacts a surface, instead of waiting for a second activation. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Config)
	bool PullOnContact;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Config)
	float LineCutLength;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Config)
	float MaxLineLength;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Config)
	float LaunchSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Config)
	float PullSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Config, meta = (EditCondition = "!SkipReel"))
	float ReelSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Config)
	float BreakLeeway;

	FGrappleConfig()
	{
		GrappleAttachSocket = NAME_None;
		SkipReel = true;
		EverCheckForBreak = true;
		CheckForBreakOnlyIfPulling = false;
		MaxGrapples = 3;
		RechargeMode = ERechargeRule::OnRechargeTimer;
		RecieveChargeOnGroundOnly = false;
		AimFreedom = EAimFreedom::Free;
		PullOnContact = false;
		LineCutLength = 150.f;
		MaxLineLength = 1000.f;
		LaunchSpeed = 2000.f;
		PullSpeed = 900.f;
		ReelSpeed = 2500.f;
		BreakLeeway = 20.f;
	}
};

USTRUCT()
struct FGrappledLedgeData
{
	GENERATED_BODY()

	UPROPERTY()
	FHitResult ForwardHit;

	UPROPERTY()
	FHitResult DownwardHit;

	UPROPERTY()
	bool IsLedge;

	FGrappledLedgeData()
	{
		IsLedge = false;
	}
};

USTRUCT(BlueprintType)
struct FPhysicsGrappleConfig
{
	GENERATED_BODY()

	/* Determines how stiff or springy the cable is; higher values mean a stiffer cable and vice-versa */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = Config, meta = (DisplayName = "Stiffness (k)"))
	float LineSpringCoefficient;

	/* Determines how quickly the effects of the cable's spring physics 'wear down'. A Higher value means the spring will come to a stop quicker, but values that are too high will likely cause issues... */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = Config, meta = (DisplayName = "Spring Damping (b)"))
	float LineDampingCoefficient;

	/* Whether we should move the character up slightly when leaving a grapple to help with getting over ledges. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Config)
	bool OffsetCharacterHeightOnCutLine;

	/* Determines whether the line should behave as a string or a spring. Strings will only correct themselves if they are too long, springs will bounce in a more uniform manner. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = Config)
	bool StringContraint;

	FPhysicsGrappleConfig()
	{
		LineSpringCoefficient = 1.f;
		LineDampingCoefficient = 0.1f;
		OffsetCharacterHeightOnCutLine = false;
		StringContraint = true;
	}
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FHookEventDelegate);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FHookGrappledLedgeDelegate, const FHitResult&, forwardTrace, const FHitResult&, downwardTrace);

USTRUCT()
struct FHookedActorData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = Hooked)
	TWeakObjectPtr<AActor> Actor;

	UPROPERTY(BlueprintReadOnly, Category = Hooked)
	TWeakObjectPtr<UPrimitiveComponent> Component;

	UPROPERTY(BlueprintReadOnly, Category = Hooked)
	FVector LocalOffset;

	FHookedActorData()
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

UCLASS(ClassGroup = (Character), meta = (BlueprintSpawnableComponent))
class VERT_API UGrapplingComponent : public USceneComponent
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

	UPROPERTY(BlueprintAssignable, Category = "Grappling")
	FHookEventDelegate OnHookBreak;

	UPROPERTY(BlueprintAssignable, Category = "Grappling")
	FHookEventDelegate OnStartPulling;

	UPROPERTY(BlueprintAssignable, Category = "Grappling")
	FHookGrappledLedgeDelegate OnGrappleToLedgeTransition;

public:
	/* Should we ignore all the fancy stuff and just use LaunchCharacter? */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Config")
	bool UsePhysicsForSwing = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Config")
	bool UsePhysicsForPull = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Config")
	bool SnapGrappleAim = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Config", meta = (EditCondition = "SnapGrappleAim"))
	float TraceRadius = 100.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Config", meta = (EditCondition = "SnapGrappleAim"))
	TEnumAsByte<ECollisionChannel> TraceChannel = ECC_SphereTracer;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Config", meta = (EditCondition = "SnapGrappleAim"))
	bool ShowTraceDebug = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Config")
	FGrappleConfig GrappleConfig;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Config", meta = (EditCondition = "UsePhysicsForSwing || UsePhysicsForPull"))
	FPhysicsGrappleConfig PhysicsGrappleConfig;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Audio")
	class UAkAudioEvent* FireSound = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Audio")
	UAkAudioEvent* LatchSound = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Audio")
	UAkAudioEvent* UnlatchSound = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Audio")
	UAkAudioEvent* SheathSound = nullptr;

protected:
	UPROPERTY()
	class USkeletalMeshComponent* MeshComponent;

	UPROPERTY()
	USphereComponent* SphereComponent;

	UPROPERTY()
	class UProjectileMovementComponent* ProjectileComponent;

	UPROPERTY()
	class UParticleSystemComponent* GrappleBeamPSC = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Effects")
	class UParticleSystem* GrappleBeamFX = nullptr;

	UPROPERTY()
	AActor* AnchorActor = nullptr;

public:
	UGrapplingComponent();

	void OnLanded();
	void SetGrappleColour(const FLinearColor& newColour);

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	FORCEINLINE const TWeakObjectPtr<class AVertCharacterBase>& GetCharacterOwner() const { return mCharacterOwner; }
	FORCEINLINE USkeletalMeshComponent* GetMesh() const { return MeshComponent; }

	UFUNCTION(BlueprintCallable, Category = "Grappling")
	bool Reset();

	UFUNCTION(BlueprintCallable, Category = "Grappling")
	bool ExecuteGrapple(FVector& aimDirection);

	UFUNCTION(BlueprintCallable, Category = "Grappling")
	bool StartPulling();

	FORCEINLINE UFUNCTION(BlueprintCallable)
	AActor* GetHookedActor() const { return mHookAttachment.Actor.Get(); }

	FORCEINLINE UFUNCTION(BlueprintCallable)
	UPrimitiveComponent* GetHookedPrimitive() const { return mHookAttachment.Component.Get(); }

	FORCEINLINE UFUNCTION(BlueprintCallable, Category = "Grappling")
	float GetLineLength() const { return mDistanceFromLauncher; }

	FORCEINLINE UFUNCTION(BlueprintCallable, Category = "Grappling")
	FVector GetVectorToHook() const { return SphereComponent->GetComponentLocation() - GetComponentLocation(); }

	FORCEINLINE UFUNCTION(BlueprintCallable, Category = Grappling)
	FVector GetHookLocation() const { return SphereComponent->GetComponentLocation(); }

	FORCEINLINE UFUNCTION(BlueprintCallable)
	float GetActualLineLength() const { return (SphereComponent->GetComponentLocation()- GetComponentLocation()).Size(); }

	FORCEINLINE UFUNCTION(BlueprintCallable)
	float GetActualLineLengthSqr() const { return (SphereComponent->GetComponentLocation() - GetComponentLocation()).SizeSquared(); }

	FORCEINLINE UFUNCTION(BlueprintCallable)
	void LockDesiredLineLength() { mDistanceFromLauncher = GetActualLineLength(); }

	UFUNCTION(BlueprintCallable, Category = "Grappling")
	FORCEINLINE FVector GetLineDirection() const { return GetOwner() ? (SphereComponent->GetComponentLocation() - GetOwner()->GetActorLocation()).GetSafeNormal() : FVector::ZeroVector; }

	UFUNCTION(BlueprintCallable, Category = "Usage")
	FORCEINLINE int32 GetRemainingGrapples() const { return mRemainingGrapples; }

	UFUNCTION(BlueprintCallable, Category = "States")
	FORCEINLINE EGrappleState GetGrappleState() const { return mGrappleState; }

	UFUNCTION(BlueprintCallable, Category = "States")
	FORCEINLINE bool IsGrappleDeployed() const { return mGrappleState == EGrappleState::HookDeployed || mGrappleState == EGrappleState::HookDeployedAndReturning; }
	
	FORCEINLINE UFUNCTION(BlueprintCallable, Category = "Grappling")
	float GetRechargePercent() const { return mRechargeTimer.GetProgressPercent(); }

	FORCEINLINE UFUNCTION(BlueprintCallable, Category = "Grappling")
	bool IsRecharging() const { return mRechargeTimer.IsRunning(); }

	UFUNCTION(BlueprintNativeEvent)
	void OnHit(class UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	UFUNCTION(BlueprintNativeEvent)
	void OnBeginOverlap(class UPrimitiveComponent* overlappedComp, AActor* otherActor, UPrimitiveComponent* otherComp, int32 otherBodyIndex, bool fromSweep, const FHitResult& sweepResult);

protected:
	void DetachProjectile();
	void ReattachProjectile();
	UProjectileMovementComponent* RecreateProjectileVelocity(AActor* owner);
	void DisableAndDestroyAnchor();

	// Called when the game starts
	virtual void BeginPlay() override;

	UFUNCTION(BlueprintNativeEvent, Category = "Recharging")
	void OnGrappleRechargeTimerFinished();

private:
	void UpdateRechargeState();
	void ActivateHookCollision();
	void DeactivateHookCollision();
	void StartReeling();
	void TickReel(float DeltaSeconds);
	void TickHookDeployed(float DeltaSeconds);
	void TickHookDeployedAndReturning(float DeltaSeconds);
	void SheatheHook();
	void DeployHook(AActor* OtherActor, UPrimitiveComponent* OtherComp, const FHitResult& hit, bool attachToTarget = true);
	void DetatchHook();
	bool IsLineDepleted(float DeltaSeconds);
	FVector FindBestGrappleLocation(const FVector& projectedAim);
	bool FindLedgeFromTraceResults(FGrappledLedgeData& outGrappleTargetDate, FVector& outProjectedAim, const FHitResult& hit, const TArray<AActor*>& ignoreActors, const TArray<UPrimitiveComponent*>& ignoreComponents);
	void FindIgnoreActorsAndComponents(const TArray<FHitResult>& hitResults, TArray<AActor*>& outActors, TArray<UPrimitiveComponent*>& outComponents, const AActor* excludeActor = nullptr, const UPrimitiveComponent* excludeComponent = nullptr);
	void CheckForGrappleBreak();

private:
	TWeakObjectPtr<class AVertCharacterBase> mCharacterOwner = nullptr;

	bool mGeometryBlockingGrab = false;
	float mDistanceFromLauncher = 0.f;
	int32 mRemainingGrapples = 0;
	EGrappleState mGrappleState = EGrappleState::HookSheathed;
	FVertTimer mRechargeTimer;
	FHookedActorData mHookAttachment;

	// Ledge grappling members
	FGrappledLedgeData mLedgeData;
};
