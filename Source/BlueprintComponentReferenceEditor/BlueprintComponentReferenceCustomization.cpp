// Copyright 2024, Aquanox.

#include "BlueprintComponentReferenceCustomization.h"

#include "BlueprintComponentReferenceEditorModule.h"
#include "BlueprintEditor.h"
#include "BlueprintEditorTabs.h"
#include "DetailWidgetRow.h"
#include "IDetailChildrenBuilder.h"
#include "IDetailPropertyRow.h"
#include "PropertyCustomizationHelpers.h"
#include "PropertyHandle.h"
#include "SlotBase.h"
#include "Brushes/SlateNoResource.h"
#include "Components/ActorComponent.h"
#include "Containers/UnrealString.h"
#include "Delegates/Delegate.h"
#include "Engine/SCS_Node.h"
#include "GameFramework/Actor.h"
#include "Internationalization/Internationalization.h"
#include "Layout/BasicLayoutWidgetSlot.h"
#include "Misc/AssertionMacros.h"
#include "Misc/Attribute.h"
#include "Styling/AppStyle.h"
#include "Styling/SlateIconFinder.h"
#include "Templates/Casts.h"
#include "Types/SlateEnums.h"
#include "UObject/Class.h"
#include "UObject/Field.h"
#include "UObject/GarbageCollection.h"
#include "UObject/NameTypes.h"
#include "UObject/Object.h"
#include "UObject/ObjectMacros.h"
#include "UObject/ObjectPtr.h"
#include "UObject/UnrealType.h"
#include "UObject/UObjectGlobals.h"
#include "UObject/UObjectIterator.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SNullWidget.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SComboButton.h"
#include "Widgets/Text/STextBlock.h"

namespace CRMeta
{
	static const FName AllowedClasses = "AllowedClasses"; // list of allowed classes or interfaces (at least one)
	static const FName DisallowedClasses = "DisallowedClasses"; // list of disallowed classes or interfaces
	static const FName ImplementsInterface = "ImplementsInterface"; // list of required interfaces to implement (all)

	static const FName NoNavigate = "NoNavigate"; // disables component selection
	static const FName NoPicker = "NoPicker"; // disables picker

	static const FName ShowBlueprint = "ShowBlueprint"; // ignore SCS components
	static const FName ShowNative = "ShowNative"; // ignore native components
	static const FName ShowInstanced = "ShowInstanced"; // ignore instanced components
	static const FName ShowPathOnly = "ShowPathOnly"; // ignore components without variables

	inline bool HasValue(TSharedRef<IPropertyHandle> const& InPropertyHandle, const FName& InName)
	{
		return InPropertyHandle->GetMetaDataProperty()->HasMetaData(InName);
	}
	inline bool GetValue(TSharedRef<IPropertyHandle> const& InPropertyHandle, const FName& InName, bool bDefaultValue)
	{
		bool bResult = 	bDefaultValue;
		if (InPropertyHandle->GetMetaDataProperty()->HasMetaData(InName))
		{
			bResult = InPropertyHandle->GetMetaDataProperty()->GetBoolMetaData(InName);
		}
		return bResult;
	}
}

#define LOCTEXT_NAMESPACE "BlueprintComponentReferenceCustomization"

TSharedRef<IPropertyTypeCustomization> FBlueprintComponentReferenceCustomization::MakeInstance()
{
	return MakeShared<FBlueprintComponentReferenceCustomization>();
}

void FBlueprintComponentReferenceCustomization::CustomizeHeader(TSharedRef<IPropertyHandle> InPropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	PropertyHandle = InPropertyHandle;
	PropertyUtilities = CustomizationUtils.GetPropertyUtilities();

	ClassHelper = FBCREditorModule::Get().GetClassHelper();
	check(ClassHelper.IsValid());

	ChooserContext.Reset();
	CachedComponentNode.Reset();
	CachedPropertyAccess = FPropertyAccess::Fail;

	// this will disable use of default "Reset To Defaults" for this header
	InPropertyHandle->MarkResetToDefaultCustomized(true);

	bAllowNavigate = true;
	bAllowPicker = true;
	bAllowClear = true;
	bAllowNative = true;
	bAllowBlueprint = true;
	bAllowInstanced = false;
	bAllowPathOnly = false;

	if (FStructProperty* const Property = CastFieldChecked<FStructProperty>(InPropertyHandle->GetProperty()))
	{
		check(FBlueprintComponentReference::StaticStruct() == Property->Struct);

		// global
		bAllowPicker = !CRMeta::HasValue(InPropertyHandle, CRMeta::NoPicker);
		// actions
		bAllowNavigate = bAllowPicker && !CRMeta::HasValue(InPropertyHandle, CRMeta::NoNavigate);
		bAllowClear = !(InPropertyHandle->GetMetaDataProperty()->PropertyFlags & CPF_NoClear);
		// filtering
		bAllowNative = CRMeta::GetValue(InPropertyHandle, CRMeta::ShowNative, bAllowNative);
		bAllowBlueprint = CRMeta::GetValue(InPropertyHandle, CRMeta::ShowBlueprint, bAllowBlueprint);
		bAllowInstanced = CRMeta::GetValue(InPropertyHandle, CRMeta::ShowInstanced, bAllowInstanced);
		bAllowPathOnly = CRMeta::GetValue(InPropertyHandle, CRMeta::ShowPathOnly, bAllowPathOnly);

		BuildClassFilters();
		BuildComboBox();

		InPropertyHandle->SetOnPropertyValueChanged(FSimpleDelegate::CreateSP(this, &FBlueprintComponentReferenceCustomization::OnPropertyValueChanged));
		OnPropertyValueChanged();

		HeaderRow.NameContent()
		[
			InPropertyHandle->CreatePropertyNameWidget()
		]
		.ValueContent()
		.HAlign(HAlign_Fill)
		[
			SNew(SHorizontalBox)
			+SHorizontalBox::Slot()
			.FillWidth(1.0f)
			[
				ComponentComboButton.ToSharedRef()
			]
			+SHorizontalBox::Slot()
			.AutoWidth()
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			.Padding(2.0f, 1.0f)
			[
				PropertyCustomizationHelpers::MakeBrowseButton(
					FSimpleDelegate::CreateSP(this, &FBlueprintComponentReferenceCustomization::OnBrowseComponent),
					LOCTEXT( "BrowseButtonToolTipText", "Select Component"),
					TAttribute<bool>(bAllowNavigate),
					true
				)
			]
			+SHorizontalBox::Slot()
			.AutoWidth()
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			.Padding(2.0f, 1.0f)
			[
				PropertyCustomizationHelpers::MakeClearButton(
					FSimpleDelegate::CreateSP(this, &FBlueprintComponentReferenceCustomization::OnClear),
					LOCTEXT("ClearButtonToolTipText", "Clear Component"),
					TAttribute<bool>(bAllowClear)
				)
			]
		]
		.IsEnabled(MakeAttributeSP(this, &FBlueprintComponentReferenceCustomization::CanEdit));
	}
}

void FBlueprintComponentReferenceCustomization::CustomizeChildren(TSharedRef<IPropertyHandle> InStructPropertyHandle, IDetailChildrenBuilder& StructBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
	uint32 NumberOfChild;
	if (InStructPropertyHandle->GetNumChildren(NumberOfChild) == FPropertyAccess::Success)
	{
		for (uint32 Index = 0; Index < NumberOfChild; ++Index)
		{
			TSharedRef<IPropertyHandle> ChildPropertyHandle = InStructPropertyHandle->GetChildHandle(Index).ToSharedRef();
			ChildPropertyHandle->MarkResetToDefaultCustomized(true);
			ChildPropertyHandle->SetOnPropertyValueChanged(FSimpleDelegate::CreateSP(this, &FBlueprintComponentReferenceCustomization::OnPropertyValueChanged));
			StructBuilder.AddProperty(ChildPropertyHandle)
				.ShowPropertyButtons(true)
				.IsEnabled(MakeAttributeSP(this, &FBlueprintComponentReferenceCustomization::CanEditChildren));
		}
	}
}

FString FBlueprintComponentReferenceCustomization::GetLoggingContextString() const
{
	return PropertyHandle.IsValid() ? FString(PropertyHandle->GetPropertyPath()) : TEXT("Invalid");
}

void FBlueprintComponentReferenceCustomization::BuildClassFilters()
{
	auto AddToClassFilters = [](const UClass* Class, TArray<const UClass*>& ComponentList)
	{
		if (Class->IsChildOf(UActorComponent::StaticClass()))
		{
			ComponentList.Add(Class);
		}
	};

	auto ParseClassFilters = [AddToClassFilters](const FString& MetaDataString, TArray<const UClass*>& ComponentList)
	{
		if (!MetaDataString.IsEmpty())
		{
			TArray<FString> ClassFilterNames;
			MetaDataString.ParseIntoArrayWS(ClassFilterNames, TEXT(","), true);

			for (const FString& ClassName : ClassFilterNames)
			{
				if (UClass* Class = FBlueprintComponentReferenceHelper::FindClassByName(ClassName))
				{
					// If the class is an interface, expand it to be all classes in memory that implement the class.
					if (Class->HasAnyClassFlags(CLASS_Interface))
					{
						for (TObjectIterator<UClass> ClassIt; ClassIt; ++ClassIt)
						{
							UClass* const ClassWithInterface = (*ClassIt);
							if (ClassWithInterface->ImplementsInterface(Class))
							{
								AddToClassFilters(ClassWithInterface, ComponentList);
							}
						}
					}
					else
					{
						AddToClassFilters(Class, ComponentList);
					}
				}
			}
		}
	};

	const FString& AllowedClassesFilterString = PropertyHandle->GetMetaData(CRMeta::AllowedClasses);
	ParseClassFilters(AllowedClassesFilterString, AllowedComponentClassFilters);

	const FString& DisallowedClassesFilterString = PropertyHandle->GetMetaData(CRMeta::DisallowedClasses);
	ParseClassFilters(DisallowedClassesFilterString, DisallowedComponentClassFilters);

	const FString& RequiredInterfaceFiltersString = PropertyHandle->GetMetaData(CRMeta::ImplementsInterface);
	ParseClassFilters(RequiredInterfaceFiltersString, RequiredInterfaceFilters);
}

void FBlueprintComponentReferenceCustomization::BuildComboBox()
{
	TSharedPtr<SVerticalBox> ObjectContent;
	SAssignNew(ObjectContent, SVerticalBox)
	+ SVerticalBox::Slot()
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Center)
		[
			SNew(SImage)
			.Image(this, &FBlueprintComponentReferenceCustomization::GetComponentIcon)
		]
		+ SHorizontalBox::Slot()
		.Padding(2, 0, 0, 0)
		.FillWidth(1)
		.VAlign(VAlign_Center)
		[
			SNew(STextBlock)
			//.TextStyle( FAppStyle::Get(), "PropertyEditor.AssetClass" )
			.Font( FAppStyle::GetFontStyle( "PropertyWindow.NormalFont" ) )
			.Text(this, &FBlueprintComponentReferenceCustomization::OnGetComponentName)
			.ToolTipText(this, &FBlueprintComponentReferenceCustomization::OnGetComponentTooltip)
		]
	];

	SAssignNew(ComponentComboButton, SComboButton)
		//.ButtonStyle(FAppStyle::Get(), "PropertyEditor.AssetComboStyle")
		//.ForegroundColor(FAppStyle::GetColor("PropertyEditor.AssetName.ColorAndOpacity"))
		.OnGetMenuContent(this, &FBlueprintComponentReferenceCustomization::OnGetMenuContent)
		.OnMenuOpenChanged(this, &FBlueprintComponentReferenceCustomization::OnMenuOpenChanged)
		.ContentPadding(FMargin(2,2,2,1))
		.Visibility(bAllowPicker ? EVisibility::Visible : EVisibility::Collapsed)
		.ButtonContent()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.HAlign(HAlign_Left)
			.VAlign(VAlign_Center)
			[
				SNew(SImage)
				.Image(this, &FBlueprintComponentReferenceCustomization::GetStatusIcon)
			]
			+ SHorizontalBox::Slot()
			.Padding(2, 0, 0, 0)
			.FillWidth(1)
			.VAlign(VAlign_Center)
			[
				ObjectContent.ToSharedRef()
			]
		];
}

void FBlueprintComponentReferenceCustomization::DetermineOuterActor()
{
	AActor* OuterActor = nullptr;
	UClass* OuterActorClass = nullptr;

	TArray<UObject*> ObjectList;
	PropertyHandle->GetOuterObjects(ObjectList);
	for (UObject* OuterObject : ObjectList)
	{
		while (::IsValid(OuterObject))
		{
			UE_LOG(LogComponentReferenceEditor, Log, TEXT("%s DetermineOuterActor: Object=%s"), *GetLoggingContextString(), *GetNameSafe(OuterActor));
			if (AActor* Actor = Cast<AActor>(OuterObject))
			{
				UE_LOG(LogComponentReferenceEditor, Log, TEXT("%s ->  Actor=%s"), *GetLoggingContextString(), *GetNameSafe(Actor));
				OuterActor = Actor;
				break;
			}
			if (UActorComponent* Component = Cast<UActorComponent>(OuterObject))
			{
				UE_LOG(LogComponentReferenceEditor, Log, TEXT("%s ->  Component=%s"), *GetLoggingContextString(), *GetNameSafe(Component));
				if (Component->GetOwner())
				{
					OuterActor = Component->GetOwner();
					break;
				}
			}
			if (UBlueprintGeneratedClass* Class = Cast<UBlueprintGeneratedClass>(OuterObject))
			{
				UE_LOG(LogComponentReferenceEditor, Log, TEXT("%s ->  Class=%s"), *GetLoggingContextString(), *GetNameSafe(Class));
				OuterActorClass = Class;
				break;
			}
			OuterObject = OuterObject->GetOuter();
		}
	}

	if (!OuterActor && OuterActorClass)
	{
		OuterActor = OuterActorClass->GetDefaultObject<AActor>();
	}

	if (OuterActor && !OuterActorClass)
	{
		OuterActorClass = OuterActor->GetClass();
	}

	UE_LOG(LogComponentReferenceEditor, Log, TEXT("%s DetermineOuterActor: Located Actor=%s BP=%s"), *GetLoggingContextString(), *GetNameSafe(OuterActor), *GetNameSafe(OuterActorClass));

	if (!ChooserContext.IsValid()
		|| ChooserContext->GetActor() != OuterActor
		|| ChooserContext->GetClass() != OuterActorClass)
	{
		ChooserContext = ClassHelper->CreateChooserContext(
			FString(PropertyHandle->GetPropertyPath()),
			OuterActor,
			OuterActorClass);
	}
}

bool FBlueprintComponentReferenceCustomization::IsComponentReferenceValid(const FBlueprintComponentReference& Value) const
{
	AActor* const SearchActor = ChooserContext.IsValid() ? ChooserContext->GetActor() : nullptr;
	if (UActorComponent* NewComponent = Value.GetComponent(SearchActor))
	{
		if (!IsFilteredObject(NewComponent))
		{
			return false;
		}

		if (NewComponent->GetOwner() == nullptr)
		{
			return false;
		}

		TArray<UObject*> ObjectList;
		PropertyHandle->GetOuterObjects(ObjectList);

		// Is the Outer object in the same world/level
		for (UObject* Obj : ObjectList)
		{
			AActor* Actor = Cast<AActor>(Obj);
			if (Actor == nullptr)
			{
				if (UActorComponent* ActorComponent = Cast<UActorComponent>(Obj))
				{
					Actor = ActorComponent->GetOwner();
				}
			}

			if (Actor)
			{
				if (NewComponent->GetOwner()->GetLevel() != Actor->GetLevel())
				{
					return false;
				}
			}
		}
	}

	return true;
}

void FBlueprintComponentReferenceCustomization::SetValue(const FBlueprintComponentReference& Value)
{
	UE_LOG(LogComponentReferenceEditor, Log, TEXT("%s SetValue %s"), *GetLoggingContextString(), *Value.GetValueString());

	ComponentComboButton->SetIsOpen(false);

	const bool bIsEmpty = Value.IsNull();
	if (bIsEmpty || IsComponentReferenceValid(Value))
	{
		FString TextValue;
		CastFieldChecked<const FStructProperty>(PropertyHandle->GetProperty())->Struct->ExportText(TextValue, &Value, &Value, nullptr, EPropertyPortFlags::PPF_None, nullptr);
		ensure(PropertyHandle->SetValueFromFormattedString(TextValue) == FPropertyAccess::Result::Success);
	}
}

FPropertyAccess::Result FBlueprintComponentReferenceCustomization::GetValue(FBlueprintComponentReference& OutValue) const
{
	// Potentially accessing the value while garbage collecting or saving the package could trigger a crash.
	// so we fail to get the value when that is occurring.
	if (GIsSavingPackage || IsGarbageCollecting())
	{
		return FPropertyAccess::Fail;
	}

	FPropertyAccess::Result Result = FPropertyAccess::Fail;
	if (PropertyHandle.IsValid() && PropertyHandle->IsValidHandle())
	{
		TArray<void*> RawData;
		PropertyHandle->AccessRawData(RawData);

		switch(RawData.Num())
		{
		case 0:
			Result = FPropertyAccess::Success;
			break;
		case 1:
			if (void* RawPtr = RawData[0])
			{
				const FBlueprintComponentReference& ThisReference = *static_cast<const FBlueprintComponentReference*>(RawPtr);
				OutValue = ThisReference;
				Result = FPropertyAccess::Success;
			}
			break;
		default:
			Result = FPropertyAccess::MultipleValues;
			break;
		}
	}
	return Result;
}

void FBlueprintComponentReferenceCustomization::OnPropertyValueChanged()
{
	UE_LOG(LogComponentReferenceEditor, Log, TEXT("%s OnPropertyValueChanged"), *GetLoggingContextString());

	CachedComponentNode.Reset();
	DetermineOuterActor();

	FBlueprintComponentReference TmpComponentReference;
	CachedPropertyAccess = GetValue(TmpComponentReference);
	if (CachedPropertyAccess == FPropertyAccess::Success)
	{
		auto Found = ChooserContext->FindComponent(TmpComponentReference);
		if (!IsComponentReferenceValid(TmpComponentReference))
		{
			CachedComponentNode.Reset();
			if (!TmpComponentReference.IsNull())
			{
				SetValue(FBlueprintComponentReference());
			}
		}
		else
		{
			CachedComponentNode = Found;
		}
	}
}

bool FBlueprintComponentReferenceCustomization::IsFilteredNode(TSharedPtr<FComponentInfo> Node) const
{
	if (!Node.IsValid())
		return false;

	if (Node->GetDesiredMode() == EBlueprintComponentReferenceMode::ObjectPath && !bAllowPathOnly)
		return false;

	if (Node->IsInstancedComponent())
	{
		if (/*Node->GetDesiredMode() == EBlueprintComponentReferenceMode::VariableName && */!bAllowInstanced)
			return false;
	}
	else
	{
		if (Node->IsNativeComponent() && !bAllowNative)
			return false;
		if (!Node->IsNativeComponent() && !bAllowBlueprint)
			return false;
	}

	return true;
}


bool FBlueprintComponentReferenceCustomization::CanEdit() const
{
	if (PropertyHandle.IsValid())
	{
		return !PropertyHandle->IsEditConst();
	}
	return bAllowPicker;
}

bool FBlueprintComponentReferenceCustomization::CanEditChildren() const
{
	if (!bAllowPicker)
	{
		return CanEdit();
	}
	return CanEdit() && !ChooserContext.IsValid();
}

const FSlateBrush* FBlueprintComponentReferenceCustomization::GetComponentIcon() const
{
	auto LocalNode = CachedComponentNode.Pin();
	if (LocalNode.IsValid())
	{
		if (UClass* LocalClass = LocalNode->GetComponentClass())
		{
			return FSlateIconFinder::FindIconBrushForClass(LocalClass);
		}
	}
	return FSlateIconFinder::FindIconBrushForClass(UActorComponent::StaticClass());
}

FText FBlueprintComponentReferenceCustomization::OnGetComponentTooltip() const
{
	if (CachedPropertyAccess == FPropertyAccess::Success)
	{
		auto LocalNode = CachedComponentNode.Pin();
		if (LocalNode.IsValid())
		{
			return LocalNode->GetTooltipText();
		}
	}
	else if (CachedPropertyAccess == FPropertyAccess::MultipleValues)
	{
		return LOCTEXT("MultipleValues", "Multiple Values");
	}
	return LOCTEXT("NoComponent", "None");
}

FText FBlueprintComponentReferenceCustomization::OnGetComponentName() const
{
	if (CachedPropertyAccess == FPropertyAccess::Success)
	{
		auto LocalNode = CachedComponentNode.Pin();
		if (LocalNode.IsValid())
		{
			return LocalNode->GetDisplayText();
		}
	}
	else if (CachedPropertyAccess == FPropertyAccess::MultipleValues)
	{
		return LOCTEXT("MultipleValues", "Multiple Values");
	}
	return LOCTEXT("NoComponent", "None");
}

const FSlateBrush* FBlueprintComponentReferenceCustomization::GetStatusIcon() const
{
	static FSlateNoResource EmptyBrush = FSlateNoResource();

	if (CachedPropertyAccess == FPropertyAccess::Fail)
	{
		return FAppStyle::GetBrush("Icons.Error");
	}
	return &EmptyBrush;
}

TSharedRef<SWidget> FBlueprintComponentReferenceCustomization::OnGetMenuContent()
{
	FMenuBuilder MenuBuilder(true, nullptr);

	//MenuBuilder.BeginSection(NAME_None, LOCTEXT("CurrentComponentOperationsHeader", "Current Reference"));
	//{
	//	if (bAllowBrowse)
	//	{
	//		MenuBuilder.AddMenuEntry(
	//			LOCTEXT("SelectComponent", "Select"),
	//			LOCTEXT("SelectComponent_Tooltip", "Select this component"),
	//			FSlateIcon(),
	//			FUIAction(FExecuteAction::CreateSP(this, &FBlueprintComponentReferenceCustomization::OnBrowseComponent))
	//		);
	//	}
	//
	//	if (bAllowClear)
	//	{
	//		MenuBuilder.AddMenuEntry(
	//			LOCTEXT("ClearComponent", "Clear"),
	//			LOCTEXT("ClearComponent_ToolTip", "Clears the component set on this field"),
	//			FSlateIcon(),
	//			FUIAction(FExecuteAction::CreateSP(this, &FBlueprintComponentReferenceCustomization::OnClear))
	//		);
	//	}
	//}
	//MenuBuilder.EndSection();

	bool bHasElements = false;

	for (const auto& HierarchyInfo : ChooserContext->ClassHierarchy)
	{
		if (HierarchyInfo->GetNodes().IsEmpty())
			continue;

		TArray<TSharedRef<FComponentInfo>> LocalArray;
		for(auto& Node : HierarchyInfo->GetNodes())
		{
			if (IsFilteredNode(Node))
			{
				LocalArray.Add(Node.ToSharedRef());
			}
		}

		if (!LocalArray.IsEmpty())
		{
			MenuBuilder.BeginSection(NAME_None, HierarchyInfo->GetDisplayText());
			for(auto& Node : LocalArray)
			{
				AddMenuNodes(MenuBuilder, Node, FString());

				bHasElements = true;
			}
			MenuBuilder.EndSection();
		}
	}

	if (!bHasElements)
	{
		MenuBuilder.BeginSection(NAME_None, LOCTEXT("CurrentComponentOperationsHeader", "Current Reference"));

		MenuBuilder.AddMenuEntry(
			LOCTEXT("SelectComponent", "No elements found"),
			LOCTEXT("SelectComponent_Tooltip", "No elements found"),
			FSlateIcon(),
			FUIAction()
		);

		MenuBuilder.EndSection();
	}

	return MenuBuilder.MakeWidget();
}

void FBlueprintComponentReferenceCustomization::AddMenuNodes(FMenuBuilder& MenuBuilder, TSharedRef<FComponentInfo> Node, FString Path)
{
	if (IsFilteredObject(Node->GetComponentTemplate()))
	{
		MenuBuilder.AddMenuEntry(
			Node->GetDisplayText(),
			FText::GetEmpty(),
			FSlateIconFinder::FindIconForClass(Node->GetComponentClass()),
			FUIAction(
				FExecuteAction::CreateSP(this, &FBlueprintComponentReferenceCustomization::OnComponentSelected, Node)
			)
		);
	}
}

void FBlueprintComponentReferenceCustomization::OnMenuOpenChanged(bool bOpen)
{
	if (!bOpen)
	{
		ComponentComboButton->SetMenuContent(SNullWidget::NullWidget);
	}
}

void FBlueprintComponentReferenceCustomization::OnClear()
{
	SetValue(FBlueprintComponentReference());
}

void FBlueprintComponentReferenceCustomization::OnBrowseComponent()
{
	auto LocalNode = CachedComponentNode.Pin();
	if (!LocalNode.IsValid() || !ChooserContext.IsValid())
	{
		return;
	}

	AActor* const SearchActor = ChooserContext.IsValid() ? ChooserContext->GetActor() : nullptr;

	// Find editor for owning blueprint
	UAssetEditorSubsystem* AssetEditorSubsystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>();

	FBlueprintEditor* BlueprintEditor = nullptr;
	UBlueprint* EditedBlueprint = nullptr;

	for(UObject* EditedAsset : AssetEditorSubsystem->GetAllEditedAssets())
	{
		if (UBlueprint* Blueprint = Cast<UBlueprint>(EditedAsset))
		{
			UBlueprintGeneratedClass* GeneratedClass = Cast<UBlueprintGeneratedClass>(Blueprint->GeneratedClass);
			if (GeneratedClass && GeneratedClass->GetFName() == SearchActor->GetClass()->GetFName())
			{
				BlueprintEditor =  (FBlueprintEditor*)AssetEditorSubsystem->FindEditorForAsset(Blueprint, false);
				EditedBlueprint = Blueprint;
				break;
			}
		}
	}

	if (BlueprintEditor && EditedBlueprint)
	{
		// Open Viewport Tab
		BlueprintEditor->FocusWindow();
		BlueprintEditor->GetTabManager()->TryInvokeTab(FBlueprintEditorTabs::SCSViewportID);

		// Select the Component in the Viewport tab view
		if (auto Template = LocalNode->GetComponentTemplate())
		{
			BlueprintEditor->FindAndSelectSubobjectEditorTreeNode(Template, false);
		}
	}
}

bool FBlueprintComponentReferenceCustomization::IsFilteredObject(const UObject* Object) const
{
	if (!IsValid(Object))
		return false;

	const UClass* ObjectClass = Object->GetClass();

	bool bAllowedToSetBasedOnFilter = true;

	if (RequiredInterfaceFilters.Num() > 0 && bAllowedToSetBasedOnFilter)
	{
		for (const UClass* AllowedClass : RequiredInterfaceFilters)
		{
			const bool bAllowedClassIsInterface = AllowedClass->HasAnyClassFlags(CLASS_Interface);
			if (!bAllowedClassIsInterface || !ObjectClass->ImplementsInterface(AllowedClass))
			{
				bAllowedToSetBasedOnFilter = false;
				break;
			}
		}
	}

	if (AllowedComponentClassFilters.Num() > 0 && bAllowedToSetBasedOnFilter)
	{
		bAllowedToSetBasedOnFilter = false;
		for (const UClass* AllowedClass : AllowedComponentClassFilters)
		{
			const bool bAllowedClassIsInterface = AllowedClass->HasAnyClassFlags(CLASS_Interface);
			if (ObjectClass->IsChildOf(AllowedClass)
				|| (bAllowedClassIsInterface && ObjectClass->ImplementsInterface(AllowedClass)))
			{
				bAllowedToSetBasedOnFilter = true;
				break;
			}
		}
	}

	if (DisallowedComponentClassFilters.Num() > 0 && bAllowedToSetBasedOnFilter)
	{
		for (const UClass* DisallowedClass : DisallowedComponentClassFilters)
		{
			const bool bDisallowedClassIsInterface = DisallowedClass->HasAnyClassFlags(CLASS_Interface);
			if (ObjectClass->IsChildOf(DisallowedClass)
				|| (bDisallowedClassIsInterface && ObjectClass->ImplementsInterface(DisallowedClass)))
			{
				bAllowedToSetBasedOnFilter = false;
				break;
			}
		}
	}

	return bAllowedToSetBasedOnFilter;
}

void FBlueprintComponentReferenceCustomization::OnComponentSelected(TSharedRef<FComponentInfo> Node)
{
	ComponentComboButton->SetIsOpen(false);

	CachedComponentNode = Node;

	FBlueprintComponentReference Result;

	if (Node->GetDesiredMode() == EBlueprintComponentReferenceMode::VariableName)
	{
		Result = FBlueprintComponentReference(EBlueprintComponentReferenceMode::VariableName, Node->GetVariableName());
	}
	else
	{
		Result = FBlueprintComponentReference(EBlueprintComponentReferenceMode::ObjectPath, Node->GetObjectName());
	}

	SetValue(Result);
}

void FBlueprintComponentReferenceCustomization::CloseComboButton()
{
	ComponentComboButton->SetIsOpen(false);
}


#undef LOCTEXT_NAMESPACE
