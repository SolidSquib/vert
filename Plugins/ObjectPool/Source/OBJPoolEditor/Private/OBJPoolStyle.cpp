/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////
///			Copyright 2017 (C) Bruno Xavier B. Leite
//////////////////////////////////////////////////////////////
/*
	BY EXECUTING, READING, EDITING, COPYING OR KEEPING FILES FROM THIS SOFTWARE SOURCE CODE,
	YOU AGREE TO THE FOLLOWING TERMS IN ADDITION TO EPIC GAMES MARKETPLACE EULA:
	- YOU HAVE READ AND AGREE TO EPIC GAMES TERMS: https://publish.unrealengine.com/faq
	- YOU AGREE DEVELOPER RESERVES ALL RIGHTS TO THE SOFTWARE PROVIDED, GRANTED BY LAW.
	- YOU AGREE YOU'LL NOT CREATE OR PUBLISH DERIVATIVE SOFTWARE TO THE MARKETPLACE.
	- YOU AGREE DEVELOPER WILL NOT PROVIDE SOFTWARE OUTSIDE MARKETPLACE ENVIRONMENT.
	- YOU AGREE DEVELOPER WILL NOT PROVIDE PAID OR EXCLUSIVE SUPPORT SERVICES.
	- YOU AGREE DEVELOPER PROVIDED SUPPORT CHANNELS, ARE UNDER HIS SOLE DISCRETION.
*/
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "OBJPoolStyle.h"
#include "SlateStyle.h"
#include "IPluginManager.h"
#include "OBJPoolEditorPrivatePCH.h"

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define PLUGIN_BRUSH(RelativePath,...) FSlateImageBrush(FOBJPoolStyle::InContent(RelativePath,TEXT(".png")),__VA_ARGS__)

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

TSharedPtr<FSlateStyleSet> FOBJPoolStyle::StyleSet = nullptr;
TSharedPtr<ISlateStyle> FOBJPoolStyle::Get() {return StyleSet;}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

FString FOBJPoolStyle::InContent(const FString &RelativePath, const TCHAR* Extension) {
	static FString Content = IPluginManager::Get().FindPlugin(TEXT("ObjectPool"))->GetContentDir();
	return (Content/RelativePath)+Extension;
}

FName FOBJPoolStyle::GetStyleSetName() {
	static FName StyleName(TEXT("OBJPoolStyle"));
	return StyleName;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void FOBJPoolStyle::Initialize() {
	if (StyleSet.IsValid()) {return;}
	//
	const FVector2D Icon16x16(16.0f,16.0f);
	const FVector2D Icon128x128(128.0f,128.0f);
	//
	StyleSet = MakeShareable(new FSlateStyleSet(GetStyleSetName()));
	StyleSet->SetContentRoot(IPluginManager::Get().FindPlugin(TEXT("ObjectPool"))->GetContentDir());
	//
	StyleSet->Set("ClassIcon.ObjectPool", new PLUGIN_BRUSH(TEXT("Icons/Pool_16x"),Icon16x16));
	StyleSet->Set("ClassThumbnail.ObjectPool", new PLUGIN_BRUSH(TEXT("Icons/Pool_128x"),Icon128x128));
	StyleSet->Set("ClassIcon.PawnPool", new PLUGIN_BRUSH(TEXT("Icons/Pawn_16x"),Icon16x16));
	StyleSet->Set("ClassThumbnail.PawnPool", new PLUGIN_BRUSH(TEXT("Icons/Pawn_128x"),Icon128x128));
	StyleSet->Set("ClassIcon.CharacterPool", new PLUGIN_BRUSH(TEXT("Icons/Character_16x"),Icon16x16));
	StyleSet->Set("ClassThumbnail.CharacterPool", new PLUGIN_BRUSH(TEXT("Icons/Character_128x"),Icon128x128));
	StyleSet->Set("ClassIcon.PooledProjectile", new PLUGIN_BRUSH(TEXT("Icons/Projectile_16x"),Icon16x16));
	StyleSet->Set("ClassThumbnail.PooledProjectile", new PLUGIN_BRUSH(TEXT("Icons/Projectile_128x"),Icon128x128));
	//
	FSlateStyleRegistry::RegisterSlateStyle(*StyleSet.Get());
	//FClassIconFinder::RegisterIconSource(StyleSet.Get());
};

void FOBJPoolStyle::Shutdown() {
	if (StyleSet.IsValid()) {
		FSlateStyleRegistry::UnRegisterSlateStyle(*StyleSet.Get());
		ensure(StyleSet.IsUnique());
		StyleSet.Reset();
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#undef PLUGIN_BRUSH

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
