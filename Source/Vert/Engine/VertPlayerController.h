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
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable, Category = "PawnPossesion")
	FOnPossessedDelegate OnPossessed;

	UPROPERTY(BlueprintAssignable, Category = "PawnPossesion")
	FOnUnPossessedDelegate OnUnPossessed;

public:
	AVertPlayerController();

	virtual void DropIn();
	virtual bool CanRestartPlayer() override;
	virtual void Possess(APawn* aPawn) override;
	virtual void UnPossess() override;

	class UVertLocalPlayer* GetVertLocalPlayer();

	FORCEINLINE void ToggleFOV() { UE_LOG(LogVertPlayerController, Warning, TEXT("Toggling FOV for test")); mTestFOV = !mTestFOV; }
	FORCEINLINE bool IsTestingFOV() const { return mTestFOV; }

	UFUNCTION(BlueprintCallable, Category = "PlayerManagement")
	virtual void DropOut();

protected:
	virtual void SetupInputComponent() override;
	virtual ASpectatorPawn* SpawnSpectatorPawn() override;

private:
	bool mTestFOV = false;
};
