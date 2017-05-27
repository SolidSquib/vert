// Copyright Inside Out Games Ltd. 2017

#pragma once

#include "GameFramework/Actor.h"
#include "VertPlayerCameraActor.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogVertPlayerCamera, Log, All);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCameraPointReachedDelegate, const TArray<APawn*>&, pawns);

UCLASS(BlueprintType, Blueprintable)
class VERT_API AVertPlayerCameraActor : public AActor
{
	GENERATED_BODY()
	
public:
	UPROPERTY(BlueprintAssignable, Category = "Spline|Events")
	FOnCameraPointReachedDelegate OnCameraReachEndOfTrack;

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnCameraPointReachedDelegate OnCameraBecomeActive;

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnCameraPointReachedDelegate OnCameraBecomeInactive;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Camera)
	float InterpSpeed = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Zoom)
	float MaxArmLength = 3000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Zoom)
	float MinArmLength = 1000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Config)
	bool ConstantVelocity = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Config)
	bool LookAtTarget = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = Bounds)
	bool LockPawnsToBounds = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Bounds, meta = (EditCondition = "LockPawnToBounds"))
	float MaximumDistance = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Spline)
	class ASplineActor* CameraSpline = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Spline)
	ASplineActor* PlayerSpline = nullptr;

	// Set whether this camera should lock it's position to the spline's desired X-axis
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spline|AxisLock")
	bool LockX = true;

	// Set whether this camera should lock it's position to the spline's desired Y-axis
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spline|AxisLock")
	bool LockY = true;

	// Set whether this camera should lock it's position to the spline's desired Z-axis
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spline|AxisLock")
	bool LockZ = true;

	// Set the freedom granted to the camera in any one direction
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spline|AxisLock", Meta = (EditCondition = "!LockX || !LockY || !LockZ"))
	float SplineFreedom = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Spline)
	int SplineIterationMax = 10;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Spline)
	float DistanceThreshold = 60.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Spline, meta = (UIMin = 0.0, UIMax = 1.0))
	float TimeDifferenceThreshold = 0.75f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Spline)
	bool StopAtEnd = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Debug)
	bool ShowDebugInfo = false;

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

	void ActivateCamera();
	void DeactivateCamera();

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
	void UpdateTargetCameraPosition();
	void UpdateCameraZoom(float deltaTime, const FVector& zoomTarget);
	void CorrectPawnPositions(const FVector& largestDistance);

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

private:
	// Debug functions
	void SetupDebugNumbers();
	void AddSplineNumbers(ASplineActor* spline);
	void DebugSplineMovement();

	// Spline functions
	float RecursiveDistanceCheck(int iteration, float startTime, float endTime);
	FVector MakePositionVectorForSpline(const FVector& desiredSplineLocation);

	// Update functions
	FVector UpdateDesiredTime(float DeltaTime);
	void UpdateCamera(float DeltaTime);
	void UpdateCamera(float DeltaTime, const FVector& cameraDesiredLocation);

private:
	int32 mPawnOverride = -1;
	int32 mZoomOverride = -1;
	FVector mTargetLocation = FVector::ZeroVector;
	TArray<class APawn*> mPawnsToFollow;
	TArray<AVertPlayerController*> mPlayerControllers;
	float mSplineDesiredTime;
	float mSplineCurrentTime;
	bool mHasReachedEnd = false;
};
