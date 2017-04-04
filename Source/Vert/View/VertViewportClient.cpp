// Copyright Inside Out Games Ltd. 2017

#include "Vert.h"
#include "VertViewportClient.h"

DECLARE_LOG_CATEGORY_CLASS(LogVertViewportClient, Log, All);

void UVertViewportClient::Init(FWorldContext& WorldContext, UGameInstance* OwningGameInstance, bool bCreateNewAudioDevice /*= true*/)
{
	Super::Init(WorldContext, OwningGameInstance, bCreateNewAudioDevice);

	mVertGameInstance = Cast<UVertGameInstance>(OwningGameInstance);
}

bool UVertViewportClient::InputKey(FViewport* pViewport, int32 ControllerId, FKey Key, EInputEvent EventType, float AmountDepressed /*= 1.f*/, bool bGamepad /*= false*/)
{
	const AVertGameState* gameState = GetWorld() ? GetWorld()->GetGameState<AVertGameState>() : nullptr;

	if (gameState)
	{
		// Check if a player is attempting to drop-in on a gamepad, not currently supporting keyboard for this.
		if (mVertGameInstance.IsValid() && mVertGameInstance->IsControllerIDAvailable(ControllerId) && Key == EKeys::Gamepad_Special_Right && EventType == EInputEvent::IE_Pressed)
		{
			FString outError;
			mVertGameInstance->CreateLocalPlayer(ControllerId, outError, gameState->GetMatchState() == MatchState::InProgress);
			return true;
		}
	}

	return Super::InputKey(pViewport, ControllerId, Key, EventType, AmountDepressed, bGamepad);
}