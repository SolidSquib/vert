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

#pragma once

#include "OBJPool.h"
#include "K2Node.h"
#include "CoreMinimal.h"
#include "Textures/SlateIcon.h"
#include "UObject/ObjectMacros.h"
#include "EdGraph/EdGraphNodeUtils.h"
#include "K2Node_SpawnActorFromPool.generated.h"

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class FBlueprintActionDatabaseRegistrar;
class UEdGraph;

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

UCLASS()
class OBJPOOLEDITOR_API UK2Node_SpawnActorFromPool : public UK2Node {
	GENERATED_UCLASS_BODY()
private:
	void MaybeUpdateCollisionPin(TArray<UEdGraphPin*> &OldPins);
protected:
	FText NodeTooltip;
	FNodeTextCache CachedNodeTitle;
	//
	void OnClassPinChanged();
public:
	virtual void AllocateDefaultPins() override;
	virtual void PostPlacedNewNode() override;
	virtual FText GetTooltipText() const override;
	virtual FLinearColor GetNodeTitleColor() const override;
	virtual void PinDefaultValueChanged(UEdGraphPin* Pin) override;
	virtual void PinConnectionListChanged(UEdGraphPin* Pin) override;
	virtual FSlateIcon GetIconAndTint(FLinearColor &OutColor) const override;
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual bool IsCompatibleWithGraph(const UEdGraph* TargetGraph) const override;
	virtual void GetPinHoverText(const UEdGraphPin &Pin, FString &HoverTextOut) const override;
	virtual bool HasExternalDependencies(TArray<class UStruct*>* OptionalOutput) const override;
	virtual void ExpandNode(class FKismetCompilerContext &CompilerContext, UEdGraph* SourceGraph) override;
	//
	virtual FText GetMenuCategory() const override;
	virtual bool IsNodeSafeToIgnore() const override {return true;}
	virtual void ReallocatePinsDuringReconstruction(TArray<UEdGraphPin*> &OldPins) override;
	virtual void GetMenuActions(FBlueprintActionDatabaseRegistrar &ActionRegistrar) const override;
	virtual void GetNodeAttributes( TArray<TKeyValuePair<FString, FString>> &OutNodeAttributes ) const override;
	virtual class FNodeHandlingFunctor* CreateNodeHandler(class FKismetCompilerContext &CompilerContext) const override;
	//
	bool IsSpawnVarPin(UEdGraphPin* Pin);
	void CreatePinsForClass(UClass* InClass, TArray<UEdGraphPin*> &OutClassPins);
	//
	UEdGraphPin* GetThenPin() const;
	UEdGraphPin* GetResultPin() const;
	UEdGraphPin* GetOwnerPin() const;
	UEdGraphPin* GetSuccessPin() const;
	UEdGraphPin* GetReconstructPin() const;
	UEdGraphPin* GetWorldContextPin() const;
	UEdGraphPin* GetSpawnOptionsPin() const;
	UEdGraphPin* GetSpawnTransformPin() const;
	UEdGraphPin* GetCollisionHandlingOverridePin() const;
	UEdGraphPin* GetPoolPin(const TArray<UEdGraphPin*>* InPinsToSearch=NULL) const;
	UClass* GetClassToSpawn(const TArray<UEdGraphPin*>* InPinsToSearch=NULL) const;
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////