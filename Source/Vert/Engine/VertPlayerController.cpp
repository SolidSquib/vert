// Copyright Inside Out Games Ltd. 2017

#include "Vert.h"
#include "VertPlayerController.h"

DEFINE_LOG_CATEGORY(LogVertPlayerController);

AVertPlayerController::AVertPlayerController()
{
	PlayerCameraManagerClass = AVertCameraManager::StaticClass();
}

void AVertPlayerController::DropIn()
{
	// #MI_TODO: Needs to be slightly more fancy eventually but it does the job for now.
	SetSpectatorPawn(SpawnSpectatorPawn());
}

void AVertPlayerController::DropOut()
{
	UnPossess();		
	// #MI_TODO: possess by AIController or remove character...
}

bool AVertPlayerController::UsingGamepad() const
{
	return mControllerType != EControllerType::Keyboard_Mouse;
}

bool AVertPlayerController::InputKey(FKey Key, EInputEvent EventType, float AmountDepressed, bool bGamepad)
{
	// #MI_TODO: input device type for UI
	if (bGamepad)
	{
		bShowMouseCursor = false;
		bEnableClickEvents = false;
		bEnableMouseOverEvents = false;
		mControllerType = EControllerType::Gamepad_Xbox;
	}
	else
	{
		bShowMouseCursor = true;
		bEnableClickEvents = true;
		bEnableMouseOverEvents = true;
		mControllerType = EControllerType::Keyboard_Mouse;
	}

	return Super::InputKey(Key, EventType, AmountDepressed, bGamepad);
}

UVertLocalPlayer* AVertPlayerController::GetVertLocalPlayer()
{
	if (ULocalPlayer* localPlayer = GetLocalPlayer())
	{
		if (UVertLocalPlayer* vertLocalPlayer = Cast<UVertLocalPlayer>(localPlayer))
		{
			return vertLocalPlayer;
		}
	}

	return nullptr;
}

void AVertPlayerController::Possess(APawn* aPawn)
{
	Super::Possess(aPawn);

	if (aPawn)
	{
		UE_LOG(LogVertPlayerController, Warning, TEXT("Broadcasting controller possessed"));
		OnPossessed.Broadcast(aPawn);
	}
}

void AVertPlayerController::UnPossess()
{
	if (GetPawn())
	{
		UE_LOG(LogVertPlayerController, Warning, TEXT("Broadcasting controller unpossessed"));
		OnUnPossessed.Broadcast(GetPawn());
	}

	Super::UnPossess();
}

void AVertPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (InputComponent)
	{
		InputComponent->BindAction("DropIn", IE_Pressed, this, &AVertPlayerController::DropIn);
		InputComponent->BindAction("ToggleFOV", IE_Pressed, this, &AVertPlayerController::ToggleFOV);
	}	
}

ASpectatorPawn* AVertPlayerController::SpawnSpectatorPawn()
{
	ASpectatorPawn* pawn = nullptr;
	return Super::SpawnSpectatorPawn();

	UE_LOG(LogVertPlayerController, Warning, TEXT("Player inactive, no spectator spawned"));

	return pawn;
}

void AVertPlayerController::SetGodMode(bool enable)
{
	mGodMode = enable;
}

void AVertPlayerController::SetInfiniteWeaponUsage(bool enable)
{
	mInfiniteWeaponUsage = enable;
}

void AVertPlayerController::SetInfiniteClip(bool enable)
{
	mInfiniteClip = enable;
}

bool AVertPlayerController::HasInfiniteWeaponUsage() const
{
	return mInfiniteWeaponUsage;
}

bool AVertPlayerController::HasInfiniteClip() const
{
	return mInfiniteClip;
}

bool AVertPlayerController::HasGodMode() const 
{
	return mGodMode;
}

void AVertPlayerController::DisplayClientMessage(FString s)
{
	ClientMessage(s);
}

void AVertPlayerController::DisplayInt(FString label, int32 theInt)
{
	label += " ";
	label += FString::FromInt(theInt);
	ClientMessage(label);
}

void AVertPlayerController::DisplayFloat(FString label, float theFloat)
{
	label += " ";
	label += FString::SanitizeFloat(theFloat);
	ClientMessage(label);
}

void AVertPlayerController::DisplayVector(FString label, FVector theVector)
{
	label += " ";
	label += FString::SanitizeFloat(theVector.X) + ", " + FString::SanitizeFloat(theVector.Y) + ", " + FString::SanitizeFloat(theVector.Z);
	ClientMessage(label);
}

void AVertPlayerController::DisplayVector2D(FString label, FVector2D theVector)
{
	label += " ";
	label += FString::SanitizeFloat(theVector.X) + ", " + FString::SanitizeFloat(theVector.Y);
	ClientMessage(label);
}

void AVertPlayerController::OverridePawnFollow(int32 pawnIndex)
{
	if (AVertGameMode* mode = Cast<AVertGameMode>(GetWorld()->GetAuthGameMode()))
	{
		AVertPlayerCameraActor* cam = mode->GetActivePlayerCamera();
		if (cam)
		{
			cam->OverridePawnFollow(pawnIndex);
		}
	}
}

void AVertPlayerController::OverrideCameraZoom(int32 cameraZoomAmount)
{
	if (AVertGameMode* mode = Cast<AVertGameMode>(GetWorld()->GetAuthGameMode()))
	{
		AVertPlayerCameraActor* cam = mode->GetActivePlayerCamera();
		if (cam)
		{
			cam->OverrideCameraZoom(cameraZoomAmount);
		}
	}
}

void AVertPlayerController::EnableDebugInfo(bool enable)
{
	for (UActorComponent* component : GetComponents())
	{
		if (IDebuggable* debuggable = Cast<IDebuggable>(component))
		{
			debuggable->EnableDebug(enable);
		}
	}
}