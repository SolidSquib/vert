// Copyright Inside Out Games Ltd. 2017

#include "VertViewportClient.h"
#include "Vert.h"

DECLARE_LOG_CATEGORY_CLASS(LogVertViewportClient, Log, All);

//************************************
// Method:    Init
// FullName:  UVertViewportClient::Init
// Access:    virtual public 
// Returns:   void
// Qualifier:
// Parameter: FWorldContext & WorldContext
// Parameter: UGameInstance * OwningGameInstance
// Parameter: bool bCreateNewAudioDevice
//************************************
void UVertViewportClient::Init(FWorldContext& WorldContext, UGameInstance* OwningGameInstance, bool bCreateNewAudioDevice /*= true*/)
{
	Super::Init(WorldContext, OwningGameInstance, bCreateNewAudioDevice);

	mVertGameInstance = Cast<UVertGameInstance>(OwningGameInstance);
}

//************************************
// Method:    InputKey
// FullName:  UVertViewportClient::InputKey
// Access:    virtual public 
// Returns:   bool
// Qualifier:
// Parameter: FViewport * pViewport
// Parameter: int32 ControllerId
// Parameter: FKey Key
// Parameter: EInputEvent EventType
// Parameter: float AmountDepressed
// Parameter: bool bGamepad
//************************************
bool UVertViewportClient::InputKey(FViewport* pViewport, int32 ControllerId, FKey Key, EInputEvent EventType, float AmountDepressed /*= 1.f*/, bool bGamepad /*= false*/)
{
	const AVertGameState* gameState = GetWorld() ? GetWorld()->GetGameState<AVertGameState>() : nullptr;

	if (gameState && gameState->CanPlayersJoin())
	{
		// Check if a player is attempting to drop-in on a gamepad, not currently supporting keyboard for this.
		if (mVertGameInstance.IsValid() && mVertGameInstance->IsControllerIDAvailable(ControllerId) && Key == EKeys::Gamepad_Special_Right && EventType == EInputEvent::IE_Pressed)
		{
			FString outError;
			ULocalPlayer* newPlayer = mVertGameInstance->CreateLocalPlayer(ControllerId, outError, gameState->IsMatchInProgress());

			return true;
		}
	}

	return Super::InputKey(pViewport, ControllerId, Key, EventType, AmountDepressed, bGamepad);
}

//************************************
// Method:    PostRender
// FullName:  UVertViewportClient::PostRender
// Access:    virtual public 
// Returns:   void
// Qualifier:
// Parameter: UCanvas * Canvas
//************************************
void UVertViewportClient::PostRender(UCanvas* Canvas)
{
	Super::PostRender(Canvas);

	// Fade if requested
	if (mIsFading)
	{
		DrawScreenFade(Canvas);
	}
}

//************************************
// Method:    ClearFade
// FullName:  UVertViewportClient::ClearFade
// Access:    virtual public 
// Returns:   void
// Qualifier:
//************************************
void UVertViewportClient::ClearFade()
{
	mIsFading = false;
}

//************************************
// Method:    Fade
// FullName:  UVertViewportClient::Fade
// Access:    virtual public 
// Returns:   void
// Qualifier:
// Parameter: const float duration
// Parameter: const bool toBlack
//************************************
void UVertViewportClient::Fade(const float duration, const bool toBlack)
{
	const UWorld* world = GetWorld();
	if (world)
	{
		mIsFading = true;
		mToBlack = toBlack;
		mFadeDuration = duration;
		mFadeStartTime = world->GetTimeSeconds();
	}
}

//************************************
// Method:    DrawScreenFade
// FullName:  UVertViewportClient::DrawScreenFade
// Access:    public 
// Returns:   void
// Qualifier:
// Parameter: UCanvas * canvas
//************************************
void UVertViewportClient::DrawScreenFade(UCanvas* canvas)
{
	if (mIsFading)
	{
		const UWorld* world = GetWorld();
		if (world)
		{
			const float time = world->GetTimeSeconds();
			const float alpha = FMath::Clamp((time - mFadeStartTime) / mFadeDuration, 0.f, 1.f);

			// Make sure that we stay black in a fade to black
			if (alpha == 1.f && !mToBlack)
			{
				mIsFading = false;
			}
			else
			{
				FColor oldColour = canvas->DrawColor;
				FLinearColor fadeColour = FLinearColor::Black;
				fadeColour.A = mToBlack ? alpha : 1 - alpha;
				canvas->DrawColor = fadeColour.ToFColor(true);
				canvas->DrawTile(canvas->DefaultTexture, 0, 0, canvas->ClipX, canvas->ClipY, 0, 0, canvas->DefaultTexture->GetSizeX(), canvas->DefaultTexture->GetSizeY());
				canvas->DrawColor = oldColour;
			}
		}
	}
}