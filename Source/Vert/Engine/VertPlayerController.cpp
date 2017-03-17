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
