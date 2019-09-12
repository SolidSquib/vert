#include "VertPlayerController_Menu.h"
#include "AkGameplayStatics.h"


AVertPlayerController_Menu::AVertPlayerController_Menu(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	bAutoManageActiveCameraTarget = false;
	SetIgnoreLookInput(true);
	SetIgnoreMoveInput(true);
}

//************************************
// Method:    PostInitializeComponents
// FullName:  AVertPlayerController_Menu::PostInitializeComponents
// Access:    virtual public 
// Returns:   void
// Qualifier:
//************************************
void AVertPlayerController_Menu::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	// Initialise slate styles for menus if needed here.
}

//************************************
// Method:    SetupInputComponent
// FullName:  AVertPlayerController_Menu::SetupInputComponent
// Access:    virtual protected 
// Returns:   void
// Qualifier:
//************************************
void AVertPlayerController_Menu::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (InputComponent)
	{
		InputComponent->BindAction("MenuBack", IE_Pressed, this, &AVertPlayerController_Menu::ActionMenuBackPressed);
		InputComponent->BindAction("MenuAccept", IE_Pressed, this, &AVertPlayerController_Menu::ActionMenuAcceptPressed);
	}
}

//************************************
// Method:    ActionMenuAcceptPressed
// FullName:  AVertPlayerController_Menu::ActionMenuAcceptPressed
// Access:    protected 
// Returns:   void
// Qualifier:
//************************************
void AVertPlayerController_Menu::ActionMenuAcceptPressed()
{
	if (mCurrentMenuMode == EMenuMode::MM_LOBBY)
	{
		ToggleReadyState();
	}
}

//************************************
// Method:    ActionMenuBackPressed
// FullName:  AVertPlayerController_Menu::ActionMenuBackPressed
// Access:    protected 
// Returns:   void
// Qualifier:
//************************************
void AVertPlayerController_Menu::ActionMenuBackPressed()
{
	if (mCurrentMenuMode == EMenuMode::MM_LOBBY)
	{
		if (mIsReady)
		{
			mIsReady = false;
		}
		else if (!IsPrimaryPlayer())
		{
			GetGameInstance()->RemoveLocalPlayer(GetLocalPlayer());
		}
		else
		{
			OnBackToMenuRequested.Broadcast();
		}
	}
}

//************************************
// Method:    ToggleReadyState
// FullName:  AVertPlayerController_Menu::ToggleReadyState
// Access:    protected 
// Returns:   void
// Qualifier:
//************************************
void AVertPlayerController_Menu::ToggleReadyState()
{
	mIsReady = !mIsReady;
}

//************************************
// Method:    GetIsReady
// FullName:  AVertPlayerController_Menu::GetIsReady
// Access:    public 
// Returns:   bool
// Qualifier: const
//************************************
bool AVertPlayerController_Menu::GetIsReady() const
{
	return mIsReady;
}

//************************************
// Method:    SetMenuState
// FullName:  AVertPlayerController_Menu::SetMenuState
// Access:    public 
// Returns:   void
// Qualifier:
// Parameter: EMenuMode newState
//************************************
void AVertPlayerController_Menu::SetMenuState(EMenuMode newState)
{
	mCurrentMenuMode = newState;
}

#if PLATFORM_WINDOWS || PLATFORM_MAC
//************************************
// Method:    InputKey
// FullName:  AVertPlayerController::InputKey
// Access:    virtual public 
// Returns:   bool
// Qualifier:
// Parameter: FKey Key
// Parameter: EInputEvent EventType
// Parameter: float AmountDepressed
// Parameter: bool bGamepad
//************************************
bool AVertPlayerController_Menu::InputKey(FKey Key, EInputEvent EventType, float AmountDepressed, bool bGamepad)
{
	if (bGamepad)
	{
		bShowMouseCursor = false;
		bEnableClickEvents = false;
		bEnableMouseOverEvents = false;
		mUsingGamepad = true;
	}
	else
	{
		bShowMouseCursor = true;
		bEnableClickEvents = true;
		bEnableMouseOverEvents = true;
		mUsingGamepad = false;
	}

	return Super::InputKey(Key, EventType, AmountDepressed, bGamepad);
}
#endif

//************************************
// Method:    IsUsingGamepad
// FullName:  AVertPlayerController_Menu::IsUsingGamepad
// Access:    public 
// Returns:   bool
// Qualifier: const
//************************************
bool AVertPlayerController_Menu::IsUsingGamepad() const
{
	return mUsingGamepad;
}