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
class BLUEPRINTCOMPONENTREFERENCEEDITOR_API FMetadataEditorVarCustomization : public IDetailCustomization
{
	using ThisClass = FMetadataEditorVarCustomization;
public:
	FMetadataEditorVarCustomization(
		TWeakPtr<IBlueprintEditor> InBlueprintEditor,
		TWeakObjectPtr<UBlueprint> InBlueprintPtr
	);

	static TSharedPtr<IDetailCustomization> MakeInstance(TSharedPtr<IBlueprintEditor> BlueprintEditor);

	virtual FName GetCategoryName() const { return TEXT("ComponentReferenceMetadata"); }
	virtual TSharedPtr<TStructOnScope<FMetadataContainerBase>> CreateContainer() const;

	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailLayout) override;

	virtual void OnContainerPropertyChanged(FName InName);
private:
	/** The blueprint editor instance */
	TWeakPtr<IBlueprintEditor> BlueprintEditorPtr;

	/** The blueprint we are editing */
	TWeakObjectPtr<UBlueprint> BlueprintPtr;

	/** The property we are editing */
	TArray<TWeakFieldPtr<FProperty>> PropertiesBeingCustomized;

	/** Object holding aggregate settins to be applied to properties */
	TSharedPtr<TStructOnScope<FMetadataContainerBase>> ScopedSettings;

};
