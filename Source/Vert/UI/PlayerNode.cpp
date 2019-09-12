// Copyright Inside Out Games Limited 2017

#include "PlayerNode.h"
#include "Engine/VertPlayerController.h"

AVertPlayerController* UPlayerNode::GetPlayerControllerOwner() const 
{
	if (mPlayerController.IsValid() && !mPlayerController->IsPendingKill())
		return mPlayerController.Get();

	return nullptr;
}

void UPlayerNode::SetPlayerControllerOwner(class AVertPlayerController* newOwner)
{
	if (newOwner && !newOwner->IsPendingKill())
		mPlayerController = newOwner;
}