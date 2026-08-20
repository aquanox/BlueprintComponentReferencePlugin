#pragma once

#include "BlueprintComponentReferenceExtras.h"
#include "BlueprintComponentReferenceCustomization.h"

#if defined(WITH_BCR_EXTRAS) && WITH_BCR_EXTRAS

/**
 * Base class for MeshSocketReference
 */
class BLUEPRINTCOMPONENTREFERENCEEDITOR_API FMeshSocketReferenceCustomization
	: public FBlueprintComponentReferenceCustomization
{
	using Super = FBlueprintComponentReferenceCustomization;
	using ThisClass = FMeshSocketReferenceCustomization;
public:
	FMeshSocketReferenceCustomization();

	virtual void CustomizeHeaderImpl(TSharedRef<IPropertyHandle> InPropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& PTCUtils) override;
	virtual void CustomizeChildImpl(TSharedRef<IPropertyHandle> InPropertyHandle, TSharedRef<IPropertyHandle> InChildPropertyHandle, IDetailChildrenBuilder& StructBuilder, IPropertyTypeCustomizationUtils& PTCUtils) override;


protected:

	virtual void OnComponentSelected(FBlueprintComponentReference& NewValue) override;
	virtual void OnPropertyValueChanged(FName Source) override;

	TSharedPtr<class SWidget> SocketComboButton;
	TArray<TSharedPtr<FString>> SocketDataSource;

	TSharedPtr<SWidget> BuildSocketCombo();
	void UpdateSocketComboData();
	void SocketCombo_OnGetStrings(TArray< TSharedPtr<FString> >&, TArray<TSharedPtr<SToolTip>>&, TArray<bool>&) const;
	void SocketCombo_OnSelect(const FString& Value);


};

#endif
