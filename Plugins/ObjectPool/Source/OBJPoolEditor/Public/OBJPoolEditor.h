////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
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
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "OBJPool.h"
#include "Public/AssetTypeActions_Base.h"
#include "Editor/UnrealEd/Classes/Factories/Factory.h"
#include "OBJPoolEditor.generated.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define LOCTEXT_NAMESPACE "Synaptech"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

UCLASS()
class OBJPOOLEDITOR_API UOBJPoolFactory : public UFactory {
	GENERATED_UCLASS_BODY()
protected:
	virtual bool IsMacroFactory() const { return false; }
public:
	virtual UObject* FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
};

UCLASS()
class OBJPOOLEDITOR_API UPWPoolFactory : public UFactory {
	GENERATED_UCLASS_BODY()
protected:
	virtual bool IsMacroFactory() const { return false; }
public:
	virtual UObject* FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
};

UCLASS()
class OBJPOOLEDITOR_API UCHPoolFactory : public UFactory {
	GENERATED_UCLASS_BODY()
protected:
	virtual bool IsMacroFactory() const { return false; }
public:
	virtual UObject* FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static EAssetTypeCategories::Type SY_AssetCategory;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class FATA_OBJP : public FAssetTypeActions_Base {
public:
	virtual uint32 GetCategories() override { return SY_AssetCategory; }
	virtual FText GetName() const override { return NSLOCTEXT("AssetTypeActions","FATA_OBJP","Object Pool"); }
	virtual UClass* GetSupportedClass() const override { return UObjectPool::StaticClass(); }
	virtual FColor GetTypeColor() const override { return FColor(27,190,27); }
	virtual FText GetAssetDescription(const FAssetData &AssetData) const override;
};

class FATA_PWP : public FAssetTypeActions_Base {
public:
	virtual uint32 GetCategories() override { return SY_AssetCategory; }
	virtual FText GetName() const override { return NSLOCTEXT("AssetTypeActions","FATA_PWP","Pawn Pool"); }
	virtual UClass* GetSupportedClass() const override { return UPawnPool::StaticClass(); }
	virtual FColor GetTypeColor() const override { return FColor(190,27,27); }
	virtual FText GetAssetDescription(const FAssetData &AssetData) const override;
};

class FATA_CHP : public FAssetTypeActions_Base {
public:
	virtual uint32 GetCategories() override { return SY_AssetCategory; }
	virtual FText GetName() const override { return NSLOCTEXT("AssetTypeActions","FATA_CHP","Character Pool"); }
	virtual UClass* GetSupportedClass() const override { return UCharacterPool::StaticClass(); }
	virtual FColor GetTypeColor() const override { return FColor(27,27,190); }
	virtual FText GetAssetDescription(const FAssetData &AssetData) const override;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#undef LOCTEXT_NAMESPACE

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
