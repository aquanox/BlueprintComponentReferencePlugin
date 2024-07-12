// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "BlueprintComponentReferenceUtils.h"
#include "IDetailCustomization.h"

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

	void LoadSettingsFromProperty(UBlueprint* InBlueprint, const FProperty* InProp);
	void ApplySettingsToProperty(FProperty* InProp);

private:
	/** The blueprint editor instance */
	TSharedPtr<IBlueprintEditor>	BlueprintEditorPtr;

	/** The blueprint we are editing */
	TWeakObjectPtr<UBlueprint>		BlueprintPtr;

	/** The property we are editing */
	TArray<TWeakFieldPtr<FProperty>> PropertiesBeingCustomized;

	/** Object holding aggregate settins to be applied to properties */
	TSharedRef<TStructOnScope<FBlueprintComponentReferenceExtras>> ScopedSettings;

};



#if 0

class FBCRClassFilter : public IClassViewerFilter
{
public:
	/** All children of these classes will be included unless filtered out by another setting. */
	TSet< const UClass* > AllowedChildrenOfClasses;

	virtual bool IsClassAllowed(const FClassViewerInitializationOptions& InInitOptions, const UClass* InClass, TSharedRef< FClassViewerFilterFuncs > InFilterFuncs ) override
	{
		// If it appears on the allowed child-of classes list (or there is nothing on that list)
		return InFilterFuncs->IfInChildOfClassesSet( AllowedChildrenOfClasses, InClass) != EFilterReturn::Failed;
	}

	virtual bool IsUnloadedClassAllowed(const FClassViewerInitializationOptions& InInitOptions, const TSharedRef< const IUnloadedBlueprintData > InUnloadedClassData, TSharedRef< FClassViewerFilterFuncs > InFilterFuncs) override
	{
		// If it appears on the allowed child-of classes list (or there is nothing on that list)
		return InFilterFuncs->IfInChildOfClassesSet( AllowedChildrenOfClasses, InUnloadedClassData) != EFilterReturn::Failed;
	}
};


class FBlueprintComponentReferenceViewCustomization : public IDetailCustomNodeBuilder, public TSharedFromThis<FBlueprintComponentReferenceViewCustomization>
{
public:
	FBlueprintComponentReferenceViewCustomization(TSharedRef<IPropertyHandle> InPropertyHandle, TSharedRef<FBlueprintComponentReferenceViewSettings> InSettings);

public:
	virtual void GenerateHeaderRowContent(FDetailWidgetRow& NodeRow) override;
	virtual void GenerateChildContent(IDetailChildrenBuilder& ChildrenBuilder) override;
	virtual FName GetName() const override;
	virtual TSharedPtr<IPropertyHandle> GetPropertyHandle() const override;


	static ECheckBoxState ToBool(bool b) { return b ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; }
	static bool FromBool(ECheckBoxState b) { return b == ECheckBoxState::Checked; }
	static ECheckBoxState InvertState(ECheckBoxState s)
	{
		return s == ECheckBoxState::Checked ? ECheckBoxState::Unchecked : ECheckBoxState::Checked;
	}

	ECheckBoxState	GetShowNative() const;
	void			ToggleShowNative() { OnShowNativeChanged(InvertState(GetShowNative())); }
	void			OnShowNativeChanged(ECheckBoxState Value);

	ECheckBoxState	GetShowBlueprint() const;
	void			ToggleShowBlueprint() { OnShowBlueprintChanged(InvertState(GetShowBlueprint())); }
	void			OnShowBlueprintChanged(ECheckBoxState Value);

	ECheckBoxState	GetShowInstanced() const;
	void			ToggleShowInstanced() { OnShowInstancedChanged(InvertState(GetShowInstanced())); }
	void			OnShowInstancedChanged(ECheckBoxState Value);

	ECheckBoxState	GetShowPathOnly() const;
	void			ToggleShowPathOnly() { OnShowPathOnlyChanged(InvertState(GetShowPathOnly())); }
	void			OnShowPathOnlyChanged(ECheckBoxState Value);

	FText			GetBaseClassText() const;
	TSharedRef<SWidget> GenerateClassPicker() const;
	void			OnBaseClassPicked(UClass* InClass);

	void				ApplySettings();
private:

	TSharedRef<IPropertyHandle> TargetHandle;
	TSharedRef<FBlueprintComponentReferenceViewSettings> TargetSettings;
};

#endif
