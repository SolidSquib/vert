// Copyright Inside Out Games Ltd. 2017

#include "Vert.h"
#include "VertPlayerController.h"

DEFINE_LOG_CATEGORY(LogVertPlayerController);

AVertPlayerController::AVertPlayerController()
{
	PlayerCameraManagerClass = AVertCameraManager::StaticClass();
	bAutoManageActiveCameraTarget = false;
}

void AVertPlayerController::DropIn()
{
	// #MI_TODO: Needs to be slightly more fancy eventually but it does the job for now.

	if (UVertLocalPlayer* player = GetVertLocalPlayer())
	{
		if (!player->IsPlayerInGame())
		{
			player->PlayerJoinGame();
			SetSpectatorPawn(SpawnSpectatorPawn());
		}
	}
}

void AVertPlayerController::DropOut()
{
	if (UVertLocalPlayer* player = GetVertLocalPlayer())
	{
		if (player->IsPlayerInGame())
		{
			player->PlayerLeaveGame();
			UnPossess();
			
			// #MI_TODO: possess by AIController or remove character...
		}
	}
}

bool AVertPlayerController::CanRestartPlayer()
{
	return Super::CanRestartPlayer() && GetVertLocalPlayer() && GetVertLocalPlayer()->IsPlayerInGame();
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
		InputComponent->BindAxis("LeftThumbstickMoveX", this, &AVertPlayerController::LeftThumbstickMoveX);
		InputComponent->BindAxis("LeftThumbstickMoveY", this, &AVertPlayerController::LeftThumbstickMoveY);
		InputComponent->BindAxis("RightThumbstickMoveX", this, &AVertPlayerController::RightThumbstickMoveX);
		InputComponent->BindAxis("RightThumbstickMoveY", this, &AVertPlayerController::RightThumbstickMoveY);
		InputComponent->BindAxis("MouseMove", this, &AVertPlayerController::MouseMove);
	}	
}

ASpectatorPawn* AVertPlayerController::SpawnSpectatorPawn()
{
	ASpectatorPawn* pawn = nullptr;

	if (UVertLocalPlayer* player = GetVertLocalPlayer())
	{
		if (player->IsPlayerInGame())
		{
			return Super::SpawnSpectatorPawn();
		}
	}

	UE_LOG(LogVertPlayerController, Warning, TEXT("Player inactive, no spectator spawned"));

	return pawn;
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

void AVertPlayerController::RightThumbstickMoveX(float value)
{
	mAxisPositions.RightX = value;
}

void AVertPlayerController::RightThumbstickMoveY(float value)
{
	mAxisPositions.RightY = value;
}

void AVertPlayerController::LeftThumbstickMoveX(float value)
{
	mAxisPositions.LeftX = value;
}

void AVertPlayerController::LeftThumbstickMoveY(float value)
{
	mAxisPositions.LeftY = value;
}

void AVertPlayerController::MouseMove(float value)
{
	const ULocalPlayer* localPlayer = Cast<ULocalPlayer>(Player);
	if (localPlayer && localPlayer->ViewportClient)
	{
		FVector2D mousePosition;
		if (localPlayer->ViewportClient->GetMousePosition(mousePosition))
		{
			FVector worldLocation, worldDirection;
			if (GetPawn())
			{
				FVector2D playerScreenLocation, mouseDirection;
				if (ProjectWorldLocationToScreen(GetPawn()->GetActorLocation(), playerScreenLocation))
				{
					mouseDirection = mousePosition - playerScreenLocation;
					mouseDirection *= 100;
					mouseDirection = mouseDirection.GetSafeNormal();
					mAxisPositions.MouseDirection = FVector(mouseDirection.X, 0.f, -mouseDirection.Y);
				}				
			}
		}		
	}
}