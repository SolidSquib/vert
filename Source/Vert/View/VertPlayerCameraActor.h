// Copyright Inside Out Games Ltd. 2017

#pragma once

#include "GameFramework/Actor.h"
#include "VertPlayerCameraActor.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogVertPlayerCamera, Log, All);

UCLASS(BlueprintType, Blueprintable)
class VERT_API AVertPlayerCameraActor : public AActor
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Camera)
	float InterpSpeed = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Zoom)
	float MaxArmLength = 1000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Zoom)
	float MinArmLength = 30.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Config)
	bool ConstantVelocity;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = Bounds)
	bool LockPawnsToBounds = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Bounds, meta = (EditCondition = "LockPawnToBounds"))
	float MaximumDistance = 100.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Spline)
	class USplineComponent* CameraSpline = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Spline)
	USplineComponent* PlayerSpline = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Spline)
	int SplineIterationMax = 10;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Spline)
	float DistanceThreshold = 60.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Spline, meta = (UIMin = 0.0, UIMax = 1.0))
	float TimeDifferenceThreshold = 0.75f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Debug)
	bool ShowDebugInfo = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug", meta = (UIMin = 0.0, UIMax = 1.0, EditCondition = "ShowDebugInfo"))
	float CameraDebugTime = 0.f;

protected:
	UPROPERTY()
	class USceneComponent* SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class UCameraComponent* CameraComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class USpringArmComponent* CameraBoom;

public:	
	// Sets default values for this actor's properties
	AVertPlayerCameraActor();

	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UFUNCTION(BlueprintCallable, Category = "PlayerPosition")
	void RegisterPlayerPawn(class APawn* pawnToFollow);

	UFUNCTION(BlueprintCallable, Category = "PlayerPosition")
	void UnregisterPlayerPawn(class APawn* pawnToFollow);

	UFUNCTION(BlueprintCallable)
	void OverridePawnFollow(int32 pawnIndex);

	UFUNCTION(BlueprintCallable)
	void OverrideCameraZoom(int32 cameraZoomAmount);

protected:
	void UpdateCameraPositionAndZoom(FVector& meanLocation, FVector& largestDistance, float& hLengthSqr, float& vLengthSqr);
	void UpdateCameraZoom(float hLengthSqr, float vLengthSqr);
	void CorrectPawnPositions(const FVector& largestDistance);

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

private:
	// Debug functions
	void ScrubCameraTime();
	void SetupDebugNumbers();
	void AddSplineNumbers(USplineComponent* spline);
	void DebugSplineMovement();

	// Spline functions
	float RecursiveDistanceCheck(int iteration, float startTime, float endTime);

	// Update functions
	FVector UpdateDesiredTime(float DeltaTime);
	void UpdateCamera(float DeltaTime, const FVector& cameraDesiredLocation);

private:
	int32 mPawnOverride = -1;
	int32 mZoomOverride = -1;
	TArray<class APawn*> mPawnsToFollow;
	TArray<AVertPlayerController*> mPlayerControllers;
	float mSplineDesiredTime;
	float mSplineCurrentTime;
};
