// Copyright Inside Out Games Ltd. 2017

#include "VertHUD.h"
#include "Vert.h"

DECLARE_LOG_CATEGORY_CLASS(LogVertHUD, Log, All);

AVertHUD::AVertHUD(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	#if !UE_SERVER
	{
		static ConstructorHelpers::FObjectFinder<UFont> fontObj(TEXT("/Game/UI/HUD/nasalization16"));
		static ConstructorHelpers::FObjectFinder<UFont> bigFontObj(TEXT("/Game/UI/HUD/nasalization30"));
		Font = fontObj.Object;
		BigFont = bigFontObj.Object;
	}
	#endif

	mFont.bEnableShadow = true;
}

void AVertHUD::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
}

void AVertHUD::DrawHUD()
{
	Super::DrawHUD();

	if (!Canvas)
		return;

	mScaleUI = Canvas->ClipY / 1080.0f;

	DrawPlayerNodes();
	//DrawPlayerName();
	//DrawHealth();
}

void AVertHUD::BeginPlay()
{
	Super::BeginPlay();
}

//************************************
// Method:    DrawPlayerNodes
// FullName:  AVertHUD::DrawPlayerNodes
// Access:    private 
// Returns:   void
// Qualifier:
//************************************
void AVertHUD::DrawPlayerNodes()
{
// 	if (AVertGameState* gameState = GetWorld() ? GetWorld()->GetGameState<AVertGameState>() : nullptr)
// 	{
// 		for (auto i = 0; i < MAX_PLAYERS; ++i)
// 		{
// 			const float drawPosX = (Canvas->ClipX / MAX_PLAYERS) * i;
// 
// 			FString nameString;
// 			if (gameState->PlayerArray.Num() <= i)
// 			{
// 				
// 			}
// 			else
// 			{
// 				AController* controller = gameState->PlayerArray[i]->GetInstigatorController();
// 				AVertPlayerController* 
// 			}
// 		}
// 	}
}

void AVertHUD::DrawPlayerName()
{
	if (AVertGameState* gameState = GetWorld() ? GetWorld()->GetGameState<AVertGameState>() : nullptr)
	{
		for (auto i = 0; i < MAX_PLAYERS; ++i)
		{
			const float drawPosX = (Canvas->ClipX / MAX_PLAYERS) * i;

			FString nameString;
			if (gameState->PlayerArray.Num() <= i)
				nameString = "Press start to join";
			else
				nameString = gameState->PlayerArray[i]->PlayerName; //FString::Printf(TEXT("Player %i"), i + 1);

			FCanvasTextItem textItem(FVector2D::ZeroVector, FText::GetEmpty(), Font, FLinearColor::White);
			float sizeX, sizeY;
			float textScale = 1.0f;

			textItem.Font = Font;
			Canvas->StrLen(Font, nameString, sizeX, sizeY);
			Canvas->SetDrawColor(FColor::White);
			textItem.SetColor(FLinearColor::Blue);
			textItem.Text = FText::FromString(nameString);
			textItem.Scale = FVector2D(textScale*mScaleUI, textScale* mScaleUI);
			Canvas->DrawItem(textItem, Canvas->OrgX + drawPosX, Canvas->ClipY - sizeY * mScaleUI);

			nameY = sizeY;
		}
	}
}

void AVertHUD::DrawHealth()
{
	if (AVertGameState* gameState = GetWorld() ? GetWorld()->GetGameState<AVertGameState>() : nullptr)
	{
		for (auto i = 0; i < gameState->PlayerArray.Num(); ++i)
		{
			const float drawPosX = (Canvas->ClipX / MAX_PLAYERS) * i;

			FCanvasTextItem textItem(FVector2D::ZeroVector, FText::GetEmpty(), BigFont, FLinearColor::White);
			float sizeX, sizeY, prevX, prevY;
			float textScale = 1.0f;

			FString text;
			if (AVertPlayerState* vertPlayerState = Cast<AVertPlayerState>(gameState->PlayerArray[i]))
				text = FString::Printf(TEXT("%i%s"), vertPlayerState->GetShownDamageTaken(), "%");
			else
				text = "ERROR";

			textItem.Font = BigFont;
			Canvas->StrLen(BigFont, text, sizeX, sizeY);
			Canvas->StrLen(Font, text, prevX, prevY);
			Canvas->SetDrawColor(FColor::White);
			textItem.SetColor(FLinearColor::Blue);
			textItem.Text = FText::FromString(text);
			textItem.Scale = FVector2D(textScale*mScaleUI, textScale* mScaleUI);
			Canvas->DrawItem(textItem, Canvas->OrgX + drawPosX, Canvas->ClipY - ((sizeY + prevY + nameY) * mScaleUI));
		}
	}
}