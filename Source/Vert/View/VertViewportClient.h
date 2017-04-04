// Copyright Inside Out Games Ltd. 2017

#pragma once

#include "Engine/GameViewportClient.h"
#include "VertViewportClient.generated.h"

/**
 * 
 */
UCLASS()
class VERT_API UVertViewportClient : public UGameViewportClient
{
	GENERATED_BODY()
	
public:
	virtual void Init(struct FWorldContext& WorldContext, UGameInstance* OwningGameInstance, bool bCreateNewAudioDevice = true) override;
	virtual bool InputKey(FViewport* pViewport, int32 ControllerId, FKey Key, EInputEvent EventType, float AmountDepressed = 1.f, bool bGamepad = false) override;

private:
	TWeakObjectPtr<UVertGameInstance> mVertGameInstance = nullptr;
};
