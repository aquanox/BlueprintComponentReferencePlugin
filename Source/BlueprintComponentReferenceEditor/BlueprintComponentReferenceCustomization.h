// Copyright 2024, Aquanox.

#pragma once

#include "BlueprintComponentReference.h"
#include "BlueprintComponentReferenceMetadata.h"
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
class FDragDropEvent;

// Enable Drag&Drop function on a customization. Requires engine patch.
// See FBlueprintComponentReferenceCustomization::OnVerifyDrag definition for details
#ifndef WITH_BCR_DRAG_DROP
#define WITH_BCR_DRAG_DROP 0
#endif

/**
 * Component reference cutomization class
 */
class FBlueprintComponentReferenceCustomization : public IPropertyTypeCustomization
{
public:
	/** Makes a new instance of this customization for a specific detail view requesting it */
	static TSharedRef<IPropertyTypeCustomization> MakeInstance();

	virtual void CustomizeHeader(TSharedRef<IPropertyHandle> InPropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& PropertyTypeCustomizationUtils) override;
	virtual void CustomizeChildren(TSharedRef<IPropertyHandle> InPropertyHandle, IDetailChildrenBuilder& StructBuilder, IPropertyTypeCustomizationUtils& PropertyTypeCustomizationUtils) override;

private:
	/** Build a simple debug context for the property */
	FString GetLoggingContextString() const;
	FString CachedContextString;

	/** Build the combobox widget. */
	void BuildComboBox();

	/**
	 * Determine the context the customization is used in
	 */
	void DetermineContext();

	/**
	 * Set the value of the asset referenced by this property editor.
	 * Will set the underlying property handle if there is one.
	 */
	void SetValue(const FBlueprintComponentReference& Value);

	/** Get the value referenced by this widget. */
	FPropertyAccess::Result GetValue(FBlueprintComponentReference& OutValue) const;

	/** Callback when the property value changed. */
	void OnPropertyValueChanged(FName Source);

	bool IsComponentReferenceValid(const FBlueprintComponentReference& Value) const;

	bool CanEdit() const;
	bool CanEditChildren() const;

	const FSlateBrush* GetComponentIcon() const;
	FText OnGetComponentName() const;
	FText OnGetComponentTooltip() const;
	const FSlateBrush* GetStatusIcon() const;

	TSharedRef<SWidget> OnGetMenuContent();
	void OnMenuOpenChanged(bool bOpen);

	void OnClear();
	void OnNavigateComponent();
	void OnComponentSelected(TSharedPtr<FComponentInfo> Node);

	void CloseComboButton();

	void ResetViewSettings();
	bool TestNode(const TSharedPtr<FComponentInfo>& Node) const;
	bool TestObject(const UObject* Object) const;

#if WITH_BCR_DRAG_DROP
	bool OnVerifyDrag(TSharedPtr<FDragDropOperation> InDragDrop);
	FReply OnDropped(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent);
	FReply OnDrop(TSharedPtr<FDragDropOperation> InDragDrop);
#endif

private:
	/** The property handle we are customizing */
	TSharedPtr<IPropertyHandle> PropertyHandle;
	/** Cached hierarchy utilities */
	TSharedPtr<FBlueprintComponentReferenceHelper> ClassHelper;

	/** Main combo button */
	TSharedPtr<SComboButton> ComponentComboButton;

	/** Container with customization view settings */
	FBlueprintComponentReferenceMetadata ViewSettings;

	/** component picker helper */
	TSharedPtr<FComponentPickerContext>	ComponentPickerContext;
	/** currently selected node */
	TWeakPtr<FComponentInfo> CachedComponentNode;
	/** last call property access state */
	FPropertyAccess::Result CachedPropertyAccess = FPropertyAccess::Result::Fail;

	struct FSelectionData
	{
		TSharedPtr<FHierarchyInfo>			Category;
		TArray<TSharedPtr<FComponentInfo>>	Elements;
	};
	TArray<FSelectionData> CachedChoosableElements;
};
