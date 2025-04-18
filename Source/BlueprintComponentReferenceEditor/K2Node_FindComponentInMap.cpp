﻿// Copyright 2024, Aquanox.

#include "K2Node_FindComponentInMap.h"

#include "BlueprintActionDatabaseRegistrar.h"
#include "BlueprintComponentReferenceLibrary.h"
#include "BlueprintNodeSpawner.h"
#include "EdGraphSchema_K2.h"
#include "Kismet2/WildcardNodeUtils.h"

UK2Node_FindComponentInMap::UK2Node_FindComponentInMap()
{
	FunctionReference.SetExternalMember(
		GET_FUNCTION_NAME_CHECKED(UBlueprintComponentReferenceLibrary, Map_FindComponent),
		UBlueprintComponentReferenceLibrary::StaticClass()
	);
}

FText UK2Node_FindComponentInMap::GetMenuCategory() const
{
	// return INVTEXT("Utilities|ComponentReference");
	return Super::GetMenuCategory();
}

void UK2Node_FindComponentInMap::AllocateDefaultPins()
{
	Super::AllocateDefaultPins();

	ConformPinTypes();
}

void UK2Node_FindComponentInMap::NotifyPinConnectionListChanged(UEdGraphPin* Pin)
{
	Super::NotifyPinConnectionListChanged(Pin);
	
	ConformPinTypes();
}

void UK2Node_FindComponentInMap::PostReconstructNode()
{
	Super::PostReconstructNode();
	
	ConformPinTypes();
}
// @see UK2node_CAllFunction::ConformContainerPins
void UK2Node_FindComponentInMap::ConformPinTypes()
{
	const UEdGraphSchema_K2* Schema = CastChecked<UEdGraphSchema_K2>(GetSchema());

	const auto TryReadTypeToPropagate = [](UEdGraphPin* Pin, bool& bOutPropagated, FEdGraphTerminalType& TypeToPropagete)
	{
		if (Pin && !bOutPropagated)
		{
			if (Pin->HasAnyConnections() || !Pin->DoesDefaultValueMatchAutogenerated() )
			{
				FEdGraphTerminalType TypeToPotentiallyPropagate;
				if (Pin->LinkedTo.Num() != 0)
				{
					TypeToPotentiallyPropagate = Pin->LinkedTo[0]->GetPrimaryTerminalType();
				}
				else
				{
					TypeToPotentiallyPropagate = Pin->GetPrimaryTerminalType();
				}
				
				if (TypeToPotentiallyPropagate.TerminalCategory != UEdGraphSchema_K2::PC_Wildcard)
				{
					bOutPropagated = true;
					TypeToPropagete = TypeToPotentiallyPropagate;
				}
			}
		}
	};
	
	const auto TryPropagateType = [Schema](UEdGraphPin* Pin, const FEdGraphTerminalType& TerminalType, bool bTypeIsAvailable)
	{
		if(Pin)
		{
			if(bTypeIsAvailable)
			{
				const FEdGraphTerminalType PrimaryType = Pin->GetPrimaryTerminalType();
				if( PrimaryType.TerminalCategory != TerminalType.TerminalCategory ||
					PrimaryType.TerminalSubCategory != TerminalType.TerminalSubCategory ||
					PrimaryType.TerminalSubCategoryObject != TerminalType.TerminalSubCategoryObject)
				{
					// terminal type changed:
					if (Pin->SubPins.Num() > 0 && Pin->PinType.PinCategory != UEdGraphSchema_K2::PC_Wildcard)
					{
						Schema->RecombinePin(Pin->SubPins[0]);
					}

					Pin->PinType.PinCategory = TerminalType.TerminalCategory;
					Pin->PinType.PinSubCategory = TerminalType.TerminalSubCategory;
					Pin->PinType.PinSubCategoryObject = TerminalType.TerminalSubCategoryObject;

					// Also propagate the CPF_UObjectWrapper flag, which will be set for "wrapped" object ptr types (e.g. TSubclassOf).
					Pin->PinType.bIsUObjectWrapper = TerminalType.bTerminalIsUObjectWrapper;
				
					// Reset default values
					if (!Schema->IsPinDefaultValid(Pin, Pin->DefaultValue, Pin->DefaultObject, Pin->DefaultTextValue).IsEmpty())
					{
						Schema->ResetPinToAutogeneratedDefaultValue(Pin, false);
					}
				}
			}
			else
			{
				// reset to wildcard:
				if (Pin->SubPins.Num() > 0)
				{
					Schema->RecombinePin(Pin->SubPins[0]);
				}

				Pin->PinType.PinCategory = UEdGraphSchema_K2::PC_Wildcard;
				Pin->PinType.PinSubCategory = NAME_None;
				Pin->PinType.PinSubCategoryObject = nullptr;
				Pin->PinType.bIsUObjectWrapper = false;
				Schema->ResetPinToAutogeneratedDefaultValue(Pin, false);
			}
		}
	};
	
	const UFunction* TargetFunction = GetTargetFunction();
	if (TargetFunction == nullptr)
	{
		return;
	}

	const FString& MapPinMetaData = TargetFunction->GetMetaData(FBlueprintMetadata::MD_MapParam);
	
	if (UEdGraphPin* MapPin = FindPinChecked(MapPinMetaData))
	{
		bool bReadyToPropagateKeyType = true;
		FEdGraphTerminalType KeyTypeToPropagate;
		TryReadTypeToPropagate(MapPin, bReadyToPropagateKeyType, KeyTypeToPropagate);
		
		KeyTypeToPropagate.TerminalCategory = UEdGraphSchema_K2::PC_Struct;
		KeyTypeToPropagate.TerminalSubCategoryObject = StaticStruct<FBlueprintComponentReference>();

		TryPropagateType(MapPin, KeyTypeToPropagate, bReadyToPropagateKeyType);
	}
}

void UK2Node_FindComponentInMap::GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const
{
	UClass* const ActionKey = GetClass();
	if (ActionRegistrar.IsOpenForRegistration(ActionKey))
	{
		UBlueprintNodeSpawner* NodeSpawner = UBlueprintNodeSpawner::Create(GetClass());
		check(NodeSpawner != nullptr);
		ActionRegistrar.AddBlueprintAction(ActionKey, NodeSpawner);
	}
}
