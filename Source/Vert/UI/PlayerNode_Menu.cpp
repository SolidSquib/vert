// Copyright Inside Out Games Limited 2017

#include "PlayerNode_Menu.h"
#include "Engine/VertPlayerController_Menu.h"

AVertPlayerController_Menu* UPlayerNode_Menu::GetPlayerControllerOwner() const
{
	if (mPlayerController.IsValid() && !mPlayerController->IsPendingKill())
		return mPlayerController.Get();

	return nullptr;
}

void UPlayerNode_Menu::SetPlayerControllerOwner(class AVertPlayerController_Menu* newOwner)
{
	if (newOwner && !newOwner->IsPendingKill())
		mPlayerController = newOwner;
}