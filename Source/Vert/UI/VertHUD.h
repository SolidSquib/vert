// Copyright Inside Out Games Ltd. 2017

#pragma once

#include "GameFramework/HUD.h"
#include "VertHUD.generated.h"

/**
 * 
 */
UCLASS()
class VERT_API AVertHUD : public AHUD
{
	GENERATED_BODY()
	
public: 
	UPROPERTY()
	UFont* Font;

	UPROPERTY()
	UFont* BigFont;

public:
	AVertHUD(const FObjectInitializer& ObjectInitializer);

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void DrawHUD() override;

protected:
	virtual void BeginPlay() override;

private:
	void DrawPlayerName();
	void DrawHealth();
	void DrawPlayerNodes();

private:
	FFontRenderInfo mFont; // Font render info, tells the renderer how to show this font
	FText mPlayerNameText;
	FText mPlayerHealthText;
	float mScaleUI; // UI scaling factor for resolutions other than Full HD
	float nameY = 0.f; // HACK
};
