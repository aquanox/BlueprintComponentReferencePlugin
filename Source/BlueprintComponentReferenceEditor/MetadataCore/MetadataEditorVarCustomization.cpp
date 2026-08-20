// Copyright 2024, Aquanox.

#include "MetadataEditorVarCustomization.h"

#include "BlueprintComponentReferenceCustomization.h"
#include "BlueprintComponentReferenceHelper.h"
#include "BlueprintComponentReferenceEditor.h"
#include "BlueprintEditorModule.h"
#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "ScopedTransaction.h"
#include "UObject/WeakFieldPtr.h"

FMetadataEditorVarCustomization::FMetadataEditorVarCustomization(TWeakPtr<IBlueprintEditor> InBlueprintEditorPtr, TWeakObjectPtr<UBlueprint> InBlueprintPtr)
{
	BlueprintEditorPtr = InBlueprintEditorPtr;
	BlueprintPtr = InBlueprintPtr;
}

TSharedPtr<IDetailCustomization> FMetadataEditorVarCustomization::MakeInstance(TSharedPtr<IBlueprintEditor> BlueprintEditor)
{
	const TArray<UObject*>* Objects = (BlueprintEditor.IsValid() ? BlueprintEditor->GetObjectsCurrentlyBeingEdited() : nullptr);
	if (Objects)
	{
		TOptional<UBlueprint*> FinalBlueprint;
		for (UObject* Object : *Objects)
		{
			UBlueprint* Blueprint = Cast<UBlueprint>(Object);
			if (Blueprint == nullptr)
			{
				return nullptr;
			}
			if (FinalBlueprint.IsSet() && FinalBlueprint.GetValue() != Blueprint)
			{
				return nullptr;
			}
			FinalBlueprint = Blueprint;
		}

		if (FinalBlueprint.IsSet())
		{
			return MakeShared<ThisClass>(BlueprintEditor, MakeWeakObjectPtr(FinalBlueprint.GetValue()));
		}
	}

	return nullptr;
}

TSharedPtr<TStructOnScope<FMetadataContainerBase>> FMetadataEditorVarCustomization::CreateContainer() const
{
	TStructOnScope<FMetadataContainerBase> Value;
	Value.InitializeAs<FBlueprintComponentReferenceMetadata>();
	return MakeShared<TStructOnScope<FMetadataContainerBase>>(MoveTemp(Value));
}

void FMetadataEditorVarCustomization::CustomizeDetails(IDetailLayoutBuilder& DetailLayout)
{
	ScopedSettings.Reset();
	PropertiesBeingCustomized.Reset();

	UBlueprint* LocalBlueprint = BlueprintPtr.Get();
	if (!IsValid(LocalBlueprint))
		return;

	TArray<TWeakObjectPtr<UObject>> ObjectsBeingCustomized;
	DetailLayout.GetObjectsBeingCustomized(ObjectsBeingCustomized);
	for (TWeakObjectPtr<UObject>& Obj : ObjectsBeingCustomized)
	{
		UPropertyWrapper* PropertyWrapper = Cast<UPropertyWrapper>(Obj.Get());
		FProperty* PropertyBeingCustomized = PropertyWrapper ? PropertyWrapper->GetProperty() : nullptr;
		if (!PropertyBeingCustomized)
			continue;
		if (!FBlueprintEditorUtils::IsVariableCreatedByBlueprint(LocalBlueprint, PropertyBeingCustomized))
			continue;

		if (FBlueprintComponentReferenceHelper::IsComponentReferenceProperty(PropertyBeingCustomized))
		{
			PropertiesBeingCustomized.Emplace(PropertyBeingCustomized);
		}
	}

	if (PropertiesBeingCustomized.Num() != 1)
	{
		return;
	}

	ScopedSettings = CreateContainer();
	ScopedSettings->Get()->LoadSettingsFromProperty(PropertiesBeingCustomized[0].Get());

	{
		// Put custom category above `Default Value`
		int32 SortOrder = DetailLayout.EditCategory("Variable").GetSortOrder();
		DetailLayout.EditCategory(GetCategoryName()).SetSortOrder(++SortOrder);
		DetailLayout.EditCategory("DefaultValue").SetSortOrder(++SortOrder);
	}

	{
		auto& Builder = DetailLayout.EditCategory(GetCategoryName());
		Builder.InitiallyCollapsed(false);

		for (TFieldIterator<FProperty> It(ScopedSettings->GetStruct()); It; ++It)
		{
			if (It->HasAnyPropertyFlags(CPF_Deprecated|CPF_Transient))
				continue;

			FSimpleDelegate ChangeHandler = FSimpleDelegate::CreateSP(this, &ThisClass::OnContainerPropertyChanged, It->GetFName());

			FAddPropertyParams Params;
			IDetailPropertyRow* PropertyRow = Builder.AddExternalStructureProperty(ScopedSettings, It->GetFName(), EPropertyLocation::Default, Params);
			PropertyRow->ShouldAutoExpand(true);

			TSharedPtr<IPropertyHandle> PropertyHandle = PropertyRow->GetPropertyHandle();
			PropertyHandle->SetOnPropertyValueChanged(ChangeHandler);
			PropertyHandle->SetOnChildPropertyValueChanged(ChangeHandler);
		}
	}
}

void FMetadataEditorVarCustomization::OnContainerPropertyChanged(FName InName)
{
	FScopedTransaction Transaction(INVTEXT("ApplySettingsToProperty"));

	UBlueprint* Blueprint = BlueprintPtr.Get();

	FMetadataContainerBase* Settings = ScopedSettings->Get();
	check(Settings);

	bool bNeedsRefresh = false;

	for (const TWeakFieldPtr<FProperty>& Property : PropertiesBeingCustomized)
	{
		if (FProperty* LocalProperty = Property.Get())
		{
			Settings->ApplySettingsToProperty(Blueprint, LocalProperty, InName);

			if (LocalProperty->FindMetaData(TEXT("RequestBlueprintRefresh")) != nullptr)
			{
				bNeedsRefresh = true;
			}
		}
	}

	if (bNeedsRefresh)
	{
		if (TSharedPtr<IBlueprintEditor> BlueprintEditor = BlueprintEditorPtr.Pin())
		{
			BlueprintEditor->RefreshMyBlueprint();
		}
	}
}
