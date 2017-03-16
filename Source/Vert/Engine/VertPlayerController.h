// Copyright Inside Out Games Ltd. 2017

#pragma once

#include "GameFramework/PlayerController.h"
#include "VertPlayerController.generated.h"

/**
 * 
 */
UCLASS()
class VERT_API AVertPlayerController : public APlayerController
{
	GENERATED_BODY()
	
public:
	virtual void DropIn();
	virtual bool CanRestartPlayer() override;

	class UVertLocalPlayer* GetVertLocalPlayer();

	UFUNCTION(BlueprintCallable, Category = "PlayerManagement")
	virtual void DropOut();

protected:
	virtual void SetupInputComponent() override;
};
