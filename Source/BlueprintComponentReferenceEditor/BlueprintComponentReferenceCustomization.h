// Copyright 2024, Aquanox.

#pragma once

#include "BlueprintComponentReference.h"
#include "BlueprintComponentReferenceHelper.h"
#include "IPropertyTypeCustomization.h"
#include "PropertyHandle.h"
#include "Containers/Array.h"
#include "PropertyEditorModule.h"
#include "Internationalization/Text.h"
#include "Styling/SlateBrush.h"
#include "Templates/SharedPointer.h"

class FMenuBuilder;

class FBlueprintComponentReferenceCustomization : public IPropertyTypeCustomization
{
public:
	/** Makes a new instance of this customization for a specific detail view requesting it */
	static TSharedRef<IPropertyTypeCustomization> MakeInstance();

	/** IPropertyTypeCustomization interface */
	virtual void CustomizeHeader(TSharedRef<IPropertyHandle> InPropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& PropertyTypeCustomizationUtils) override;
	virtual void CustomizeChildren(TSharedRef<IPropertyHandle> InPropertyHandle, IDetailChildrenBuilder& StructBuilder, IPropertyTypeCustomizationUtils& PropertyTypeCustomizationUtils) override;

	FString GetLoggingContextString() const;
private:
	/** From the property metadata, build the list of allowed and disallowed class. */
	void BuildClassFilters();

	/** Build the combobox widget. */
	void BuildComboBox();

	/**
	 * From the Detail panel outer hierarchy, find the first actor or component owner we find.
	 * This is use in case we want only component on the Self actor and to check if we did a cross-level reference.
	 */
	void DetermineOuterActor();

	/**
	 * Set the value of the asset referenced by this property editor.
	 * Will set the underlying property handle if there is one.
	 */
	void SetValue(const FBlueprintComponentReference& Value);

	/** Get the value referenced by this widget. */
	FPropertyAccess::Result GetValue(FBlueprintComponentReference& OutValue) const;

	/** Callback when the property value changed. */
	void OnPropertyValueChanged();
	/** Is this component allowed to be picked in this chooser */
	bool IsFilteredNode(TSharedPtr<struct FComponentInfo> Node) const;

	bool IsComponentReferenceValid(const FBlueprintComponentReference& Value) const;


private:
	bool CanEdit() const;
	bool CanEditChildren() const;

	const FSlateBrush* GetComponentIcon() const;
	FText OnGetComponentName() const;
	FText OnGetComponentTooltip() const;
	const FSlateBrush* GetStatusIcon() const;

	/**
	 * Get the content to be displayed in the picker menu
	 */
	TSharedRef<SWidget> OnGetMenuContent();
	/**
	 * Generate menu
	 */
	void AddMenuNodes(FMenuBuilder& MenuBuilder, TSharedRef<struct FComponentInfo> Node, FString Path);
	/**
	 * Called when the menu is closed, we handle this to force the destruction of the menu
	 */
	void OnMenuOpenChanged(bool bOpen);

	/**
	 *
	 */
	void OnClear();

	/**
	 *
	 */
	void OnBrowseComponent();

	/**
	 * Returns whether the actor/component should be filtered out from selection.
	 */
	bool IsFilteredObject(const UObject* Object) const;

	/**
	 * Delegate for handling selection in the scene outliner.
	 */
	void OnComponentSelected(TSharedRef<FComponentInfo> Node);

	/**
	 * Closes the combo button.
	 */
	void CloseComboButton();

private:
	/** The property handle we are customizing */
	TSharedPtr<IPropertyHandle> PropertyHandle;
	/** Cached property utilities */
	TSharedPtr<IPropertyUtilities> PropertyUtilities;

	/** Main combo button */
	TSharedPtr<class SComboButton> ComponentComboButton;

	/** Classes that can be used with this property */
	TArray<const UClass*> AllowedComponentClassFilters;

	/** Classes that can NOT be used with this property */
	TArray<const UClass*> DisallowedComponentClassFilters;

	/** Interfaces that must be implemented */
	TArray<const UClass*> RequiredInterfaceFilters;

	/** Whether we allow to use Picker feature */
	bool bAllowPicker = true;

	/** Whether we allow to use Browse feature */
	bool bAllowNavigate = true;
	/** Whether the asset can be 'None' in this case */
	bool bAllowClear = true;

	/** Whether we allow to pick native components */
	bool bAllowNative = true;
	/** Whether we allow to pick blueprint components */
	bool bAllowBlueprint = true;
	/** Whether we allow to pick instanced components */
	bool bAllowInstanced = true;
	/** Whether we allow to pick path-only components */
	bool bAllowPathOnly = true;

	/* */
	TSharedPtr<FBlueprintComponentReferenceHelper> ClassHelper;
	/* component picker helper */
	TSharedPtr<FChooserContext>	ChooserContext;
	/* currently selected node*/
	TWeakPtr<FComponentInfo> CachedComponentNode;
	/* last call property access state */
	FPropertyAccess::Result CachedPropertyAccess = FPropertyAccess::Result::Fail;
};
