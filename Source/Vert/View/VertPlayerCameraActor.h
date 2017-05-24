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

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = Bounds)
	bool LockPawnsToBounds = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Bounds, meta = (EditCondition = "LockPawnToBounds"))
	float MaximumDistance = 100.f;

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
	void UpdateCameraPositionAndZoom(FVector& meanLocation, FVector& largestDistance, float& hLength, float& vLength);
	void CorrectPawnPositions(const FVector& largestDistance);

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

private:
	int32 mPawnOverride = -1;
	int32 mZoomOverride = -1;
	TArray<class APawn*> mPawnsToFollow;
	TArray<AVertPlayerController*> mPlayerControllers;
};
