// Copyright Inside Out Games Ltd. 2017

#include "Vert.h"
#include "VertPlayerController.h"

void AVertPlayerController::DropIn()
{
	// #MI_TODO: Needs to be slightly more fancy eventually but it does the job for now.

	if (UVertLocalPlayer* player = GetVertLocalPlayer())
	{
		if (!player->IsPlayerInGame())
		{
			player->PlayerJoinGame();
			SpawnSpectatorPawn();
			
			UE_LOG(VertCritical, Warning, TEXT("Spawned as spectator; waiting to join: %p"), GetSpectatorPawn());

			if (ASpectatorPawn* pawn = GetSpectatorPawn())
			{
				UE_LOG(VertCritical, Warning, TEXT("Spectator location is %f, %f, %f"), pawn->GetActorLocation().X, pawn->GetActorLocation().Y, pawn->GetActorLocation().Z);
			}
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

void AVertPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (InputComponent)
	{
		InputComponent->BindAction("DropIn", IE_Pressed, this, &AVertPlayerController::DropIn);
	}	
}
