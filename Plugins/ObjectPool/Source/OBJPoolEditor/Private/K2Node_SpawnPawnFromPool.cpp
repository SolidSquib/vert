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

#include "K2Node_SpawnPawnFromPool.h"
#include "BlueprintActionDatabaseRegistrar.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Actor.h"
#include "BlueprintNodeSpawner.h"
#include "K2Node_CallFunction.h"
#include "K2Node_EnumLiteral.h"
#include "UObject/UnrealType.h"
#include "KismetCompilerMisc.h"
#include "EdGraphSchema_K2.h"
#include "Engine/EngineTypes.h"
#include "EditorCategoryUtils.h"
#include "EdGraph/EdGraph.h"
#include "KismetCompiler.h"
#include "K2Node_Select.h"

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

struct FK2Node_SpawnPawnFromPoolHelper {
	static FString PoolPinName;
	static FString ActorPinName;
	static FString OwnerPinName;
	static FString SuccessPinName;
	static FString ReconstructPinName;
	static FString WorldContextPinName;
	static FString SpawnOptionsPinName;
	static FString NoCollisionFailPinName;
	static FString SpawnTransformPinName;
	static FString CollisionHandlingOverridePinName;
};

FString FK2Node_SpawnPawnFromPoolHelper::ActorPinName(TEXT("Pawn"));
FString FK2Node_SpawnPawnFromPoolHelper::OwnerPinName(TEXT("Owner"));
FString FK2Node_SpawnPawnFromPoolHelper::PoolPinName(TEXT("ObjectPool"));
FString FK2Node_SpawnPawnFromPoolHelper::ReconstructPinName(TEXT("Reconstruct"));
FString FK2Node_SpawnPawnFromPoolHelper::SuccessPinName(TEXT("SpawnSuccessful"));
FString FK2Node_SpawnPawnFromPoolHelper::SpawnOptionsPinName(TEXT("SpawnOptions"));
FString FK2Node_SpawnPawnFromPoolHelper::SpawnTransformPinName(TEXT("SpawnTransform"));
FString FK2Node_SpawnPawnFromPoolHelper::WorldContextPinName(TEXT("WorldContextObject"));
FString FK2Node_SpawnPawnFromPoolHelper::NoCollisionFailPinName(TEXT("SpawnEvenIfColliding"));	
FString FK2Node_SpawnPawnFromPoolHelper::CollisionHandlingOverridePinName(TEXT("CollisionHandlingOverride"));

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define LOCTEXT_NAMESPACE "Synaptech"

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

UK2Node_SpawnPawnFromPool::UK2Node_SpawnPawnFromPool(const FObjectInitializer &ObjectInitializer) : Super(ObjectInitializer) {
	NodeTooltip = LOCTEXT("NodeTooltip", "Attempts to Spawn from Pool an Inactive Pawn. Creates then Spawns a fully new Pawn if there's none available in the Pool and 'Instantiate On Demand' is checked on Project Settings.");
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

FText UK2Node_SpawnPawnFromPool::GetNodeTitle(ENodeTitleType::Type TitleType) const {
	FText NodeTitle = NSLOCTEXT("K2Node","SpawnPawnFromPool_BaseTitle","Spawn Pawn From Object-Pool ...");
	//
	if (TitleType != ENodeTitleType::MenuTitle) {
		UEdGraphPin* PoolPin = GetPoolPin();
		if (PoolPin && PoolPin->LinkedTo.Num()>0) {
			auto LinkedPin = PoolPin->LinkedTo[0];
			UClass* Class = (LinkedPin) ? Cast<UClass>(LinkedPin->PinType.PinSubCategoryObject.Get()) : nullptr;
			UPawnPool* Pool = Cast<UPawnPool>(Class->ClassDefaultObject);
			if (Pool && Pool->TemplateClass->IsValidLowLevelFast() && CachedNodeTitle.IsOutOfDate(this)) {
				FText Name = Pool->TemplateClass->GetDisplayNameText();
				FFormatNamedArguments Args; Args.Add(TEXT("Name"),Name);
				CachedNodeTitle.SetCachedText(FText::Format(NSLOCTEXT("K2Node","SpawnPawnFromPool_Title","Spawn Pawn From Pool :: {Name}"),Args),this);
		} NodeTitle = CachedNodeTitle;}
	} return NodeTitle;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void UK2Node_SpawnPawnFromPool::AllocateDefaultPins() {
	const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();
	//
	/// Execution Pins.
	UEdGraphPin* SpawnPin = CreatePin(EGPD_Input,K2Schema->PC_Exec,TEXT(""),NULL,false,false,K2Schema->PN_Execute);
	UEdGraphPin* FinishPin = CreatePin(EGPD_Output,K2Schema->PC_Exec,TEXT(""),NULL,false,false,K2Schema->PN_Then);
	//
	SpawnPin->PinFriendlyName =LOCTEXT("Spawn","Spawn");
	FinishPin->PinFriendlyName =LOCTEXT("Finished","Finished Spawning");
	//
	//
	/// World Context Pin.
	if (GetBlueprint()->ParentClass->HasMetaDataHierarchical(FBlueprintMetadata::MD_ShowWorldContextPin)) {
		CreatePin(EGPD_Input,K2Schema->PC_Object,TEXT(""),UObject::StaticClass(),false,false,FK2Node_SpawnPawnFromPoolHelper::WorldContextPinName);
	}
	//
	/// Object-Pool Pin.
	UEdGraphPin* PoolPin = CreatePin(EGPD_Input,K2Schema->PC_Object,TEXT(""),UPawnPool::StaticClass(),false,false,FK2Node_SpawnPawnFromPoolHelper::PoolPinName);
	//
	/// Owner Pin.
	UEdGraphPin* OwnerPin = CreatePin(EGPD_Input,K2Schema->PC_Object,TEXT(""),AActor::StaticClass(),false,false,FK2Node_SpawnPawnFromPoolHelper::OwnerPinName);
	OwnerPin->bAdvancedView = true;
	//
	/// Collision Handling Pin.
	UEnum* const MethodEnum = FindObjectChecked<UEnum>(ANY_PACKAGE,TEXT("ESpawnActorCollisionHandlingMethod"),true);
	UEdGraphPin* const CollisionHandlingOverridePin = CreatePin(EGPD_Input,K2Schema->PC_Byte,TEXT(""),MethodEnum,false,false,FK2Node_SpawnPawnFromPoolHelper::CollisionHandlingOverridePinName);
	CollisionHandlingOverridePin->DefaultValue = MethodEnum->GetNameStringByIndex(static_cast<int>(ESpawnActorCollisionHandlingMethod::Undefined));
	//CollisionHandlingOverridePin->DefaultValue = MethodEnum->GetEnumName(static_cast<int>(ESpawnActorCollisionHandlingMethod::Undefined));
	//
	/// Reconstruct Pin.
	UEdGraphPin* ReconstructPin = CreatePin(EGPD_Input,K2Schema->PC_Boolean,TEXT(""),nullptr,false,false,FK2Node_SpawnPawnFromPoolHelper::ReconstructPinName);
	//
	/// Options Pin.
	UEdGraphPin* OptionsPin = CreatePin(EGPD_Input,K2Schema->PC_Struct,TEXT(""),FPoolSpawnOptions::StaticStruct(),false,false,FK2Node_SpawnPawnFromPoolHelper::SpawnOptionsPinName);
	//
	/// Transform Pin.
	UScriptStruct* TransformStruct = TBaseStructure<FTransform>::Get();
	UEdGraphPin* TransformPin = CreatePin(EGPD_Input,K2Schema->PC_Struct,TEXT(""),TransformStruct,false,false,FK2Node_SpawnPawnFromPoolHelper::SpawnTransformPinName);
	//
	/// Success Pin.
	UEdGraphPin* SuccessPin = CreatePin(EGPD_Output,K2Schema->PC_Boolean,TEXT(""),nullptr,false,false,FK2Node_SpawnPawnFromPoolHelper::SuccessPinName);
	//
	/// Result Pin.
	UEdGraphPin* ResultPin = CreatePin(EGPD_Output,K2Schema->PC_Object,TEXT(""),APooledPawn::StaticClass(),false,false,K2Schema->PN_ReturnValue);
	//
	if (ENodeAdvancedPins::NoPins == AdvancedPinDisplay) {
		AdvancedPinDisplay = ENodeAdvancedPins::Hidden;
	} Super::AllocateDefaultPins();
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

UClass* UK2Node_SpawnPawnFromPool::GetClassToSpawn(const TArray<UEdGraphPin*>* InPinsToSearch) const {
	const TArray<UEdGraphPin*>* PinsToSearch = InPinsToSearch ? InPinsToSearch : &Pins;
	//
	UEdGraphPin* PoolPin = GetPoolPin(PinsToSearch);
	UClass* SpawnClass = nullptr;
	//
	if (PoolPin && (PoolPin->LinkedTo.Num()>0)) {
		auto LinkedPin = PoolPin->LinkedTo[0];
		UClass* PoolClass = (LinkedPin) ? Cast<UClass>(LinkedPin->PinType.PinSubCategoryObject.Get()) : nullptr;
		UPawnPool* PoolComponent = Cast<UPawnPool>(PoolClass->ClassDefaultObject);
		if (PoolComponent && PoolComponent->TemplateClass->IsValidLowLevelFast()) {
			SpawnClass = PoolComponent->TemplateClass;
		} else {
			UE_LOG(LogTemp,Error,TEXT("{Pool}:: %s"),TEXT("Trying to Parse Object-Pool Component, but:: Invalid Template Class!"));
	}} return SpawnClass;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void UK2Node_SpawnPawnFromPool::CreatePinsForClass(UClass* InClass, TArray<UEdGraphPin*> &OutClassPins) {
	check(InClass != NULL);
	//
	const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();
	const UObject* const ClassDefaultObject = InClass->GetDefaultObject(false);
	//
	for (TFieldIterator<UProperty> PIT(InClass, EFieldIteratorFlags::IncludeSuper); PIT; ++PIT) {
		UProperty* Property = *PIT;
		UClass* PropertyClass = CastChecked<UClass>(Property->GetOuter());
		//
		const bool IsDelegate = Property->IsA(UMulticastDelegateProperty::StaticClass());
		const bool IsExposedToSpawn = UEdGraphSchema_K2::IsPropertyExposedOnSpawn(Property);
		const bool IsSettableExternally = !Property->HasAnyPropertyFlags(CPF_DisableEditOnInstance);
		//
		if (IsExposedToSpawn && !Property->HasAnyPropertyFlags(CPF_Parm) && IsSettableExternally && Property->HasAllPropertyFlags(CPF_BlueprintVisible) && !IsDelegate && (FindPin(Property->GetName())==NULL)) {
			UEdGraphPin* Pin = CreatePin(EGPD_Input, TEXT(""), TEXT(""), NULL, false, false, Property->GetName());
			const bool bPinGood = (Pin != NULL) && K2Schema->ConvertPropertyToPinType(Property,Pin->PinType);
			OutClassPins.Add(Pin);
			//
			if (ClassDefaultObject && Pin != NULL && K2Schema->PinDefaultValueIsEditable(*Pin)) {
				FString DefaultValueAsString;
				const bool bDefaultValueSet = FBlueprintEditorUtils::PropertyValueToString(Property, reinterpret_cast<const uint8*>(ClassDefaultObject), DefaultValueAsString);
				check( bDefaultValueSet );
				K2Schema->TrySetDefaultValue(*Pin, DefaultValueAsString);
			} if (Pin != nullptr) {K2Schema->ConstructBasicPinTooltip(*Pin, Property->GetToolTipText(), Pin->PinToolTip);}
	}}
	//
	UEdGraphPin* ResultPin = GetResultPin();
	ResultPin->PinType.PinSubCategoryObject = InClass;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void UK2Node_SpawnPawnFromPool::ExpandNode(class FKismetCompilerContext &CompilerContext, UEdGraph* SourceGraph) {
	Super::ExpandNode(CompilerContext,SourceGraph);
	//
	static FName BeginSpawningBlueprintFuncName = GET_FUNCTION_NAME_CHECKED(UPawnPool,BeginDeferredSpawnFromPool);
	static FName FinishSpawningFuncName = GET_FUNCTION_NAME_CHECKED(UPawnPool,FinishDeferredSpawnFromPool);
	//
	UK2Node_SpawnPawnFromPool* SpawnNode = this;
	UEdGraphPin* SpawnPoolPin = SpawnNode->GetPoolPin();
	UEdGraphPin* SpawnNodeExec = SpawnNode->GetExecPin();
	UEdGraphPin* SpawnNodeThen = SpawnNode->GetThenPin();
	UEdGraphPin* SpawnNodeResult = SpawnNode->GetResultPin();
	UEdGraphPin* SpawnSuccessPin = SpawnNode->GetSuccessPin();
	UEdGraphPin* SpawnNodeOwnerPin = SpawnNode->GetOwnerPin();
	UEdGraphPin* SpawnNodeOptions = SpawnNode->GetSpawnOptionsPin();
	UEdGraphPin* SpawnWorldContextPin = SpawnNode->GetWorldContextPin();
	UEdGraphPin* SpawnNodeTransform = SpawnNode->GetSpawnTransformPin();
	UEdGraphPin* SpawnNodeReconstructPin = SpawnNode->GetReconstructPin();
	UEdGraphPin* SpawnNodeCollisionHandlingOverride = GetCollisionHandlingOverridePin();
	//
	//
	UClass* ClassToSpawn = GetClassToSpawn();
	//
	if (!SpawnPoolPin||(SpawnPoolPin->LinkedTo.Num()==0)) {
		CompilerContext.MessageLog.Error(*LOCTEXT("SpawnPawnFromPool_Pool_Error","Spawn Node: @@ must have an @@ specified!").ToString(),SpawnNode,SpawnPoolPin);
		SpawnNode->BreakAllNodeLinks();
	return;}
	//
	//
	UK2Node_CallFunction* CallBeginSpawnNode = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(SpawnNode,SourceGraph);
	CallBeginSpawnNode->FunctionReference.SetExternalMember(BeginSpawningBlueprintFuncName,UPawnPool::StaticClass());
	CallBeginSpawnNode->AllocateDefaultPins();
	//
	UEdGraphPin* CallBeginExec = CallBeginSpawnNode->GetExecPin();
	UEdGraphPin* CallBeginResult = CallBeginSpawnNode->GetReturnValuePin();
	UEdGraphPin* CallBeginOwnerPin = CallBeginSpawnNode->FindPinChecked(FK2Node_SpawnPawnFromPoolHelper::OwnerPinName);
	UEdGraphPin* CallBeginActorPoolPin = CallBeginSpawnNode->FindPinChecked(FK2Node_SpawnPawnFromPoolHelper::PoolPinName);
	UEdGraphPin* CallBeginSuccessPin = CallBeginSpawnNode->FindPinChecked(FK2Node_SpawnPawnFromPoolHelper::SuccessPinName);
	UEdGraphPin* CallBeginOptions = CallBeginSpawnNode->FindPinChecked(FK2Node_SpawnPawnFromPoolHelper::SpawnOptionsPinName);
	UEdGraphPin* CallBeginTransform = CallBeginSpawnNode->FindPinChecked(FK2Node_SpawnPawnFromPoolHelper::SpawnTransformPinName);
	UEdGraphPin* CallBeginReconstructPin = CallBeginSpawnNode->FindPinChecked(FK2Node_SpawnPawnFromPoolHelper::ReconstructPinName);
	UEdGraphPin* CallBeginWorldContextPin = CallBeginSpawnNode->FindPinChecked(FK2Node_SpawnPawnFromPoolHelper::WorldContextPinName);
	UEdGraphPin* CallBeginCollisionHandlingOverride = CallBeginSpawnNode->FindPinChecked(FK2Node_SpawnPawnFromPoolHelper::CollisionHandlingOverridePinName);
	//
	CompilerContext.MovePinLinksToIntermediate(*SpawnNodeExec,*CallBeginExec);
	CompilerContext.MovePinLinksToIntermediate(*SpawnPoolPin,*CallBeginActorPoolPin);
	CompilerContext.MovePinLinksToIntermediate(*SpawnNodeOptions,*CallBeginOptions);
	CompilerContext.MovePinLinksToIntermediate(*SpawnSuccessPin,*CallBeginSuccessPin);
	CompilerContext.MovePinLinksToIntermediate(*SpawnNodeTransform,*CallBeginTransform);
	CompilerContext.MovePinLinksToIntermediate(*SpawnNodeReconstructPin,*CallBeginReconstructPin);
	CompilerContext.MovePinLinksToIntermediate(*SpawnNodeCollisionHandlingOverride,*CallBeginCollisionHandlingOverride);
	//
	if (SpawnWorldContextPin) {CompilerContext.MovePinLinksToIntermediate(*SpawnWorldContextPin,*CallBeginWorldContextPin);}
	if (SpawnNodeOwnerPin != nullptr) {CompilerContext.MovePinLinksToIntermediate(*SpawnNodeOwnerPin,*CallBeginOwnerPin);}
	//
	//
	UK2Node_CallFunction* CallFinishSpawnNode = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(SpawnNode,SourceGraph);
	CallFinishSpawnNode->FunctionReference.SetExternalMember(FinishSpawningFuncName,UPawnPool::StaticClass());
	CallFinishSpawnNode->AllocateDefaultPins();
	//
	UEdGraphPin* CallFinishExec = CallFinishSpawnNode->GetExecPin();
	UEdGraphPin* CallFinishThen = CallFinishSpawnNode->GetThenPin();
	UEdGraphPin* CallFinishResult = CallFinishSpawnNode->GetReturnValuePin();
	UEdGraphPin* CallFinishActor = CallFinishSpawnNode->FindPinChecked(FK2Node_SpawnPawnFromPoolHelper::ActorPinName);
	UEdGraphPin* CallFinishTransform = CallFinishSpawnNode->FindPinChecked(FK2Node_SpawnPawnFromPoolHelper::SpawnTransformPinName);
	//
	CompilerContext.MovePinLinksToIntermediate(*SpawnNodeThen,*CallFinishThen);
	CompilerContext.CopyPinLinksToIntermediate(*CallBeginTransform,*CallFinishTransform);
	//
	CallBeginResult->MakeLinkTo(CallFinishActor);
	CallFinishResult->PinType = SpawnNodeResult->PinType;
	CompilerContext.MovePinLinksToIntermediate(*SpawnNodeResult,*CallFinishResult);
	//
	//
	UEdGraphPin* LastThen = FKismetCompilerUtilities::GenerateAssignmentNodes(CompilerContext,SourceGraph,CallBeginSpawnNode,SpawnNode,CallBeginResult,ClassToSpawn);
	LastThen->MakeLinkTo(CallFinishExec);
	//
	SpawnNode->BreakAllNodeLinks();
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void UK2Node_SpawnPawnFromPool::MaybeUpdateCollisionPin(TArray<UEdGraphPin*> &OldPins) {
	// see if there's a bNoCollisionFail pin
	for (UEdGraphPin* Pin : OldPins)
	{
		if (Pin->PinName == FK2Node_SpawnPawnFromPoolHelper::NoCollisionFailPinName)
		{
			bool bHadOldCollisionPin = true;
			if (Pin->LinkedTo.Num() == 0)
			{
				// no links, use the default value of the pin
				bool const bOldCollisionPinValue = (Pin->DefaultValue == FString(TEXT("true")));

				UEdGraphPin* const CollisionHandlingOverridePin = GetCollisionHandlingOverridePin();
				if (CollisionHandlingOverridePin)
				{
					UEnum const* const MethodEnum = FindObjectChecked<UEnum>(ANY_PACKAGE, TEXT("ESpawnActorCollisionHandlingMethod"), true);
					CollisionHandlingOverridePin->DefaultValue =
						bOldCollisionPinValue
						? MethodEnum->GetNameStringByIndex(static_cast<int>(ESpawnActorCollisionHandlingMethod::AlwaysSpawn))
						: MethodEnum->GetNameStringByIndex(static_cast<int>(ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding));
				}
			}
			else
			{
				// something was linked. we will just move the links to the new pin
				// #note: this will be an invalid linkage and the BP compiler will complain, and that's intentional
				// so that users will be able to see and fix issues
				UEdGraphPin* const CollisionHandlingOverridePin = GetCollisionHandlingOverridePin();
				check(CollisionHandlingOverridePin);

				UEnum* const MethodEnum = FindObjectChecked<UEnum>(ANY_PACKAGE, TEXT("ESpawnActorCollisionHandlingMethod"), true);
				
				FGraphNodeCreator<UK2Node_EnumLiteral> AlwaysSpawnLiteralCreator(*GetGraph());
				UK2Node_EnumLiteral* const AlwaysSpawnLiteralNode = AlwaysSpawnLiteralCreator.CreateNode();
				AlwaysSpawnLiteralNode->Enum = MethodEnum;
				AlwaysSpawnLiteralNode->NodePosX = NodePosX;
				AlwaysSpawnLiteralNode->NodePosY = NodePosY;
				AlwaysSpawnLiteralCreator.Finalize();
			
				FGraphNodeCreator<UK2Node_EnumLiteral> AdjustIfNecessaryLiteralCreator(*GetGraph());
				UK2Node_EnumLiteral* const AdjustIfNecessaryLiteralNode = AdjustIfNecessaryLiteralCreator.CreateNode();
				AdjustIfNecessaryLiteralNode->Enum = MethodEnum;
				AdjustIfNecessaryLiteralNode->NodePosX = NodePosX;
				AdjustIfNecessaryLiteralNode->NodePosY = NodePosY;
				AdjustIfNecessaryLiteralCreator.Finalize();

				FGraphNodeCreator<UK2Node_Select> SelectCreator(*GetGraph());
				UK2Node_Select* const SelectNode = SelectCreator.CreateNode();
				SelectNode->NodePosX = NodePosX;
				SelectNode->NodePosY = NodePosY;
				SelectCreator.Finalize();

				// find pins we want to set and link up
				auto FindEnumInputPin = [](UK2Node_EnumLiteral const* Node)
				{
					for (UEdGraphPin* NodePin : Node->Pins)
					{
						if (NodePin->PinName == Node->GetEnumInputPinName())
						{
							return NodePin;
						}
					}
					return (UEdGraphPin*)nullptr;
				};

				UEdGraphPin* const AlwaysSpawnLiteralNodeInputPin = FindEnumInputPin(AlwaysSpawnLiteralNode);
				UEdGraphPin* const AdjustIfNecessaryLiteralInputPin = FindEnumInputPin(AdjustIfNecessaryLiteralNode);

				TArray<UEdGraphPin*> SelectOptionPins;
				SelectNode->GetOptionPins(SelectOptionPins);
				UEdGraphPin* const SelectIndexPin = SelectNode->GetIndexPin();

				auto FindResultPin = [](UK2Node const* Node)
				{
					for (UEdGraphPin* NodePin : Node->Pins)
					{
						if (EEdGraphPinDirection::EGPD_Output == NodePin->Direction)
						{
							return NodePin;
						}
					}
					return (UEdGraphPin*)nullptr;
				};
				UEdGraphPin* const AlwaysSpawnLiteralNodeResultPin = FindResultPin(AlwaysSpawnLiteralNode);
				check(AlwaysSpawnLiteralNodeResultPin);
				UEdGraphPin* const AdjustIfNecessaryLiteralResultPin = FindResultPin(AdjustIfNecessaryLiteralNode);
				check(AdjustIfNecessaryLiteralResultPin);

				UEdGraphPin* const OldBoolPin = Pin->LinkedTo[0];
				check(OldBoolPin);

				//
				// now set data and links that we want to set
				//

				AlwaysSpawnLiteralNodeInputPin->DefaultValue = MethodEnum->GetNameStringByIndex(static_cast<int>(ESpawnActorCollisionHandlingMethod::AlwaysSpawn));
				AdjustIfNecessaryLiteralInputPin->DefaultValue = MethodEnum->GetNameStringByIndex(static_cast<int>(ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding));

				OldBoolPin->BreakLinkTo(Pin);
				OldBoolPin->MakeLinkTo(SelectIndexPin);

				AlwaysSpawnLiteralNodeResultPin->MakeLinkTo(SelectOptionPins[0]);
				AdjustIfNecessaryLiteralResultPin->MakeLinkTo(SelectOptionPins[1]);
				
				UEdGraphPin* const SelectOutputPin = SelectNode->GetReturnValuePin();
				check(SelectOutputPin);
				SelectOutputPin->MakeLinkTo(CollisionHandlingOverridePin);

				// tell select node to update its wildcard status
				SelectNode->NotifyPinConnectionListChanged(SelectIndexPin);
				SelectNode->NotifyPinConnectionListChanged(SelectOptionPins[0]);
				SelectNode->NotifyPinConnectionListChanged(SelectOptionPins[1]);
				SelectNode->NotifyPinConnectionListChanged(SelectOutputPin);

			}
		}
	}
}

void UK2Node_SpawnPawnFromPool::ReallocatePinsDuringReconstruction(TArray<UEdGraphPin*> &OldPins) {
	AllocateDefaultPins();
	UClass* UseSpawnClass = GetClassToSpawn(&OldPins);
	if( UseSpawnClass != NULL )
	{
		TArray<UEdGraphPin*> ClassPins;
		CreatePinsForClass(UseSpawnClass, ClassPins);
	}

	MaybeUpdateCollisionPin(OldPins);
	RestoreSplitPins(OldPins);
}

bool UK2Node_SpawnPawnFromPool::IsSpawnVarPin(UEdGraphPin* Pin) {
	const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();
	//
	UEdGraphPin* ParentPin = Pin->ParentPin;
	while (ParentPin) {
		if (ParentPin->PinName == FK2Node_SpawnPawnFromPoolHelper::SpawnTransformPinName) {return false;}
	ParentPin = ParentPin->ParentPin;}
	//
	return (
		Pin->PinName != K2Schema->PN_Execute &&
		Pin->PinName != K2Schema->PN_Then &&
		Pin->PinName != K2Schema->PN_ReturnValue &&
		Pin->PinName != FK2Node_SpawnPawnFromPoolHelper::PoolPinName &&
		Pin->PinName != FK2Node_SpawnPawnFromPoolHelper::OwnerPinName &&
		Pin->PinName != FK2Node_SpawnPawnFromPoolHelper::SuccessPinName &&
		Pin->PinName != FK2Node_SpawnPawnFromPoolHelper::ReconstructPinName &&
		Pin->PinName != FK2Node_SpawnPawnFromPoolHelper::WorldContextPinName &&
		Pin->PinName != FK2Node_SpawnPawnFromPoolHelper::SpawnOptionsPinName &&
		Pin->PinName != FK2Node_SpawnPawnFromPoolHelper::SpawnTransformPinName &&
		Pin->PinName != FK2Node_SpawnPawnFromPoolHelper::CollisionHandlingOverridePinName
	);
}

void UK2Node_SpawnPawnFromPool::PostPlacedNewNode() {
	Super::PostPlacedNewNode();

	UClass* UseSpawnClass = GetClassToSpawn();
	if( UseSpawnClass != NULL )
	{
		TArray<UEdGraphPin*> ClassPins;
		CreatePinsForClass(UseSpawnClass, ClassPins);
	}
}

void UK2Node_SpawnPawnFromPool::OnClassPinChanged() {
 	const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();

	// Remove all pins related to archetype variables
	TArray<UEdGraphPin*> OldPins = Pins;
	TArray<UEdGraphPin*> OldClassPins;

	for (int32 i = 0; i < OldPins.Num(); i++)
	{
		UEdGraphPin* OldPin = OldPins[i];
		if (IsSpawnVarPin(OldPin))
		{
			Pins.Remove(OldPin);
			OldClassPins.Add(OldPin);
		}
	}

	CachedNodeTitle.MarkDirty();

	UClass* UseSpawnClass = GetClassToSpawn();
	TArray<UEdGraphPin*> NewClassPins;
	if (UseSpawnClass != NULL)
	{
		CreatePinsForClass(UseSpawnClass, NewClassPins);
	}

	UEdGraphPin* ResultPin = GetResultPin();
	// Cache all the pin connections to the ResultPin, we will attempt to recreate them
	TArray<UEdGraphPin*> ResultPinConnectionList = ResultPin->LinkedTo;
	// Because the archetype has changed, we break the output link as the output pin type will change
	ResultPin->BreakAllPinLinks();

	// Recreate any pin links to the Result pin that are still valid
	for (UEdGraphPin* Connections : ResultPinConnectionList)
	{
		K2Schema->TryCreateConnection(ResultPin, Connections);
	}

	// Rewire the old pins to the new pins so connections are maintained if possible
	RewireOldPinsToNewPins(OldClassPins, NewClassPins);

	// Destroy the old pins
	DestroyPinList(OldClassPins);

	// Refresh the UI for the graph so the pin changes show up
	UEdGraph* Graph = GetGraph();
	Graph->NotifyGraphChanged();

	// Mark dirty
	FBlueprintEditorUtils::MarkBlueprintAsModified(GetBlueprint());
}

void UK2Node_SpawnPawnFromPool::PinConnectionListChanged(UEdGraphPin* ChangedPin) {
	if (ChangedPin && (ChangedPin->PinName==FK2Node_SpawnPawnFromPoolHelper::PoolPinName)) {OnClassPinChanged();}
}

void UK2Node_SpawnPawnFromPool::PinDefaultValueChanged(UEdGraphPin* ChangedPin) {
	if (ChangedPin && (ChangedPin->PinName==FK2Node_SpawnPawnFromPoolHelper::PoolPinName)) {OnClassPinChanged();}
}

UEdGraphPin* UK2Node_SpawnPawnFromPool::GetThenPin() const {
	const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();
	//
	UEdGraphPin* Pin = FindPinChecked(K2Schema->PN_Then);
	check(Pin->Direction==EGPD_Output); return Pin;
}

UEdGraphPin* UK2Node_SpawnPawnFromPool::GetPoolPin(const TArray<UEdGraphPin*>* InPinsToSearch) const {
	const TArray<UEdGraphPin*>* PinsToSearch = InPinsToSearch ? InPinsToSearch : &Pins;
	//
	UEdGraphPin* Pin = NULL;
	for( auto PinIt = PinsToSearch->CreateConstIterator(); PinIt; ++PinIt )
	{
		UEdGraphPin* TestPin = *PinIt;
		if( TestPin && TestPin->PinName == FK2Node_SpawnPawnFromPoolHelper::PoolPinName )
		{
			Pin = TestPin;
			break;
		}
	}
	check(Pin == NULL || Pin->Direction == EGPD_Input);
	return Pin;
}

UEdGraphPin* UK2Node_SpawnPawnFromPool::GetSpawnOptionsPin() const {
	UEdGraphPin* Pin = FindPinChecked(FK2Node_SpawnPawnFromPoolHelper::SpawnOptionsPinName);
	check(Pin->Direction == EGPD_Input);
	return Pin;
}

UEdGraphPin* UK2Node_SpawnPawnFromPool::GetSpawnTransformPin() const {
	UEdGraphPin* Pin = FindPinChecked(FK2Node_SpawnPawnFromPoolHelper::SpawnTransformPinName);
	check(Pin->Direction == EGPD_Input);
	return Pin;
}

UEdGraphPin* UK2Node_SpawnPawnFromPool::GetCollisionHandlingOverridePin() const {
	UEdGraphPin* const Pin = FindPinChecked(FK2Node_SpawnPawnFromPoolHelper::CollisionHandlingOverridePinName);
	check(Pin->Direction == EGPD_Input);
	return Pin;
}

UEdGraphPin* UK2Node_SpawnPawnFromPool::GetOwnerPin() const {
	UEdGraphPin* Pin = FindPin(FK2Node_SpawnPawnFromPoolHelper::OwnerPinName);
	check(Pin == NULL || Pin->Direction == EGPD_Input);
	return Pin;
}

UEdGraphPin* UK2Node_SpawnPawnFromPool::GetReconstructPin() const {
	UEdGraphPin* Pin = FindPin(FK2Node_SpawnPawnFromPoolHelper::ReconstructPinName);
	check(Pin == NULL || Pin->Direction == EGPD_Input); return Pin;
}

UEdGraphPin* UK2Node_SpawnPawnFromPool::GetWorldContextPin() const {
	UEdGraphPin* Pin = FindPin(FK2Node_SpawnPawnFromPoolHelper::WorldContextPinName);
	check(Pin == NULL || Pin->Direction == EGPD_Input);
	return Pin;
}

UEdGraphPin* UK2Node_SpawnPawnFromPool::GetSuccessPin() const {
	UEdGraphPin* Pin = FindPin(FK2Node_SpawnPawnFromPoolHelper::SuccessPinName);
	check(Pin == NULL || Pin->Direction == EGPD_Output);
	return Pin;
}

UEdGraphPin* UK2Node_SpawnPawnFromPool::GetResultPin() const {
	const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();

	UEdGraphPin* Pin = FindPinChecked(K2Schema->PN_ReturnValue);
	check(Pin->Direction == EGPD_Output);
	return Pin;
}

FLinearColor UK2Node_SpawnPawnFromPool::GetNodeTitleColor() const {
	return Super::GetNodeTitleColor();
}

bool UK2Node_SpawnPawnFromPool::IsCompatibleWithGraph(const UEdGraph* TargetGraph) const  {
	UBlueprint* Blueprint = FBlueprintEditorUtils::FindBlueprintForGraph(TargetGraph);
	return Super::IsCompatibleWithGraph(TargetGraph) && (!Blueprint || (FBlueprintEditorUtils::FindUserConstructionScript(Blueprint) != TargetGraph && Blueprint->GeneratedClass->GetDefaultObject()->ImplementsGetWorld()));
}

void UK2Node_SpawnPawnFromPool::GetNodeAttributes( TArray<TKeyValuePair<FString, FString>> &OutNodeAttributes ) const {
	UClass* ClassToSpawn = GetClassToSpawn();
	const FString ClassToSpawnStr = ClassToSpawn ? ClassToSpawn->GetName() : TEXT( "InvalidClass" );
	OutNodeAttributes.Add( TKeyValuePair<FString, FString>( TEXT( "Type" ), TEXT( "SpawnPawnFromPool" ) ));
	OutNodeAttributes.Add( TKeyValuePair<FString, FString>( TEXT( "Class" ), GetClass()->GetName() ));
	OutNodeAttributes.Add( TKeyValuePair<FString, FString>( TEXT( "Name" ), GetName() ));
	OutNodeAttributes.Add( TKeyValuePair<FString, FString>( TEXT( "PawnClass" ), ClassToSpawnStr ));
}

FNodeHandlingFunctor* UK2Node_SpawnPawnFromPool::CreateNodeHandler(FKismetCompilerContext &CompilerContext) const {
	return new FNodeHandlingFunctor(CompilerContext);
}

bool UK2Node_SpawnPawnFromPool::HasExternalDependencies(TArray<class UStruct*>* OptionalOutput) const {
	UClass* SourceClass = GetClassToSpawn();
	const UBlueprint* SourceBlueprint = GetBlueprint();
	const bool bResult = (SourceClass != NULL) && (SourceClass->ClassGeneratedBy != SourceBlueprint);
	if (bResult && OptionalOutput)
	{
		OptionalOutput->AddUnique(SourceClass);
	}
	const bool bSuperResult = Super::HasExternalDependencies(OptionalOutput);
	return bSuperResult || bResult;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void UK2Node_SpawnPawnFromPool::GetPinHoverText(const UEdGraphPin &Pin, FString &HoverTextOut) const {
	const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();
	//
	if (UEdGraphPin* PoolPin = GetPoolPin()) {
		K2Schema->ConstructBasicPinTooltip(*PoolPin,LOCTEXT("PoolPinDescription","The Object-Pool Owner of Pawn Instance."),PoolPin->PinToolTip);
	} if (UEdGraphPin* TransformPin = GetSpawnTransformPin()) {
		K2Schema->ConstructBasicPinTooltip(*TransformPin,LOCTEXT("TransformPinDescription","The transform to spawn the Pawn with"),TransformPin->PinToolTip);
	} if (UEdGraphPin* CollisionHandlingOverridePin = GetCollisionHandlingOverridePin()) {
		K2Schema->ConstructBasicPinTooltip(*CollisionHandlingOverridePin,LOCTEXT("CollisionHandlingOverridePinDescription","Specifies how to handle collisions at the Spawn Point. If undefined,uses Actor Class Settings."),CollisionHandlingOverridePin->PinToolTip);
	} if (UEdGraphPin* OwnerPin = GetOwnerPin()) {
		K2Schema->ConstructBasicPinTooltip(*OwnerPin,LOCTEXT("OwnerPinDescription","Can be left empty; primarily used for replication or visibility."),OwnerPin->PinToolTip);
	} if (UEdGraphPin* ReconstructPin = GetReconstructPin()) {
		K2Schema->ConstructBasicPinTooltip(*ReconstructPin,LOCTEXT("ReconstructPinDescription","If checked, this will force the Spawning Pawn to re-run its Construction Scripts;\nThis results on expensive respawn operation and will affect performance!\nAvoid pulling Pawns every second with this option enabled."),ReconstructPin->PinToolTip);
	} if (UEdGraphPin* ResultPin = GetResultPin()) {
		K2Schema->ConstructBasicPinTooltip(*ResultPin,LOCTEXT("ResultPinDescription","The Pawn Spawned from the Pool."),ResultPin->PinToolTip);
	} return Super::GetPinHoverText(Pin,HoverTextOut);
}

FText UK2Node_SpawnPawnFromPool::GetTooltipText() const {
	return NodeTooltip;
}

FSlateIcon UK2Node_SpawnPawnFromPool::GetIconAndTint(FLinearColor &OutColor) const {
	static FSlateIcon Icon(TEXT("OBJPoolStyle"),TEXT("ClassIcon.PawnPool"));
	return Icon;
}

void UK2Node_SpawnPawnFromPool::GetMenuActions(FBlueprintActionDatabaseRegistrar &ActionRegistrar) const {
	UClass* ActionKey = GetClass();
	//
	if (ActionRegistrar.IsOpenForRegistration(ActionKey)) {
		UBlueprintNodeSpawner* NodeSpawner = UBlueprintNodeSpawner::Create(GetClass());
		check(NodeSpawner != nullptr);
		ActionRegistrar.AddBlueprintAction(ActionKey, NodeSpawner);
	}
}

FText UK2Node_SpawnPawnFromPool::GetMenuCategory() const {
	return LOCTEXT("PoolCategory","Object Pool");
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#undef LOCTEXT_NAMESPACE

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////