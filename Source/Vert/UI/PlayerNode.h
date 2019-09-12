// Copyright Inside Out Games Limited 2017

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PlayerNode.generated.h"

/**
 * 
 */
UCLASS()
class VERT_API UPlayerNode : public UUserWidget
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintCallable)
	AVertPlayerController* GetPlayerControllerOwner() const;

	UFUNCTION(BlueprintCallable)
	void SetPlayerControllerOwner(class AVertPlayerController* newOwner);

private:
	TWeakObjectPtr<AVertPlayerController> mPlayerController = nullptr;
};
