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

#include "OBJPoolEditor.h"
#include "OBJPoolEditorPrivatePCH.h"
#include "Kismet2/KismetEditorUtilities.h"

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define LOCTEXT_NAMESPACE "Synaptech"

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

UOBJPoolFactory::UOBJPoolFactory(const class FObjectInitializer &OBJ) : Super(OBJ) {
	SupportedClass = UObjectPool::StaticClass();
	bEditAfterNew = true;
	bCreateNew = true;
}

UObject* UOBJPoolFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) {
	check(Class->IsChildOf(UObjectPool::StaticClass()));
	return FKismetEditorUtilities::CreateBlueprint(Class,InParent,Name,BPTYPE_Normal,UBlueprint::StaticClass(),UBlueprintGeneratedClass::StaticClass(),TEXT("AssetTypeActions"));
}

FText FATA_OBJP::GetAssetDescription(const FAssetData &AssetData) const {
	return FText::FromString(FString(TEXT("Blueprintable Object-Pool Component.")));
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

UPWPoolFactory::UPWPoolFactory(const class FObjectInitializer &OBJ) : Super(OBJ) {
	SupportedClass = UPawnPool::StaticClass();
	bEditAfterNew = true;
	bCreateNew = true;
}

UObject* UPWPoolFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) {
	check(Class->IsChildOf(UPawnPool::StaticClass()));
	return FKismetEditorUtilities::CreateBlueprint(Class,InParent,Name,BPTYPE_Normal,UBlueprint::StaticClass(),UBlueprintGeneratedClass::StaticClass(),TEXT("AssetTypeActions"));
}

FText FATA_PWP::GetAssetDescription(const FAssetData &AssetData) const {
	return FText::FromString(FString(TEXT("Blueprintable Pawn Object-Pool Component.")));
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

UCHPoolFactory::UCHPoolFactory(const class FObjectInitializer &OBJ) : Super(OBJ) {
	SupportedClass = UCharacterPool::StaticClass();
	bEditAfterNew = true;
	bCreateNew = true;
}

UObject* UCHPoolFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) {
	check(Class->IsChildOf(UCharacterPool::StaticClass()));
	return FKismetEditorUtilities::CreateBlueprint(Class,InParent,Name,BPTYPE_Normal,UBlueprint::StaticClass(),UBlueprintGeneratedClass::StaticClass(),TEXT("AssetTypeActions"));
}

FText FATA_CHP::GetAssetDescription(const FAssetData &AssetData) const {
	return FText::FromString(FString(TEXT("Blueprintable Character Object-Pool Component.")));
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#undef LOCTEXT_NAMESPACE

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
