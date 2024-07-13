// Copyright 2024, Aquanox.

#pragma once

#include "BlueprintComponentReference.h"
#include "BlueprintComponentReferenceHelper.h"
#include "IDetailCustomNodeBuilder.h"
#include "IPropertyTypeCustomization.h"
#include "PropertyHandle.h"
#include "Containers/Array.h"
#include "PropertyEditorModule.h"
#include "Internationalization/Text.h"
#include "Styling/SlateBrush.h"
#include "Templates/SharedPointer.h"

class FMenuBuilder;
class SComboButton;

/**
 * Clas holding BCR filter settings
 */
struct FBlueprintComponentReferenceViewSettings // : public FBlueprintComponentReferenceMetadata
{
	/** Whether we allow to use Picker feature */
	bool bUsePicker = true;

	/** Whether we allow to use Navigate/Browse feature */
	bool bUseNavigate = true;
	/** Whether the asset can be 'None' in this case */
	bool bUseClear = true;

	/** Whether we allow to pick native components */
	bool bShowNative = true;
	/** Whether we allow to pick blueprint components */
	bool bShowBlueprint = true;
	/** Whether we allow to pick instanced components */
	bool bShowInstanced = true;
	/** Whether we allow to pick path-only components */
	bool bShowPathOnly = true;

	/** Classes that can be used with this property */
	TArray<TWeakObjectPtr<UClass>> AllowedClasses;

	/** Classes that can NOT be used with this property */
	TArray<TWeakObjectPtr<UClass>> DisallowedClasses;

	/** Interfaces that must be implemented to be eligible for this property */
	//TArray<TWeakObjectPtr<UClass>> RequiredInterfaces;

	void Reset();

	bool IsFilteredNode(const TSharedPtr<FComponentInfo>& Node) const;
	bool IsFilteredObject(const UObject* Object) const;
};

/**
 * Component reference cutomization class
 */
class FBlueprintComponentReferenceCustomization : public IPropertyTypeCustomization
{
public:
	/** Makes a new instance of this customization for a specific detail view requesting it */
	static TSharedRef<IPropertyTypeCustomization> MakeInstance();

	/** IPropertyTypeCustomization interface */
	virtual void CustomizeHeader(TSharedRef<IPropertyHandle> InPropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& PropertyTypeCustomizationUtils) override;
	virtual void CustomizeChildren(TSharedRef<IPropertyHandle> InPropertyHandle, IDetailChildrenBuilder& StructBuilder, IPropertyTypeCustomizationUtils& PropertyTypeCustomizationUtils) override;

private:
	/* Build a simple debug context for the property */
	FString GetLoggingContextString() const;

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
	void OnPropertyValueChanged(FName Source);

	/** */
	bool IsComponentReferenceValid(const FBlueprintComponentReference& Value) const;

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
	void OnNavigateComponent();

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
	/* Cached hierarchy utilities */
	TSharedPtr<FBlueprintComponentReferenceHelper> ClassHelper;

	/** Main combo button */
	TSharedPtr<SComboButton> ComponentComboButton;

	/** */
	FBlueprintComponentReferenceViewSettings Settings;

	/* component picker helper */
	TSharedPtr<FComponentPickerContext>	ComponentPickerContext;
	/* currently selected node*/
	TWeakPtr<FComponentInfo> CachedComponentNode;
	/* last call property access state */
	FPropertyAccess::Result CachedPropertyAccess = FPropertyAccess::Result::Fail;
};
