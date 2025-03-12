// Copyright 2024, Aquanox.

#include "BlueprintComponentReferenceVarCustomization.h"

#include "BlueprintComponentReferenceCustomization.h"
#include "BlueprintComponentReferenceHelper.h"
#include "BlueprintComponentReferenceEditor.h"
#include "BlueprintEditorModule.h"
#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "ScopedTransaction.h"

FBlueprintComponentReferenceVarCustomization::FBlueprintComponentReferenceVarCustomization(TSharedPtr<IBlueprintEditor> InBlueprintEditor, TWeakObjectPtr<UBlueprint> InBlueprintPtr)
{
	BlueprintEditorPtr = InBlueprintEditor;
	BlueprintPtr		= InBlueprintPtr;
}

TSharedPtr<IDetailCustomization> FBlueprintComponentReferenceVarCustomization::MakeInstance(TSharedPtr<IBlueprintEditor> BlueprintEditor)
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
			return MakeShared<FBlueprintComponentReferenceVarCustomization>(BlueprintEditor, MakeWeakObjectPtr(FinalBlueprint.GetValue()));
		}
	}

	return nullptr;
}

TSharedPtr<TStructOnScope<FBlueprintComponentReferenceVarCustomization::FMetadataContainer>> FBlueprintComponentReferenceVarCustomization::CreateContainer() const
{
	return MakeShared<TStructOnScope<FMetadataContainer>>(MakeStructOnScope<FMetadataContainer>());
}

void FBlueprintComponentReferenceVarCustomization::CustomizeDetails(IDetailLayoutBuilder& DetailLayout)
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

		for (TFieldIterator<FProperty> It(ScopedSettings->GetStruct(), EFieldIterationFlags::Default); It; ++It)
		{
			if (It->HasAnyPropertyFlags(CPF_Deprecated|CPF_Transient))
				continue;
			
			FSimpleDelegate ChangeHandler = FSimpleDelegate::CreateSP(this, &FBlueprintComponentReferenceVarCustomization::OnContainerPropertyChanged, It->GetFName());

			FAddPropertyParams Params;
			IDetailPropertyRow* PropertyRow = Builder.AddExternalStructureProperty(ScopedSettings, It->GetFName(), EPropertyLocation::Default, Params);
			PropertyRow->ShouldAutoExpand(true);

			TSharedPtr<IPropertyHandle> PropertyHandle = PropertyRow->GetPropertyHandle();
			PropertyHandle->SetOnPropertyValueChanged(ChangeHandler);
			PropertyHandle->SetOnChildPropertyValueChanged(ChangeHandler);
		}
	}
}

void FBlueprintComponentReferenceVarCustomization::OnContainerPropertyChanged(FName InName)
{
	FScopedTransaction Transaction(INVTEXT("ApplySettingsToProperty"));

	FMetadataContainer& Settings = *ScopedSettings->Get();
	
	for (const TWeakFieldPtr<FProperty>& Property : PropertiesBeingCustomized)
	{
		if (FProperty* Local = Property.Get())
		{
			Settings.ApplySettingsToProperty(BlueprintPtr.Get(), Local, InName);
		}
	}
}
