// Copyright 2024, Aquanox.

#pragma once

#include "BlueprintComponentReferenceLibrary.h"
#include "IDetailCustomization.h"
#include "IDetailCustomNodeBuilder.h"

class IBlueprintEditor;
class UBlueprint;

/**
 *
 */
class  FBlueprintComponentReferenceVarCustomization : public IDetailCustomization
{
public:
	FBlueprintComponentReferenceVarCustomization(TSharedPtr<IBlueprintEditor> InBlueprintEditor, TWeakObjectPtr<UBlueprint> InBlueprintPtr);

	static TSharedPtr<IDetailCustomization> MakeInstance(TSharedPtr<IBlueprintEditor> BlueprintEditor);
protected:

	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailLayout) override;

	void OnPropertyChanged(FName InName);

	void LoadSettingsFromProperty(const FProperty* InProp);
	void ApplySettingsToProperty(FProperty* InProp, const FName& InChanged);

private:
	/** The blueprint editor instance */
	TSharedPtr<IBlueprintEditor>	BlueprintEditorPtr;

	/** The blueprint we are editing */
	TWeakObjectPtr<UBlueprint>		BlueprintPtr;

	/** The property we are editing */
	TArray<TWeakFieldPtr<FProperty>> PropertiesBeingCustomized;

	/** Object holding aggregate settins to be applied to properties */
	TSharedRef<TStructOnScope<FBlueprintComponentReferenceMetadata>> ScopedSettings;

};
