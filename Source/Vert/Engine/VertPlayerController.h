// Copyright Inside Out Games Ltd. 2017

#pragma once

#include "GameFramework/PlayerController.h"
#include "VertPlayerController.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogVertPlayerController, Log, All);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPossessedDelegate, APawn*, pawn);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnUnPossessedDelegate, APawn*, pawn);

/**
 * 
 */
UCLASS()
class VERT_API AVertPlayerController : public APlayerController
{
	struct AxisPositions
	{
		float RightX, RightY, LeftX, LeftY;
		FVector MouseDirection;

		AxisPositions()
		{
			RightX = 0;
			RightY = 0;
			LeftX = 0;
			LeftY = 0;
			MouseDirection = FVector::ZeroVector;
		}
	};

	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable, Category = "PawnPossesion")
	FOnPossessedDelegate OnPossessed;

	UPROPERTY(BlueprintAssignable, Category = "PawnPossesion")
	FOnUnPossessedDelegate OnUnPossessed;

public:
	AVertPlayerController();

	void DisplayClientMessage(FString s);
	void DisplayInt(FString label, int32 theInt);
	void DisplayFloat(FString label, float theFloat);
	void DisplayVector(FString label, FVector theVector);
	void DisplayVector2D(FString label, FVector2D theVector);
	void RightThumbstickMoveX(float value);
	void RightThumbstickMoveY(float value);
	void LeftThumbstickMoveX(float value);
	void LeftThumbstickMoveY(float value);
	void MouseMove(float value);

	virtual void DropIn();
	virtual bool CanRestartPlayer() override;
	virtual void Possess(APawn* aPawn) override;
	virtual void UnPossess() override;

	class UVertLocalPlayer* GetVertLocalPlayer();

	FORCEINLINE void ToggleFOV() { UE_LOG(LogVertPlayerController, Warning, TEXT("Toggling FOV for test")); mTestFOV = !mTestFOV; }
	FORCEINLINE bool IsTestingFOV() const { return mTestFOV; }
	
	UFUNCTION(BlueprintCallable, Category = Thumbsticks)
	FORCEINLINE FVector GetPlayerLeftThumbstick() const { return FVector(mAxisPositions.LeftX, 0.f, mAxisPositions.LeftY); }

	UFUNCTION(BlueprintCallable, Category = Thumbsticks)
	FORCEINLINE FVector GetPlayerRightThumbstick() const { return FVector(mAxisPositions.RightX, 0.f, mAxisPositions.RightY); }

	UFUNCTION(BlueprintCallable, Category = Thumbsticks)
	FORCEINLINE FVector2D GetPlayerLeftThumbstick2D() const { return FVector2D(mAxisPositions.LeftX, mAxisPositions.LeftY); }

	UFUNCTION(BlueprintCallable, Category = Thumbsticks)
	FORCEINLINE FVector2D GetPlayerRightThumbstick2D() const { return FVector2D(mAxisPositions.RightX, mAxisPositions.RightY); }

	UFUNCTION(BlueprintCallable, Category = Thumbsticks)
	FORCEINLINE FVector GetPlayerLeftThumbstickDirection() const { return (FVector(mAxisPositions.LeftX, 0.f, mAxisPositions.LeftY) * 100).GetSafeNormal(); }

	UFUNCTION(BlueprintCallable, Category = Thumbsticks)
	FORCEINLINE FVector GetPlayerRightThumbstickDirection() const { return (FVector(mAxisPositions.RightX, 0.f, mAxisPositions.RightY) * 100).GetSafeNormal(); }

	UFUNCTION(BlueprintCallable, Category = Thumbsticks)
	FORCEINLINE FVector2D GetPlayerLeftThumbstickDirection2D() const { return (FVector2D(mAxisPositions.LeftX, mAxisPositions.LeftY) * 100).GetSafeNormal(); }

	UFUNCTION(BlueprintCallable, Category = Thumbsticks)
	FORCEINLINE FVector2D GetPlayerRightThumbstickDirection2D() const { return (FVector2D(mAxisPositions.RightX, mAxisPositions.RightY) * 100).GetSafeNormal(); }

	UFUNCTION(BlueprintCallable, Category = "Mouse")
	FORCEINLINE FVector GetPlayerMouseDirection() const { return mAxisPositions.MouseDirection; }

	UFUNCTION(BlueprintCallable, Category = "PlayerManagement")
	virtual void DropOut();

protected:
	virtual void SetupInputComponent() override;
	virtual ASpectatorPawn* SpawnSpectatorPawn() override;

private:
	bool mTestFOV = false;
	AxisPositions mAxisPositions;
};
