// Fill out your copyright notice in the Description page of Project Settings.


#include "BlueprintComponentReferenceVarCustomization.h"

#include "BlueprintComponentReferenceCustomization.h"
#include "BlueprintComponentReferenceHelper.h"
#include "BlueprintComponentReferenceUtils.h"
#include "BlueprintEditorModule.h"
#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"
#include "Kismet2/BlueprintEditorUtils.h"

DEFINE_LOG_CATEGORY_STATIC(BCRVariableCustomization, Log, All);

FBlueprintComponentReferenceVarCustomization::FBlueprintComponentReferenceVarCustomization(TSharedPtr<IBlueprintEditor> InBlueprintEditor, TWeakObjectPtr<UBlueprint> InBlueprintPtr)
	: ScopedSettings(MakeShared<TStructOnScope<FBlueprintComponentReferenceExtras>>())
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
	ScopedSettings->InitializeFrom(MakeStructOnScope<FBlueprintComponentReferenceExtras>());

	PropertiesBeingCustomized.Reset();

	UBlueprint* LocalBlueprint = BlueprintPtr.Get();
	if (!IsValid(LocalBlueprint))
		return;

	TArray<TWeakObjectPtr<UObject>> ObjectsBeingCustomized;
	DetailLayout.GetObjectsBeingCustomized(ObjectsBeingCustomized);
	for (TWeakObjectPtr<UObject>& Obj : ObjectsBeingCustomized)
	{
		UPropertyWrapper* PropertyWrapper = Cast<UPropertyWrapper>(Obj.Get());
		FProperty* PropertyBeingCustomized = PropertyWrapper ? CastField<FStructProperty>(PropertyWrapper->GetProperty()) : nullptr;
		if (!PropertyBeingCustomized)
			continue;
		if (!FBlueprintEditorUtils::IsVariableCreatedByBlueprint(LocalBlueprint, PropertyBeingCustomized))
			continue;

		if (CastFieldChecked<FStructProperty>(PropertyBeingCustomized)->Struct == FBlueprintComponentReference::StaticStruct())
		{
			PropertiesBeingCustomized.Emplace(PropertyBeingCustomized);

			LoadSettingsFromProperty(LocalBlueprint, PropertyBeingCustomized);
		}
	}

	if (PropertiesBeingCustomized.Num() != 1)
	{ // todo: fix multiedit
		return;
	}

	{
		// Put custom category above `Default Value`
		int32 SortOrder = DetailLayout.EditCategory("Variable").GetSortOrder();
		DetailLayout.EditCategory("ComponentReferenceMetadata").SetSortOrder(++SortOrder);
		DetailLayout.EditCategory("DefaultValue").SetSortOrder(++SortOrder);
	}

	{
		auto& Builder = DetailLayout.EditCategory("ComponentReferenceMetadata");
		Builder.InitiallyCollapsed(false);

		for (TFieldIterator<FProperty> It(ScopedSettings->GetStruct(), EFieldIterationFlags::Default); It; ++It)
		{
			FAddPropertyParams Params;
			IDetailPropertyRow* PropertyRow = Builder.AddExternalStructureProperty(ScopedSettings, It->GetFName(), EPropertyLocation::Default, Params);
			PropertyRow->ShouldAutoExpand(true);

			TSharedPtr<IPropertyHandle> PropertyHandle = PropertyRow->GetPropertyHandle();
			PropertyHandle->SetOnPropertyValueChanged(FSimpleDelegate::CreateSP(this, &FBlueprintComponentReferenceVarCustomization::OnPropertyChanged, It->GetFName()));
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
			ApplySettingsToProperty(Local);
		}
	}

	UBlueprint* LocalBlueprint = BlueprintPtr.Get();
	if (IsValid(LocalBlueprint))
	{
		FBlueprintEditorUtils::MarkBlueprintAsModified(LocalBlueprint);
	}
}

void FBlueprintComponentReferenceVarCustomization::LoadSettingsFromProperty(UBlueprint* InBlueprint, const FProperty* InProp)
{
	UE_LOG(BCRVariableCustomization, Log, TEXT("LoadSettingsFromProperty(%s)"), *InProp->GetFName().ToString());

	FBlueprintComponentReferenceExtras& Settings = *ScopedSettings->Get();

	Settings.bShowNative = FBlueprintComponentReferenceHelper::GetBoolMetaDataValue(InProp, CRMeta::ShowNative, Settings.bShowNative);
	Settings.bShowBlueprint = FBlueprintComponentReferenceHelper::GetBoolMetaDataValue(InProp, CRMeta::ShowBlueprint, Settings.bShowBlueprint);
	Settings.bShowInstanced = FBlueprintComponentReferenceHelper::GetBoolMetaDataValue(InProp, CRMeta::ShowInstanced,  Settings.bShowInstanced);
	Settings.bShowPathOnly = FBlueprintComponentReferenceHelper::GetBoolMetaDataValue(InProp, CRMeta::ShowPathOnly,  Settings.bShowPathOnly);


	FBlueprintComponentReferenceHelper::GetClassListMetadata(InProp, CRMeta::AllowedClasses, [&](UClass* InClass)
	{
		if (!Settings.AllowedClasses.Contains(InClass))
		{
			Settings.AllowedClasses.Add(InClass);
		}
	});

	FBlueprintComponentReferenceHelper::GetClassListMetadata(InProp, CRMeta::DisallowedClasses, [&](UClass* InClass)
	{
		if (!Settings.DisallowedClasses.Contains(InClass))
		{
			Settings.DisallowedClasses.Add(InClass);
		}
	});

	//FBlueprintComponentReferenceHelper::GetClassListMetadata(InProp, CRMeta::ImplementsInterface, [&](UClass* InClass)
	//{
	//	if (!Settings.RequiredInterfaces.Contains(InClass))
	//	{
	//		Settings.RequiredInterfaces.Add(InClass);
	//	}
	//});
}

void FBlueprintComponentReferenceVarCustomization::ApplySettingsToProperty(FProperty* Property)
{
	UE_LOG(BCRVariableCustomization, Log, TEXT("ApplySettingsToProperty(%s)"), *Property->GetFName().ToString());

	FScopedTransaction Transaction(FText::Format(INVTEXT("Set MetaData [{0}]"), FText::FromName(Property->GetFName())));

	FBlueprintComponentReferenceExtras& Settings = *ScopedSettings->Get();

	auto SetMetaData = [Property, this](const FName& InName, const FString& InValue)
	{
		if (Property)
		{
			if (UBlueprint* Blueprint = BlueprintPtr.Get())
			{
				for (FBPVariableDescription& VariableDescription : Blueprint->NewVariables)
				{
					if (VariableDescription.VarName == Property->GetFName())
					{
						Blueprint->Modify();
						Property->SetMetaData(InName, FString(InValue));
						VariableDescription.SetMetaData(InName, InValue);
					}
				}
			}
		}
	};

	SetMetaData(CRMeta::ShowNative, Settings.bShowNative ? TEXT("true") : TEXT("false"));
	SetMetaData(CRMeta::ShowBlueprint, Settings.bShowBlueprint ? TEXT("true") : TEXT("false"));
	SetMetaData(CRMeta::ShowInstanced, Settings.bShowInstanced ? TEXT("true") : TEXT("false"));
	SetMetaData(CRMeta::ShowPathOnly, Settings.bShowPathOnly ? TEXT("true") : TEXT("false"));
}


#if 0

FBlueprintComponentReferenceViewCustomization::FBlueprintComponentReferenceViewCustomization(TSharedRef<IPropertyHandle> InPropertyHandle, TSharedRef<FBlueprintComponentReferenceViewSettings> InSettings)
	: TargetHandle(InPropertyHandle), TargetSettings(InSettings), ScopedSettings(MakeShared<TStructOnScope<FBlueprintComponentReferenceExtras>>())
{
	ScopedSettings->InitializeFrom(MakeStructOnScope<FBlueprintComponentReferenceExtras>());
}


void FBlueprintComponentReferenceViewCustomization::GenerateHeaderRowContent(FDetailWidgetRow& NodeRow)
{
	NodeRow.NameContent()
	[
		SNew( STextBlock )
		.Text( LOCTEXT("AdvancedViewSettings", "Advanced") )
		.Font( FAppStyle::GetFontStyle( TEXT("PropertyWindow.NormalFont") ) )
	];
}

void FBlueprintComponentReferenceViewCustomization::GenerateChildContent(IDetailChildrenBuilder& ChildrenBuilder)
{
#if 0
	// SHOW NATIVE
	{
		ChildrenBuilder.AddCustomRow( LOCTEXT("ShowNative", "Show Native") )
		.NameContent()
		[
			SNew(STextBlock)
			.Font( FAppStyle::GetFontStyle( TEXT("PropertyWindow.NormalFont") ) )
			.Text(LOCTEXT("ShowNative", "Show Native"))
		]
		.ValueContent()
		[
			SNew(SCheckBox)
			.IsChecked(this, &FBlueprintComponentReferenceViewCustomization::GetShowNative)
			.OnCheckStateChanged(this, &FBlueprintComponentReferenceViewCustomization::OnShowNativeChanged)
		];
	}
	// SHOW BLUEPRINT
	{
		ChildrenBuilder.AddCustomRow( LOCTEXT("ShowBlueprint", "Show Blueprint") )
		.NameContent()
		[
			SNew(STextBlock)
			.Font( FAppStyle::GetFontStyle( TEXT("PropertyWindow.NormalFont") ) )
			.Text(LOCTEXT("ShowBlueprint", "Show Blueprint"))
		]
		.ValueContent()
		[
			SNew(SCheckBox)
			.IsChecked(this, &FBlueprintComponentReferenceViewCustomization::GetShowBlueprint)
			.OnCheckStateChanged(this, &FBlueprintComponentReferenceViewCustomization::OnShowBlueprintChanged)
		];
	}
	// SHOW INSTANCED
	{
		ChildrenBuilder.AddCustomRow( LOCTEXT("ShowInstanced", "Show Instanced") )
		.NameContent()
		[
			SNew(STextBlock)
			.Font( FAppStyle::GetFontStyle( TEXT("PropertyWindow.NormalFont") ) )
			.Text(LOCTEXT("ShowInstanced", "Show Instanced"))
		]
		.ValueContent()
		[
			SNew(SCheckBox)
			.IsChecked(this, &FBlueprintComponentReferenceViewCustomization::GetShowInstanced)
			.OnCheckStateChanged(this, &FBlueprintComponentReferenceViewCustomization::OnShowInstancedChanged)
		];
	}
	// SHOW INSTANCED PATHONLY
	{
		ChildrenBuilder.AddCustomRow( LOCTEXT("ShowPathOnly", "Show Hidden") )
		.NameContent()
		[
			SNew(STextBlock)
			.Font( FAppStyle::GetFontStyle( TEXT("PropertyWindow.NormalFont") ) )
			.Text(LOCTEXT("ShowPathOnly", "Show Hidden"))
		]
		.ValueContent()
		[
			SNew(SCheckBox)
			.IsChecked(this, &FBlueprintComponentReferenceViewCustomization::GetShowPathOnly)
			.OnCheckStateChanged(this, &FBlueprintComponentReferenceViewCustomization::OnShowPathOnlyChanged)
		];
	}
	// BASE CLASS
	{
		ChildrenBuilder.AddCustomRow( LOCTEXT("ShowPathOnly", "Show Hidden") )
		.NameContent()
		[
			SNew(STextBlock)
			.Font( FAppStyle::GetFontStyle( TEXT("PropertyWindow.NormalFont") ) )
			.Text(LOCTEXT("ShowPathOnly", "Show Hidden"))
		]
		.ValueContent()
		[
			SNew(SComboButton)
			.OnGetMenuContent(this, &FBlueprintComponentReferenceViewCustomization::GenerateClassPicker)
			.ButtonContent()
			[
				SNew(STextBlock)
				.Text(this, &FBlueprintComponentReferenceViewCustomization::GetBaseClassText)
				.Font(FAppStyle::GetFontStyle( TEXT("PropertyWindow.NormalFont") ) )
			]
		];

	}
#endif
	{

	}
}

FName FBlueprintComponentReferenceViewCustomization::GetName() const
{
	return TEXT("ViewSettings");
}

TSharedPtr<IPropertyHandle> FBlueprintComponentReferenceViewCustomization::GetPropertyHandle() const
{
	return TargetHandle;
}

ECheckBoxState FBlueprintComponentReferenceViewCustomization::GetShowNative() const
{
	return TargetSettings->bAllowNative ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
}

void FBlueprintComponentReferenceViewCustomization::OnShowNativeChanged(ECheckBoxState Value)
{
	const bool bValue = Value == ECheckBoxState::Checked;

	TargetSettings->bAllowNative = bValue;

	CRMeta::SetValue(TargetHandle->GetMetaDataProperty(), CRMeta::ShowNative, bValue);
}

ECheckBoxState FBlueprintComponentReferenceViewCustomization::GetShowBlueprint() const
{
	return TargetSettings->bAllowBlueprint ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
}

void FBlueprintComponentReferenceViewCustomization::OnShowBlueprintChanged(ECheckBoxState Value)
{
	const bool bValue = Value == ECheckBoxState::Checked;

	TargetSettings->bAllowBlueprint = bValue;

	CRMeta::SetValue(TargetHandle->GetMetaDataProperty(), CRMeta::ShowBlueprint, bValue);
}

ECheckBoxState FBlueprintComponentReferenceViewCustomization::GetShowInstanced() const
{
	return TargetSettings->bAllowInstanced ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
}

void FBlueprintComponentReferenceViewCustomization::OnShowInstancedChanged(ECheckBoxState Value)
{
	const bool bValue = Value == ECheckBoxState::Checked;

	TargetSettings->bAllowInstanced = bValue;

	CRMeta::SetValue(TargetHandle->GetMetaDataProperty(), CRMeta::ShowInstanced, bValue);
}

ECheckBoxState FBlueprintComponentReferenceViewCustomization::GetShowPathOnly() const
{
	return TargetSettings->bAllowPathOnly ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
}

void FBlueprintComponentReferenceViewCustomization::OnShowPathOnlyChanged(ECheckBoxState Value)
{
	const bool bValue = Value == ECheckBoxState::Checked;

	TargetSettings->bAllowPathOnly = bValue;

	CRMeta::SetValue(TargetHandle->GetMetaDataProperty(), CRMeta::ShowPathOnly, bValue);
}

FText FBlueprintComponentReferenceViewCustomization::GetBaseClassText() const
{
	if (TargetSettings->AllowedComponentClassFilters.IsEmpty())
	{
		return INVTEXT("None");
	}
	else if (TargetSettings->AllowedComponentClassFilters.Num() > 1)
	{
		return INVTEXT("Multiple");
	}
	else if (TargetSettings->AllowedComponentClassFilters.Num() == 1)
	{
		return TargetSettings->AllowedComponentClassFilters[0]->GetDisplayNameText();
	}
	return FText::GetEmpty();
}

TSharedRef<SWidget> FBlueprintComponentReferenceViewCustomization::GenerateClassPicker() const
{
	FOnClassPicked OnPicked(FOnClassPicked::CreateSP(const_cast<FBlueprintComponentReferenceViewCustomization*>(this), &FBlueprintComponentReferenceViewCustomization::OnBaseClassPicked));

	//// Fill in options
	FClassViewerInitializationOptions Options;
	Options.Mode = EClassViewerMode::ClassPicker;
	Options.DisplayMode = EClassViewerDisplayMode::ListView;
	Options.InitiallySelectedClass = UActorComponent::StaticClass();

	auto ClassFilter = MakeShared<FBCRClassFilter>();
	ClassFilter->AllowedChildrenOfClasses.Add(UActorComponent::StaticClass());
	Options.ClassFilters.Add(ClassFilter);

	return SNew(SBox)
	.WidthOverride(280.0f)
	[
		SNew(SVerticalBox)
		+SVerticalBox::Slot()
		.AutoHeight()
		.MaxHeight(500.0f)
		[
			FModuleManager::LoadModuleChecked<FClassViewerModule>("ClassViewer").CreateClassViewer(Options, OnPicked)
		]
	];
}

void FBlueprintComponentReferenceViewCustomization::OnBaseClassPicked(UClass* InClass)
{
}

void FBlueprintComponentReferenceViewCustomization::ApplySettings()
{
	// apply scoped settings to metadata
}

#endif
