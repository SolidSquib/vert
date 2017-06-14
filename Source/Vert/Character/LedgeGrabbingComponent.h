// Copyright Inside Out Games Ltd. 2017

#pragma once

#include "Components/SphereComponent.h"
#include "Engine/Debuggable.h"
#include "LedgeGrabbingComponent.generated.h"

UENUM()
enum class ELedgeTransition : uint8 
{
	Climb,
	JumpAway,
	Launch,
	Attack,
	Damaged,
	Drop
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLedgeTransitionDelegate, ELedgeTransition, transition);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnLedgeGrabbedDelegate, const FHitResult&, forwardHit, const FHitResult&, downwardHit);

UCLASS(ClassGroup=(CharacterComponents), meta=(BlueprintSpawnableComponent))
class VERT_API ULedgeGrabbingComponent : public USphereComponent, public IDebuggable
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ignore")
	TArray<AActor*> ActorsToIgnore;

	UPROPERTY(BlueprintAssignable)
	FOnLedgeTransitionDelegate OnLedgeTransition;

	UPROPERTY(BlueprintAssignable)
	FOnLedgeGrabbedDelegate OnLedgeGrabbed;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	struct FVertTimer InputDelayTimer;

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

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintCallable)
	FVector GetLedgeDirection(EAimFreedom freedom = EAimFreedom::Free) const;

	UFUNCTION(BlueprintCallable)
	FORCEINLINE bool IsClimbingLedge() const { return mClimbingLedge; }

protected:
	void DropLedge();

	virtual void DrawDebugInfo() override;
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnBeginOverlap(UPrimitiveComponent* overlappedComp, AActor* otherActor, UPrimitiveComponent* otherComp, int32 otherBodyIndex, bool fromSweep, const FHitResult& sweepResult);

	UFUNCTION()
	void OnEndOverlap(UPrimitiveComponent* overlappedComp, AActor* otherActor, UPrimitiveComponent* otherComp, int32 otherBodyIndex);

	UFUNCTION()
	void StopLerping();

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
	int32 mNumOverlaps = 0;
	FVector mLerpTarget = FVector::ZeroVector;
	FVector mLastGrabLedgeNormal = FVector::ZeroVector;
	FVector mLastLedgeHeight = FVector::ZeroVector;
	TWeakObjectPtr<class ACharacter> mCharacterOwner = nullptr;
};
