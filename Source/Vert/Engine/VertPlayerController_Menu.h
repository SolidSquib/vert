#pragma once

#include "VertPlayerController_Menu.generated.h"

UENUM(BlueprintType)
enum class EMenuMode : uint8
{
	MM_MAIN UMETA(DisplayName = "Main Menu"),
	MM_LOBBY UMETA(DisplayName = "Lobby Screen"),
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnMenuActionDelegate);

UCLASS()
class AVertPlayerController_Menu : public APlayerController
{
	GENERATED_UCLASS_BODY()

public:
	UPROPERTY(BlueprintAssignable)
	FOnMenuActionDelegate OnBackToMenuRequested;

public:
	/** After game is initialized */
	virtual void PostInitializeComponents() override;
#if PLATFORM_WINDOWS || PLATFORM_MAC
	virtual bool InputKey(FKey Key, EInputEvent EventType, float AmountDepressed, bool bGamepad) override;
#endif

	UFUNCTION(BlueprintCallable, Category = "UserInput")
	bool IsUsingGamepad() const;

	UFUNCTION(BlueprintCallable, Category = "MenuStates")
	void SetMenuState(EMenuMode newState);

	UFUNCTION(BlueprintCallable, Category = "MenuStates")
	bool GetIsReady() const;

protected:
	void ActionMenuAcceptPressed();
	void ActionMenuBackPressed();
	void ToggleReadyState();

	virtual void SetupInputComponent() override;

private:
	bool mUsingGamepad = false;
	bool mIsReady = false;
	EMenuMode mCurrentMenuMode = EMenuMode::MM_MAIN;
};

