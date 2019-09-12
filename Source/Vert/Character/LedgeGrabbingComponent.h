// Copyright Inside Out Games Ltd. 2017

#pragma once

#include "Components/SphereComponent.h"
#include "Engine/Debuggable.h"
#include "LedgeGrabbingComponent.generated.h"

UENUM()
enum class ELedgeTransition : uint8 
{
	GrabLedge,
	ClimbUpLedge,
	JumpAwayFromGrabbedLedge,
	LaunchFromGrabbedLedge,
	AttackFromGrabbedLedge,
	DamagedOnGrabbedLedge,
	DropFromGrabbedLedge,
	NONE
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLedgeTransitionDelegate, ELedgeTransition, transition);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnLedgeGrabbedDelegate, const FHitResult&, forwardHit, const FHitResult&, downwardHit);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FLedgeGrabbingDelegate);

UCLASS(ClassGroup=(CharacterComponents), meta=(BlueprintSpawnableComponent))
class VERT_API ULedgeGrabbingComponent : public USphereComponent
{
	GENERATED_BODY()

	friend class AVertCharacter;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ignore")
	TArray<AActor*> ActorsToIgnore;

	UPROPERTY(BlueprintAssignable)
	FOnLedgeTransitionDelegate OnLedgeTransition;

	UPROPERTY(BlueprintAssignable)
	FOnLedgeGrabbedDelegate HoldingLedge;

	UPROPERTY(BlueprintAssignable)
	FLedgeGrabbingDelegate WantsToAttack;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
	bool ShowDebug = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	struct FVertTimer InputDelayTimer;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Audio")
	class UAkAudioEvent* ClimbSound = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Audio")
	class UAkAudioEvent* GrabLedgeSound = nullptr;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LedgeDetection")
	bool Enable = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LedgeDetection")
	bool UseRootMotion = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Range")
	FName HipSocket = NAME_None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Range")
	float GrabHeightOffset = 0.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Range")
	float HipHeightThreshold = -50.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Range")
	float ForwardRange = 150.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Range")
	float UpwardTraceZStartOffset = 500.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Range")
	float UpwardTraceForwardOffset = 70.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Range")
	float UpwardRange = 550.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Range")
	float TraceRadius = 20.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Channel")
	TEnumAsByte<ECollisionChannel> TraceChannel;

public:	
	// Sets default values for this component's properties
	ULedgeGrabbingComponent();

	void TransitionLedge(ELedgeTransition transition);
	TEnumAsByte<ECollisionChannel> GetTraceChannel() const { return TraceChannel; }
	float GetTraceRadius() const { return TraceRadius; }
	float GetForwardRange() const { return ForwardRange; }

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintCallable)
	FVector GetLedgeDirection(EAimFreedom freedom = EAimFreedom::Free) const;

	UFUNCTION(BlueprintCallable)
	void CancelLaunchFromLedge();

	UFUNCTION(BlueprintCallable)
	FORCEINLINE bool IsClimbingLedge() const { return mClimbingLedge; }

	UFUNCTION(BlueprintCallable)
	FORCEINLINE bool IsLaunchingFromLedge() const { return mLaunchingFromLedge; }

	UFUNCTION(BlueprintCallable)
	FORCEINLINE bool IsTransitioning() const { return mTransitioning; }

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnBeginOverlap(UPrimitiveComponent* overlappedComp, AActor* otherActor, UPrimitiveComponent* otherComp, int32 otherBodyIndex, bool fromSweep, const FHitResult& sweepResult);

	UFUNCTION()
	void OnEndOverlap(UPrimitiveComponent* overlappedComp, AActor* otherActor, UPrimitiveComponent* otherComp, int32 otherBodyIndex);

	UFUNCTION()
	void EndTransition();

	UFUNCTION(BlueprintCallable)
	void DropLedge();

private:
	bool ShouldGrabLedge(const FVector& ledgeHeight) const;
	bool InGrabbingRange(const FVector& ledgeHeight) const;
	bool PerformLedgeTrace(const FVector& start, const FVector& end, FHitResult& hit);
	bool TraceForForwardLedge(FHitResult& hit);
	bool TraceForUpwardLedge(FHitResult& hit);
	void GrabLedge(const FHitResult& forwardHit, const FHitResult& downwardHit, bool freshGrab = true);
	FVector GetHipLocation() const;
	void LerpToLedge(float deltaTime);

private:
	bool mCanTrace = false;
	bool mClimbingLedge = false;
	bool mTransitioning = false;
	bool mLerping = false;
	bool mLaunchingFromLedge = false;
	bool mWantsAttack = false;
	int32 mNumOverlaps = 0;
	FVector mLerpTarget = FVector::ZeroVector;
	FVector mLastGrabLedgeNormal = FVector::ZeroVector;
	FVector mLastLedgeHeight = FVector::ZeroVector;
	TWeakObjectPtr<class ACharacter> mCharacterOwner = nullptr;
	TWeakObjectPtr<class ULedgeComponent> mCurrentLedge = nullptr;
	FTimerHandle mTimerHandle_Launching;
	FTimerHandle mTimerHandle_GrabBlock;	
};
