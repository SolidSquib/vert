// Copyright Inside Out Games Ltd. 2017

#pragma once

#include "Components/SphereComponent.h"
#include "Engine/Debuggable.h"
#include "LedgeGrabbingComponent.generated.h"

UCLASS( ClassGroup=(CharacterComponents), meta=(BlueprintSpawnableComponent) )
class VERT_API ULedgeGrabbingComponent : public USphereComponent, public IDebuggable
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ignore")
	TArray<AActor*> ActorsToIgnore;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LedgeDetection")
	bool Enable = true;

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

	void DropLedge();
	void ClimbLedge();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void PostInitProperties() override;

	UFUNCTION(BlueprintCallable)
	FVector GetLedgeDirection(EAimFreedom freedom = EAimFreedom::Free) const;

	UFUNCTION(BlueprintCallable)
	FORCEINLINE bool IsClimbingLedge() const { return mClimbingLedge; }

protected:
	virtual void DrawDebugInfo() override;

	UFUNCTION()
	void OnBeginOverlap(UPrimitiveComponent* overlappedComp, AActor* otherActor, UPrimitiveComponent* otherComp, int32 otherBodyIndex, bool fromSweep, const FHitResult& sweepResult);

	UFUNCTION()
	void OnEndOverlap(UPrimitiveComponent* overlappedComp, AActor* otherActor, UPrimitiveComponent* otherComp, int32 otherBodyIndex);

private:
	bool PerformLedgeTrace(const FVector& start, const FVector& end, FHitResult& hit);
	bool TraceForForwardLedge(FHitResult& hit);
	bool TraceForUpwardLedge(FHitResult& hit);
	void GrabLedge(const FVector& wallImpactPoint, const FVector& wallImpactNormal, const FVector& ledgeHeight);

private:
	bool mCanTrace = false;
	bool mClimbingLedge = false;
	FVector mLastGrabLedgeNormal = FVector::ZeroVector;
	TWeakObjectPtr<class ACharacter> mCharacterOwner = nullptr;
};
