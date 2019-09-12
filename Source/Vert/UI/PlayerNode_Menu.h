// Copyright Inside Out Games Limited 2017

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PlayerNode_Menu.generated.h"

/**
 * 
 */
UCLASS()
class VERT_API UPlayerNode_Menu : public UUserWidget
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintCallable)
	AVertPlayerController_Menu* GetPlayerControllerOwner() const;

	UFUNCTION(BlueprintCallable)
	void SetPlayerControllerOwner(class AVertPlayerController_Menu* newOwner);

private:
	TWeakObjectPtr<AVertPlayerController_Menu> mPlayerController = nullptr;
};
