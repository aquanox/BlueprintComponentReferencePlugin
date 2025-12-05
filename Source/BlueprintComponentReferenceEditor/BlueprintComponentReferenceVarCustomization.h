// Copyright 2024, Aquanox.

#pragma once

#include "BlueprintComponentReferenceLibrary.h"
#include "BlueprintComponentReferenceMetadata.h"
#include "IDetailCustomization.h"
#include "IDetailCustomNodeBuilder.h"
#include "UObject/WeakFieldPtr.h"

class IBlueprintEditor;
class UBlueprint;

/**
 *
 */
class BLUEPRINTCOMPONENTREFERENCEEDITOR_API FBlueprintComponentReferenceVarCustomization : public IDetailCustomization
{
	using FMetadataContainer = FBlueprintComponentReferenceMetadata;
public:
	FBlueprintComponentReferenceVarCustomization(
		TSharedPtr<IBlueprintEditor> InBlueprintEditor,
		TWeakObjectPtr<UBlueprint> InBlueprintPtr
	);

	static TSharedPtr<IDetailCustomization> MakeInstance(TSharedPtr<IBlueprintEditor> BlueprintEditor);
protected:
	virtual FName GetCategoryName() const { return TEXT("ComponentReferenceMetadata"); }
	virtual TSharedPtr<TStructOnScope<FMetadataContainer>> CreateContainer() const;

	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailLayout) override;

	virtual void OnContainerPropertyChanged(FName InName);

private:
	/** The blueprint editor instance */
	TSharedPtr<IBlueprintEditor>	BlueprintEditorPtr;

	/** The blueprint we are editing */
	TWeakObjectPtr<UBlueprint>		BlueprintPtr;

	/** The property we are editing */
	TArray<TWeakFieldPtr<FProperty>> PropertiesBeingCustomized;

	/** Object holding aggregate settins to be applied to properties */
	TSharedPtr<TStructOnScope<FMetadataContainer>> ScopedSettings;

};
