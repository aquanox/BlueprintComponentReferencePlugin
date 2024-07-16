// Copyright 2024, Aquanox.

#include "BlueprintComponentReferenceVarCustomization.h"

#include "BlueprintComponentReferenceCustomization.h"
#include "BlueprintComponentReferenceHelper.h"
#include "BlueprintComponentReferenceUtils.h"
#include "BlueprintEditorModule.h"
#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "ScopedTransaction.h"

DEFINE_LOG_CATEGORY_STATIC(BCRVariableCustomization, Log, All);

FBlueprintComponentReferenceVarCustomization::FBlueprintComponentReferenceVarCustomization(TSharedPtr<IBlueprintEditor> InBlueprintEditor, TWeakObjectPtr<UBlueprint> InBlueprintPtr)
	: ScopedSettings(MakeShared<TStructOnScope<FBlueprintComponentReferenceMetadata>>())
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

void FBlueprintComponentReferenceVarCustomization::CustomizeDetails(IDetailLayoutBuilder& DetailLayout)
{
	ScopedSettings->InitializeFrom(MakeStructOnScope<FBlueprintComponentReferenceMetadata>());

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
			LoadSettingsFromProperty(PropertyBeingCustomized);

			PropertiesBeingCustomized.Emplace(PropertyBeingCustomized);
		}
	}

	if (PropertiesBeingCustomized.Num() != 1)
	{ // todo: fix multiedit
		return;
	}

	{
		// Put custom category above `Default Value`
		int32 SortOrder = DetailLayout.EditCategory("Variable").GetSortOrder();
		DetailLayout.EditCategory("ComponentReference").SetSortOrder(++SortOrder);
		DetailLayout.EditCategory("DefaultValue").SetSortOrder(++SortOrder);
	}

	{
		auto& Builder = DetailLayout.EditCategory("ComponentReference");
		Builder.InitiallyCollapsed(false);

		for (TFieldIterator<FProperty> It(ScopedSettings->GetStruct(), EFieldIterationFlags::Default); It; ++It)
		{
			FSimpleDelegate ChangeHandler = FSimpleDelegate::CreateSP(this, &FBlueprintComponentReferenceVarCustomization::OnPropertyChanged, It->GetFName());

			FAddPropertyParams Params;
			IDetailPropertyRow* PropertyRow = Builder.AddExternalStructureProperty(ScopedSettings, It->GetFName(), EPropertyLocation::Default, Params);
			PropertyRow->ShouldAutoExpand(true);

			TSharedPtr<IPropertyHandle> PropertyHandle = PropertyRow->GetPropertyHandle();
			PropertyHandle->SetOnPropertyValueChanged(ChangeHandler);
			PropertyHandle->SetOnChildPropertyValueChanged(ChangeHandler);
		}
	}
}

void FBlueprintComponentReferenceVarCustomization::OnPropertyChanged(FName InName)
{
	UE_LOG(BCRVariableCustomization, Log, TEXT("OnPropertyChanged(%s)"), *InName.ToString());

	for (auto& Property : PropertiesBeingCustomized)
	{
		if (auto Local = Property.Get())
		{
			ApplySettingsToProperty(Local, InName);
		}
	}
}

void FBlueprintComponentReferenceVarCustomization::LoadSettingsFromProperty(const FProperty* InProp)
{
	UE_LOG(BCRVariableCustomization, Log, TEXT("LoadSettingsFromProperty(%s)"), *InProp->GetFName().ToString());

#ifdef MULTI
	FBlueprintComponentReferenceMetadata Local;
	FBlueprintComponentReferenceHelper::LoadSettingsFromProperty(Local, InProp);
	// @todo: think on how to handle multiple different flags
	FBlueprintComponentReferenceMetadata& Settings = *ScopedSettings->Get();
	Settings.bUsePicker = Local.bUsePicker;
	Settings.bUseNavigate = Local.bUseNavigate;
	Settings.bUseClear = Local.bUseClear;
	Settings.bShowNative = Local.bShowNative;
	Settings.bShowBlueprint = Local.bShowBlueprint;
	Settings.bShowInstanced = Local.bShowInstanced;
	Settings.bShowPathOnly = Local.bShowPathOnly;
	Settings.AllowedClasses.Append(Local.AllowedClasses);
	Settings.DisallowedClasses.Append(Local.DisallowedClasses);
#else
	FBlueprintComponentReferenceMetadata& Settings = *ScopedSettings->Get();
	FBlueprintComponentReferenceHelper::LoadSettingsFromProperty(Settings, InProp);
#endif
}

void FBlueprintComponentReferenceVarCustomization::ApplySettingsToProperty(FProperty* Property, const FName& InChanged)
{
	UE_LOG(BCRVariableCustomization, Log, TEXT("ApplySettingsToProperty(%s)"), *Property->GetFName().ToString());

	FScopedTransaction Transaction(FText::Format(INVTEXT("ApplySettingsToProperty [{0}]"), FText::FromName(Property->GetFName())));

	FBlueprintComponentReferenceMetadata& Settings = *ScopedSettings->Get();

	FBlueprintComponentReferenceHelper::ApplySettingsToProperty(Settings, BlueprintPtr.Get(), Property, InChanged);
}
