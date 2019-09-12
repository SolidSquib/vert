// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "VertGameMode_Menu.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FVertGameModeMenuPlayerLoggedIn, AController*, player);

UCLASS()
class AVertGameMode_Menu : public AGameMode
{
	GENERATED_UCLASS_BODY()

public:
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FVertGameModeMenuPlayerLoggedIn OnPlayerLoggedIn;

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FVertGameModeMenuPlayerLoggedIn OnPlayerLoggedOut;

	UPROPERTY(EditDefaultsOnly, Category = "Fade")
	float FadeInTime = 3.f;

	UPROPERTY(EditDefaultsOnly, Category = "Widgets")
	TSubclassOf<class UUserWidget> MainMenuWidgetClass = nullptr;

	UPROPERTY()
	UUserWidget* MainMenuWidget = nullptr;

public:
	// Begin AGameModeBase interface
	/** skip it, menu doesn't require player start or pawn */
	virtual void RestartPlayer(class AController* NewPlayer) override;

	/** Returns game session class to use */
	virtual TSubclassOf<AGameSession> GetGameSessionClass() const override;
	// End AGameModeBase interface

	virtual void StartPlay() override;
	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual void Logout(AController* Exiting) override;
};