// Copyright 2024, Aquanox.

#pragma once

#include "CoreMinimal.h"
#include "K2Node_CallFunction.h"
#include "K2Node_FindComponentInMap.generated.h"

/**
 * Experimental helper that can lookup for a raw component pointer within TMap<ComponentReference, GenericValue>
 *
 * This node is a generic wrapper over function that enforces map key pin type
 *
 * @see UBlueprintComponentReferenceLibrary::Map_FindComponent
 */
UCLASS(MinimalAPI)
class UK2Node_FindComponentInMap : public UK2Node_CallFunction
{
	GENERATED_BODY()
public:
	UK2Node_FindComponentInMap();

	virtual FText GetMenuCategory() const override;
	virtual void AllocateDefaultPins() override;
	virtual void NotifyPinConnectionListChanged(UEdGraphPin* Pin) override;
	virtual void PostReconstructNode() override;

	void ConformPinTypes();

	virtual void GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const override;
};
