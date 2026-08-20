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
class BLUEPRINTCOMPONENTREFERENCEEDITOR_API FBlueprintComponentReferenceCustomization
	: public IPropertyTypeCustomization
{
	using ThisClass = FBlueprintComponentReferenceCustomization;
public:
	using FIsSupportedStructFilter = TDelegate<bool(const UScriptStruct*)>;

	FBlueprintComponentReferenceCustomization(FIsSupportedStructFilter InStructFilter = FIsSupportedStructFilter());

	virtual void CustomizeHeader(TSharedRef<IPropertyHandle> InPropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& PTCUtils) override;
	virtual void CustomizeChildren(TSharedRef<IPropertyHandle> InPropertyHandle, IDetailChildrenBuilder& StructBuilder, IPropertyTypeCustomizationUtils& PTCUtils) override;

	virtual bool IsSupportedProperty(TSharedRef<IPropertyHandle> InPropertyHandle) const;

	virtual void CustomizeHeaderImpl(TSharedRef<IPropertyHandle> InPropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& PTCUtils);
	virtual void CustomizeChildImpl(TSharedRef<IPropertyHandle> InPropertyHandle, TSharedRef<IPropertyHandle> InChildPropertyHandle,  IDetailChildrenBuilder& StructBuilder, IPropertyTypeCustomizationUtils& PTCUtils);

protected:

	/** Build a simple debug context for the property */
	FString GetLoggingContextString() const;

	/** Build the combobox widget. */
	TSharedRef<class SComboButton> BuildComboBox();

	/**
	 * Determine the context the customization is used in
	 */
	void DetermineContext();

	/**
	 * Set the BCR value to the property
	 */
	void SetValue(const FBlueprintComponentReference& Value);

	/**
	 * Sets the default value to the property
	 */
	void SetDefaultValue();

	using FValueAndError = TPair<const FBlueprintComponentReference*, FPropertyAccess::Result>;

	/**
	 * Get the value referenced by this widget.
	 *
	 * Value is always either pointer to real BCR memory or a temp storage (in case of errror)
	 */
	FValueAndError GetValue() const;

	/** Callback when the property value changed. */
	virtual void OnPropertyValueChanged(FName Source);

	virtual bool IsComponentReferenceValid(const FBlueprintComponentReference& Value) const;

	virtual bool CanEdit() const;
	virtual bool CanEditChildren() const;

	const FSlateBrush* GetComponentIcon() const;
	FText OnGetComponentName() const;
	FSlateColor OnGetComponentNameColor() const;
	FText OnGetComponentTooltip() const;
	const FSlateBrush* GetStatusIcon() const;

	TSharedRef<SWidget> OnGetMenuContent();
	void UpdateSelectionList();
	TSharedRef<SWidget> BuildComponentSelectionMenu();
	TSharedRef<SWidget> BuildComponentSelectionTable();
	void OnMenuOpenChanged(bool bOpen);

	virtual void OnClear();
	virtual void OnNavigateComponent();
	void OnComponentSelected(TSharedPtr<FComponentInfo> Node);
	virtual void OnComponentSelected(FBlueprintComponentReference& NewValue);

	void CloseComboButton();

	void ResetViewSettings();
	bool TestNode(const TSharedPtr<FComponentInfo>& Node) const;
	bool TestObject(const UObject* Object) const;

#if WITH_BCR_DRAG_DROP
	bool OnVerifyDrag(TSharedPtr<FDragDropOperation> InDragDrop);
	FReply OnDropped(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent);
	FReply OnDrop(TSharedPtr<FDragDropOperation> InDragDrop);
#endif

protected:
	/** */
	FIsSupportedStructFilter StructFilter;
	/** The property handle we are customizing */
	TSharedPtr<IPropertyHandle> PropertyHandle;
	/** The struct type this customization showing */
	TWeakObjectPtr<UScriptStruct> PropertyStruct;
	/** The temp container for the data */
	TStructOnScope<FBlueprintComponentReference> TempPropertyStorage;
	/** Cached hierarchy utilities */
	TSharedPtr<FBlueprintComponentReferenceHelper> ClassHelper;

	/** Main combo button */
	TSharedPtr<SComboButton> ComponentComboButton;

	/** Container with customization view settings */
	FBlueprintComponentReferenceMetadata ViewSettings;

	/** component picker helper */
	TSharedPtr<FComponentPickerContext>	ComponentPickerContext;

	enum class EPropertyState
	{
		// no errors
		Normal,
		// failed to access property data
		BadPropertyAccess,
		// value does not match filters
		BadReference,
		// value points to unknown component
		BadInfo
	};
	/** represents current state of customization since last update */
	EPropertyState PropertyState = EPropertyState::Normal;
	/** currently selected node */
	TWeakPtr<FComponentInfo> CachedComponentNode;

	TArray<FComponentPickerGroup> CachedChoosableElements;

	FString CachedContextString;
};
