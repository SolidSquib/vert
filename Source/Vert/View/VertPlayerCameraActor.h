// Copyright Inside Out Games Ltd. 2017

#pragma once

#include "GameFramework/Actor.h"
#include "VertPlayerCameraActor.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogVertPlayerCamera, Log, All);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCameraPointReachedDelegate, class AVertPlayerCameraActor*, Camera);

UENUM(BlueprintType)
enum class ELevelType : uint8
{
	LT_RACE UMETA(DisplayName = "Race Course"),
	LT_ARENA UMETA(DisplayName = "Player Battle"),
	LT_BOSS UMETA(DisplayName = "Boss Battle"),
	LT_FINALE UMETA(DisplayName = "Finale"),
	LT_MAX UMETA(DisplayName = "None")
};

UCLASS(BlueprintType, Blueprintable)
class VERT_API AVertPlayerCameraActor : public AActor
{
	GENERATED_BODY()
	
public:
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnCameraPointReachedDelegate OnCameraReachEndOfTrack;

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnCameraPointReachedDelegate OnCameraBecomeActive;

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnCameraPointReachedDelegate OnCameraBecomeInactive;

public:
	UPROPERTY(EditAnywhere, Category = "Levels")
	ELevelType LevelType = ELevelType::LT_MAX;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Config)
	bool IgnorePlayers = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Config)
	float InterpSpeed = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Config)
	float MaxArmLength = 3000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Config)
	float MinArmLength = 1000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Config)
	bool ConstantVelocity = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Config)
	bool LookAtTarget = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Config|Bounds")
	bool LockPawnsToBounds = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Bounds", meta = (EditCondition = "LockPawnsToBounds"))
	float MaximumDistance = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Config|Spline")
	class ASplineActor* CameraSpline = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Config|Spline")
	ASplineActor* PlayerSpline = nullptr;
	
	// Set whether this camera should lock it's position to the spline's desired X-axis
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Config|Spline")
	bool LockX = true;

	// Set whether this camera should lock it's position to the spline's desired Y-axis
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Config|Spline")
	bool LockY = true;

	// Set whether this camera should lock it's position to the spline's desired Z-axis
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Config|Spline")
	bool LockZ = true;

	// Set the freedom granted to the camera in the X direction
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Config|Spline", Meta = (EditCondition = "!LockX"))
	float SplineXFreedom = 0.f;

	// Set the freedom granted to the camera in the Y direction
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Config|Spline", Meta = (EditCondition = "!LockY"))
	float SplineYFreedom = 0.f;

	// Set the freedom granted to the camera in the Z direction
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Config|Spline", Meta = (EditCondition = "!LockZ"))
	float SplineZFreedom = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config|Spline")
	bool IsAutoSpline = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config|Spline", meta = (EditCondition = "!OnTimer"))
	bool NotifyReachedEndOnManualSpline = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
	bool OnTimer = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config", meta = (EditCondition = "OnTimer"))
	float TimerValue = 30.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config|Spline", meta = (EditCondition = "IsAutoSpline"))
	float AutoSplineSpeed = 0.1f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config|Spline", meta = (EditCondition = "IsAutoSpline"))
	bool FollowTheLeader = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config|Spline", meta = (EditCondition = "FollowTheLeader"))
	float LeaderFollowThreshold = 0.1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config|Spline", meta = (EditCondition = "FollowTheLeader"))
	float FollowDiscrepancyThreshold = 0.1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config|Spline")
	int SplineIterationMax = 10;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config|Spline")
	float DistanceThreshold = 60.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config|Spline", meta = (UIMin = 0.0, UIMax = 1.0))
	float TimeDifferenceThreshold = 0.75f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config|Spline")
	bool StopAtEnd = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Config|Bounds")
	bool KillAtEndOfBounds = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Debug)
	bool ShowDebugInfo = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config|Transition")
	float NextTransitionTime = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config|Transition")
	bool AutoTransition = true;

protected:
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "collision")
	class USphereComponent* SphereCollision;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class UCameraComponent* CameraComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class USpringArmComponent* CameraBoom;

	UPROPERTY()
	class UAkComponent* WwiseAudioComponent = nullptr;

public:	
	// Sets default values for this actor's properties
	AVertPlayerCameraActor();

	void ActivateCamera();
	void DeactivateCamera();
	void SetArmLength(float armLength);

	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UFUNCTION(BlueprintCallable)
	void OverridePawnFollow(int32 pawnIndex);

	UFUNCTION(BlueprintCallable)
	void OverrideCameraZoom(int32 cameraZoomAmount);

	UFUNCTION(BlueprintCallable)
	ELevelType GetLevelType() const { return LevelType; }

	UFUNCTION(BlueprintCallable)
	float GetCurrentArmLength() const;

	UFUNCTION(BlueprintCallable)
	bool IsCameraActive() const { return mIsCameraActive; }

protected:
	void UpdateTargetCameraPosition();
	void UpdateCameraZoom(float deltaTime, const FVector& zoomTarget);
	void CorrectPawnPositions(const FVector& largestDistance);

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	virtual void PostLoad() override;

	UFUNCTION(BlueprintCallable, Category = "CameraSpline")
	void SetCameraSplineSpeed(float newSpeed);

	UFUNCTION(BlueprintCallable, Category = "CameraSpline")
	void SetLeaderFollowThreshold(float newThreshold);

	UFUNCTION()
	void OnLeaveKillBounds(UPrimitiveComponent* overlappedComponent, AActor* otherActor, UPrimitiveComponent* otherComp, int32 otherBodyIndex);

	UFUNCTION()
	void OnTimerExpired();

private:
	// Debug functions
	void SetupDebugNumbers();
	void AddSplineNumbers(ASplineActor* spline);
	void DebugSplineMovement();

	// Spline functions
	float RecursiveDistanceCheck(int iteration, float startTime, float endTime, FVector target);
	FVector MakePositionVectorForSpline(const FVector& desiredSplineLocation);

	// Update functions
	FVector UpdateDesiredTime(float DeltaTime);
	void UpdateCamera(float DeltaTime);
	void UpdateCamera(float DeltaTime, const FVector& cameraDesiredLocation);

private:
	int32 mActorOverride = -1;
	int32 mZoomOverride = -1;
	FVector mTargetLocation = FVector::ZeroVector;
	TWeakObjectPtr<class AVertGameMode> mActiveGameMode = nullptr;
	float mSplineDesiredTime;
	float mSplineCurrentTime;
	float mActualSplineSpeed = 0.f;
	float mActualLeaderThreshold = 0.f;
	bool mHasReachedEnd = false;
	bool mIsCameraActive = false;
};
