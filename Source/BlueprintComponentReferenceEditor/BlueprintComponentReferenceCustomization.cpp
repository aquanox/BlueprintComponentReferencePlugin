// Copyright 2024, Aquanox.

#include "BlueprintComponentReferenceCustomization.h"

#include "BlueprintComponentReferenceEditor.h"
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
#include "GameFramework/Actor.h"
#include "Internationalization/Internationalization.h"
#include "Misc/AssertionMacros.h"
#include "Misc/Attribute.h"
#include "Styling/SlateIconFinder.h"
#include "Templates/Casts.h"
#include "Types/SlateEnums.h"
#include "UObject/Class.h"
#include "UObject/Field.h"
#include "UObject/GarbageCollection.h"
#include "UObject/NameTypes.h"
#include "UObject/Object.h"
#include "UObject/ObjectMacros.h"
#include "UObject/UnrealType.h"
#include "UObject/UObjectGlobals.h"
#include "UObject/UObjectIterator.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SNullWidget.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SComboButton.h"
#include "Widgets/Text/STextBlock.h"
#include "Misc/EngineVersionComparison.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Misc/ConfigCacheIni.h"
#include "SComponentPickerTableWidget.h"
#include "SlateStyleHelper.h"
#include "HAL/IConsoleManager.h"

#define LOCTEXT_NAMESPACE "BlueprintComponentReferenceCustomization"

// feature switchers
namespace Switches
{
	// Should force reset of component references that failed to resolve into components?
	constexpr bool bResetInvalidReferences = false;
	// Should use short name for logging context label?
	constexpr bool bUseShortLoggingContextName = true;
	// Should filter unique node ids
	constexpr bool bFilterUniqueNodes = true;

	constexpr int32 DefaultViewModeDefaultValue = static_cast<int32>(EBlueprintComponentReferenceViewMode::Menu);
	static TAutoConsoleVariable<int32> DefaultViewMode(
		TEXT("BCR.DefaultViewMode"),
		DefaultViewModeDefaultValue,
		TEXT("Default view mode for BCR selector 0=Default, 1=Off, 2=Menu, 3=Table")
	);
}

bool FBlueprintComponentReferenceCustomization::IsSupportedProperty(TSharedRef<IPropertyHandle> InPropertyHandle) const
{
	const FStructProperty* Property = CastField<FStructProperty>(InPropertyHandle->GetProperty());
	if (!Property)
		return false; // reject due to property type
	if (!FBlueprintComponentReferenceHelper::IsComponentReferenceType(Property->Struct))
		return false; // reject due to not being child of BCR in general
	if (StructFilter.IsBound() && !StructFilter.Execute(Property->Struct))
		return false; // reject due to custom filter
	return true;
}

FBlueprintComponentReferenceCustomization::FBlueprintComponentReferenceCustomization(FIsSupportedStructFilter InStructFilter)
	: StructFilter(InStructFilter)
{
}

void FBlueprintComponentReferenceCustomization::CustomizeHeader(TSharedRef<IPropertyHandle> InPropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& PTCUtils)
{
	PropertyHandle = InPropertyHandle;

	FBCREditorModule::GetContextFactory();

	ComponentPickerContext.Reset();
	CachedComponentNode.Reset();
	CachedContextString = GetLoggingContextString();

	UE_LOG(LogComponentReferenceEditor, Verbose, TEXT("Created customization for %s"), *GetLoggingContextString());

	// this will disable use of default "Reset To Defaults" for this header
	InPropertyHandle->MarkResetToDefaultCustomized(true);

	PropertyStruct.Reset();
	ViewSettings.ResetSettings();

	if (IsSupportedProperty(InPropertyHandle))
	{
		FStructProperty* const Property = CastFieldChecked<FStructProperty>(InPropertyHandle->GetProperty());
		PropertyStruct = Property->Struct;
		TempPropertyStorage.InitializeFrom(FStructOnScope(Property->Struct));

		// Stage 1: import settings from BCR struct
		ViewSettings.LoadSettingsFromType(Property->Struct);
		// Stage 2: import settings from member property
		ViewSettings.LoadSettingsFromProperty(InPropertyHandle->GetMetaDataProperty());

		InPropertyHandle->SetOnPropertyValueChanged(FSimpleDelegate::CreateSP(this, &ThisClass::OnPropertyValueChanged, Property->GetFName()));
		OnPropertyValueChanged(Property->GetFName());

		CustomizeHeaderImpl(InPropertyHandle, HeaderRow, PTCUtils);
	}
}

void FBlueprintComponentReferenceCustomization::CustomizeHeaderImpl(TSharedRef<IPropertyHandle> InPropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& PTCUtils)
{
	ComponentComboButton = BuildComboBox();

	TSharedPtr<SHorizontalBox> ValueContent;
	SAssignNew(ValueContent, SHorizontalBox)
	+SHorizontalBox::Slot()
	.FillWidth(1.0f)
	[
#if WITH_BCR_DRAG_DROP
		SNew(SDropTarget)
			.OnAllowDrop(this, &ThisClass::OnVerifyDrag)
			.OnIsRecognized(this, &ThisClass::OnVerifyDrag)
			.OnDropped(this, &ThisClass::OnDropped)
		[
			ComponentComboButton.ToSharedRef()
		]
#else
		ComponentComboButton.ToSharedRef()
#endif
	];

#if !UE_VERSION_OLDER_THAN(5, 0, 0)
	if (!ViewSettings.bDisableNavigate)
	{
		ValueContent->AddSlot()
		.AutoWidth()
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		.Padding(2.0f, 1.0f)
		[
			PropertyCustomizationHelpers::MakeBrowseButton(
				FSimpleDelegate::CreateSP(this, &ThisClass::OnNavigateComponent),
				LOCTEXT( "NavigateButtonToolTipText", "Select Component in Component Editor"),
				/* enabled = */ true, /* actor icon = */ true
			)
		];
	}
#endif

	if (!ViewSettings.bDisableClear)
	{
		ValueContent->AddSlot()
		.AutoWidth()
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		.Padding(2.0f, 1.0f)
		[
			PropertyCustomizationHelpers::MakeClearButton(
				FSimpleDelegate::CreateSP(this, &ThisClass::OnClear),
				LOCTEXT("ClearButtonToolTipText", "Clear Component"),
				/* enabled = */ true
			)
		];
	}

	HeaderRow.NameContent()
	[
		InPropertyHandle->CreatePropertyNameWidget()
	]
	.ValueContent()
	.MinDesiredWidth(TOptional<float>())
	.MaxDesiredWidth(TOptional<float>())
	.HAlign(HAlign_Fill)
	[
		ValueContent.ToSharedRef()
	]
	.IsEnabled(MakeAttributeSP(this, &ThisClass::CanEdit));
}

void FBlueprintComponentReferenceCustomization::CustomizeChildren(TSharedRef<IPropertyHandle> InStructPropertyHandle, IDetailChildrenBuilder& StructBuilder, IPropertyTypeCustomizationUtils& PTCUtils)
{
	uint32 NumberOfChild = 0;
	if (InStructPropertyHandle->GetNumChildren(NumberOfChild) != FPropertyAccess::Success)
		return;

	for (uint32 Index = 0; Index < NumberOfChild; ++Index)
	{
		TSharedRef<IPropertyHandle> ChildPropertyHandle = InStructPropertyHandle->GetChildHandle(Index).ToSharedRef();
		//if (IsVisibleProperty(ChildPropertyHandle))
		{
			CustomizeChildImpl(InStructPropertyHandle, ChildPropertyHandle, StructBuilder, PTCUtils);
		}
	}
}

void FBlueprintComponentReferenceCustomization::CustomizeChildImpl(TSharedRef<IPropertyHandle> InPropertyHandle, TSharedRef<IPropertyHandle> InChildPropertyHandle, IDetailChildrenBuilder& StructBuilder, IPropertyTypeCustomizationUtils& PTCUtils)
{
	InChildPropertyHandle->MarkResetToDefaultCustomized(true);

	InChildPropertyHandle->SetOnPropertyValueChanged(
		FSimpleDelegate::CreateSP(this, &ThisClass::OnPropertyValueChanged,
			InChildPropertyHandle->GetProperty()->GetFName()
		)
	);

	StructBuilder.AddProperty(InChildPropertyHandle)
		.ShowPropertyButtons(!ViewSettings.UsePicker())
		.ShouldAutoExpand(!ViewSettings.UsePicker())
		.IsEnabled(MakeAttributeSP(this, &ThisClass::CanEditChildren));
}

FString FBlueprintComponentReferenceCustomization::GetLoggingContextString() const
{
	if (!CachedContextString.IsEmpty())
	{
		return CachedContextString;
	}

	TStringBuilder<128> Buffer;
	if (PropertyHandle.IsValid())
	{
		if (!Switches::bUseShortLoggingContextName)
		{
			TArray<UObject*> PropertyOuterObjects;
			PropertyHandle->GetOuterObjects(PropertyOuterObjects);
			for(const UObject* OuterObject : PropertyOuterObjects)
			{
				Buffer.Append(OuterObject->GetPathName());
				Buffer.Append(TEXT("."));
			}
		}
		Buffer.Append(PropertyHandle->GeneratePathToProperty());
	}
	else
	{
		Buffer.Append(TEXT("Invalid"));
	}

	return Buffer.ToString();
}

TSharedRef<SComboButton> FBlueprintComponentReferenceCustomization::BuildComboBox()
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
			.Image(this, &ThisClass::GetComponentIcon)
		]
		+ SHorizontalBox::Slot()
		.Padding(2, 0, 0, 0)
		.FillWidth(1)
		.VAlign(VAlign_Center)
		[
			SNew(STextBlock)
#if UE_VERSION_OLDER_THAN(5, 0, 0)
			.TextStyle( FSlateStyleHelper::Get(), "PropertyEditor.AssetClass" )
#endif
			.Font( FSlateStyleHelper::GetFontStyle( "PropertyWindow.NormalFont" ) )
			.Text(this, &ThisClass::OnGetComponentName)
			.ColorAndOpacity(this, &ThisClass::OnGetComponentNameColor)
			.ToolTipText(this, &ThisClass::OnGetComponentTooltip)
		]
	];

	return SNew(SComboButton)
#if UE_VERSION_OLDER_THAN(5, 0, 0)
		.ButtonStyle( FSlateStyleHelper::Get(), "PropertyEditor.AssetComboStyle" )
		.ForegroundColor(FSlateStyleHelper::GetColor("PropertyEditor.AssetName.ColorAndOpacity"))
#endif
		.OnGetMenuContent(this, &ThisClass::OnGetMenuContent)
		.OnMenuOpenChanged(this, &ThisClass::OnMenuOpenChanged)
		.ContentPadding(FMargin(0,2,0,2))
		.Visibility(ViewSettings.UsePicker() ? EVisibility::Visible : EVisibility::Collapsed)
		.ButtonContent()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.HAlign(HAlign_Left)
			.VAlign(VAlign_Center)
			[
				SNew(SImage)
				.Image(this, &ThisClass::GetStatusIcon)
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

void FBlueprintComponentReferenceCustomization::DetermineContext()
{
	AActor* OuterActor = nullptr;
	UClass* OuterActorClass = nullptr;

	// DetermineContext_FromMetadata
	// Handle explicit external class metadata setting
	if (!OuterActor && !OuterActorClass && !ViewSettings.ActorClass.IsNull())
	{
		if (ViewSettings.ActorClass.IsValid())
		{
			UE_LOG(LogComponentReferenceEditor, Verbose, TEXT("%s GuessMetadata=%s"), *GetLoggingContextString(), *ViewSettings.ActorClass.ToString());

			OuterActorClass = ViewSettings.ActorClass.Get();
		}
		else
		{
			UE_LOG(LogComponentReferenceEditor, Verbose, TEXT("%s GuessMetadata=%s (loading)"), *GetLoggingContextString(), *ViewSettings.ActorClass.ToString());

			OuterActorClass = ViewSettings.ActorClass.LoadSynchronous();
		}
	}

	// DetermineContext_FromClassProperty
	// allow override explicitly set value based on context used
	TArray<UObject*> ObjectList;
	PropertyHandle->GetOuterObjects(ObjectList);

	// Handle common cases:
	// - blueprint of Actor
	// - instance of Actor
	// - instance of ActorComponent
	for (UObject* OuterObject : ObjectList)
	{
		while (IsValid(OuterObject))
		{
			UE_LOG(LogComponentReferenceEditor, Verbose, TEXT("%s GuessObject=%s"), *GetLoggingContextString(), *GetNameSafe(OuterActor));
			if (AActor* Actor = Cast<AActor>(OuterObject))
			{
				UE_LOG(LogComponentReferenceEditor, Verbose, TEXT("%s GuessObject=%s"), *GetLoggingContextString(), *GetNameSafe(Actor));
				OuterActor = Actor;
				break;
			}
			if (UActorComponent* Component = Cast<UActorComponent>(OuterObject))
			{
				UE_LOG(LogComponentReferenceEditor, Verbose, TEXT("%s GuessComponent=%s"), *GetLoggingContextString(), *GetNameSafe(Component));
				if (Component->GetOwner())
				{
					OuterActor = Component->GetOwner();
					break;
				}
			}
			// only support regular blueprints (not anim or others)
			if (UBlueprintGeneratedClass* Class = ExactCast<UBlueprintGeneratedClass>(OuterObject))
			{
				UE_LOG(LogComponentReferenceEditor, Verbose, TEXT("%s GuessClass=%s"), *GetLoggingContextString(), *GetNameSafe(Class));
				OuterActorClass = Class;
				break;
			}
			OuterObject = OuterObject->GetOuter();
		}
	}

	// DetermineContext_FromFunctionProperty
	// handle case when BCR is a local variable in a function declared in blueprint of AActor
	if (!OuterActor && !OuterActorClass && !ObjectList.Num())
	{
		FProperty* Property = PropertyHandle->GetProperty();
		UFunction* OwnerFunction = Property && !Property->IsNative() ? Property->GetOwner<UFunction>() : nullptr;
		if (OwnerFunction && OwnerFunction->GetOwnerClass())
		{
			UClass* const OwnerClass = OwnerFunction->GetOwnerClass();
			if (ExactCast<UBlueprint>(OwnerClass->ClassGeneratedBy) && OwnerClass->IsChildOf(AActor::StaticClass()))
			{
				UE_LOG(LogComponentReferenceEditor, Verbose, TEXT("%s ->  FunctionClass=%s"), *GetLoggingContextString(), *GetNameSafe(OwnerClass));

				OuterActorClass = OwnerClass;
			}
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

	UE_LOG(LogComponentReferenceEditor, Verbose, TEXT("%s DetermineOuterActor: Located Actor=%s BP=%s"), *GetLoggingContextString(), *GetNameSafe(OuterActor), *GetNameSafe(OuterActorClass));

	if (!ComponentPickerContext.IsValid()
		|| ComponentPickerContext->GetActor() != OuterActor
		|| ComponentPickerContext->GetClass() != OuterActorClass)
	{
		ComponentPickerContext = FBlueprintComponentReferenceHelper::CreateChooserContext(OuterActor, OuterActorClass, GetLoggingContextString());
	}

	if (!ComponentPickerContext.IsValid())
	{
		UE_LOG(LogComponentReferenceEditor, Warning, TEXT("Failed to determine chooser context for %s"), *GetLoggingContextString());
	}
}

bool FBlueprintComponentReferenceCustomization::IsComponentReferenceValid(const FBlueprintComponentReference& Value) const
{
	AActor* const SearchActor = ComponentPickerContext.IsValid() ? ComponentPickerContext->GetActor() : nullptr;
	if (UActorComponent* NewComponent = Value.GetComponent(SearchActor))
	{
		if (!TestObject(NewComponent))
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
	UE_LOG(LogComponentReferenceEditor, Verbose, TEXT("%s SetValue %s"), *GetLoggingContextString(), *Value.ToString());

	ComponentComboButton->SetIsOpen(false);

	const bool bIsEmpty = Value.IsNull();
	if (bIsEmpty || IsComponentReferenceValid(Value))
	{
		FString TextValue;
		PropertyStruct->ExportText(TextValue, &Value, &Value, nullptr, EPropertyPortFlags::PPF_None, nullptr);
		ensureAlways(PropertyHandle->SetValueFromFormattedString(TextValue) == FPropertyAccess::Result::Success);
	}
}

void FBlueprintComponentReferenceCustomization::SetDefaultValue()
{
	UE_LOG(LogComponentReferenceEditor, Verbose, TEXT("%s SetDefaultValue"), *GetLoggingContextString());

	check(TempPropertyStorage.IsValid());
	PropertyStruct->ClearScriptStruct(TempPropertyStorage.Get());

	FString TextValue;
	PropertyStruct->ExportText(TextValue, TempPropertyStorage.Get(), TempPropertyStorage.Get(), nullptr, EPropertyPortFlags::PPF_None, nullptr);
	ensureAlways(PropertyHandle->SetValueFromFormattedString(TextValue) == FPropertyAccess::Result::Success);
}

FBlueprintComponentReferenceCustomization::FValueAndError FBlueprintComponentReferenceCustomization::GetValue() const
{
#if UE_VERSION_OLDER_THAN(5, 8, 0)
	bool bIsSavingPackage = GIsSavingPackage;
#else
	bool bIsSavingPackage = UE::IsSavingPackage();
#endif

	check(TempPropertyStorage.IsValid());

	// Potentially accessing the value while garbage collecting or saving the package could trigger a crash.
	// so we fail to get the value when that is occurring.
	if (bIsSavingPackage || IsGarbageCollecting())
	{
		return FValueAndError(TempPropertyStorage.Get(), FPropertyAccess::Fail);
	}

	if (PropertyHandle.IsValid() && PropertyHandle->IsValidHandle())
	{
		TArray<void*> RawData;
		PropertyHandle->AccessRawData(RawData);

		if (RawData.Num() == 1)
		{
			return FValueAndError(static_cast<const FBlueprintComponentReference*>(RawData[0]), FPropertyAccess::Success);

			//PropertyStruct->CopyScriptStruct(TempPropertyStorage.Get(), RawData[0]);
			//return FValueAndError(TempPropertyStorage.Get(), FPropertyAccess::Success);
		}
		else if (RawData.Num() == 0)
		{
			return FValueAndError(TempPropertyStorage.Get(), FPropertyAccess::Fail);
		}
		else
		{
			return FValueAndError(TempPropertyStorage.Get(), FPropertyAccess::MultipleValues);
		}
	}

	return FValueAndError(TempPropertyStorage.Get(), FPropertyAccess::Fail);
}

void FBlueprintComponentReferenceCustomization::OnPropertyValueChanged(FName Source)
{
	UE_LOG(LogComponentReferenceEditor, Verbose, TEXT("%s OnPropertyValueChanged (Source=%s)"), *GetLoggingContextString(), *Source.ToString());

	CachedComponentNode.Reset();
	PropertyState = EPropertyState::Normal;

	if (!ComponentPickerContext.IsValid())
	{
		DetermineContext();
	}

	const FValueAndError Result = GetValue();
	if (Result.Value == FPropertyAccess::Success)
	{
		const FBlueprintComponentReference& TmpComponentReference = *Result.Key;
		// search for component node information within context
		if (ComponentPickerContext.IsValid() && !TmpComponentReference.IsNull())
		{
			TSharedPtr<FComponentInfo> Found = ComponentPickerContext->FindComponent(TmpComponentReference, /*  bSafeSearch = */ true);
			if (Found.IsValid())
			{
				if (Found->IsUnknown())
				{
					PropertyState = EPropertyState::BadInfo;
				}
				else if (!TestNode(Found))
				{
					PropertyState = EPropertyState::BadReference;
				}

			}
			CachedComponentNode = Found;
		}

		// attempt to resolve & validate component reference within current context
		if (!IsComponentReferenceValid(TmpComponentReference))
		{
			PropertyState = EPropertyState::BadReference;
		}
	}
	else
	{
		PropertyState = EPropertyState::BadPropertyAccess;
	}

	if (PropertyState != EPropertyState::Normal)
	{
		if (Switches::bResetInvalidReferences)
		{
			UE_LOG(LogComponentReferenceEditor, Warning, TEXT("%s Invalid reference. Resetting to none."), *GetLoggingContextString());
			SetDefaultValue();
		}
		else
		{
			const FBlueprintComponentReference& TmpComponentReference = *Result.Key;
			UE_LOG(LogComponentReferenceEditor, Warning, TEXT("%s has invalid reference (%s)"), *GetLoggingContextString(), *TmpComponentReference.ToString());
		}
	}
}

bool FBlueprintComponentReferenceCustomization::CanEdit() const
{
	if (PropertyHandle.IsValid())
	{
		return !PropertyHandle->IsEditConst();
	}
	return ViewSettings.UsePicker();
}

bool FBlueprintComponentReferenceCustomization::CanEditChildren() const
{
	if (!ViewSettings.UsePicker())
	{
		return CanEdit();
	}
	return CanEdit() && !ComponentPickerContext.IsValid();
}

const FSlateBrush* FBlueprintComponentReferenceCustomization::GetComponentIcon() const
{
	TSharedPtr<FComponentInfo> LocalNode = CachedComponentNode.Pin();
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
	if (PropertyState == EPropertyState::BadPropertyAccess)
	{
		return LOCTEXT("MultipleValues", "Multiple Values");
	}
	else if (PropertyState == EPropertyState::BadInfo)
	{
		return LOCTEXT("UnknownComponentReference", "Failed to locate target component");
	}
	else if (PropertyState == EPropertyState::BadReference)
	{
		return LOCTEXT("BadComponentReference", "Target component does not match filters specified for this property");
	}

	TSharedPtr<FComponentInfo> LocalNode = CachedComponentNode.Pin();
	if (LocalNode.IsValid())
	{
		return LocalNode->GetTooltipText();
	}
	return LOCTEXT("NoComponent", "None");
}

FText FBlueprintComponentReferenceCustomization::OnGetComponentName() const
{
	if (PropertyState == EPropertyState::BadPropertyAccess)
	{
		return LOCTEXT("MultipleValues", "Multiple Values");
	}

	TSharedPtr<FComponentInfo> LocalNode = CachedComponentNode.Pin();
	if (LocalNode.IsValid())
	{
		return LocalNode->GetDisplayText();
	}
	return LOCTEXT("NoComponent", "None");
}

FSlateColor FBlueprintComponentReferenceCustomization::OnGetComponentNameColor() const
{
	if (PropertyState != EPropertyState::Normal)
	{
#if UE_VERSION_OLDER_THAN(5, 0, 0)
		return FLinearColor(FColor(0xffbbbb22));
#else
		return FLinearColor::Yellow;
#endif
	}
	return CanEdit() ? FSlateColor::UseForeground() : FSlateColor::UseSubduedForeground();
}

const FSlateBrush* FBlueprintComponentReferenceCustomization::GetStatusIcon() const
{
	static FSlateNoResource EmptyBrush = FSlateNoResource();

	if (PropertyState != EPropertyState::Normal)
	{
		return FSlateStyleHelper::GetBrush("Icons.Error");
	}
	return &EmptyBrush;
}

TSharedRef<SWidget> FBlueprintComponentReferenceCustomization::OnGetMenuContent()
{
	UpdateSelectionList();

	EBlueprintComponentReferenceViewMode ViewMode = ViewSettings.ComponentViewMode;
	if (ViewMode == EBlueprintComponentReferenceViewMode::Default)
	{
		FString Value;
		if (!GConfig->GetString(TEXT("BlueprintComponentReference"), TEXT("DefaultViewMode"), Value, GEditorIni) || Value.IsEmpty())
		{
			Value = StaticEnum<EBlueprintComponentReferenceViewMode>()->GetNameStringByValue(Switches::DefaultViewMode.GetValueOnAnyThread());
		}

		int64 EnumValue = StaticEnum<EBlueprintComponentReferenceViewMode>()->GetValueByNameString(Value);
		if (EnumValue == INDEX_NONE)
		{
			EnumValue = Switches::DefaultViewModeDefaultValue;
		}
		ViewMode = static_cast<EBlueprintComponentReferenceViewMode>(EnumValue);
	}

	switch (ViewMode)
	{
	default:
	case EBlueprintComponentReferenceViewMode::Menu:
		return BuildComponentSelectionMenu();
	case EBlueprintComponentReferenceViewMode::Table:
		return BuildComponentSelectionTable();
	case EBlueprintComponentReferenceViewMode::Off:
		return SNullWidget::NullWidget;
	}
}

void FBlueprintComponentReferenceCustomization::UpdateSelectionList()
{
	if (!ComponentPickerContext.IsValid())
	{ // this is necessary after updating metadata or a new property
		DetermineContext();
	}

	if (ComponentPickerContext.IsValid())
	{
		TArray<FComponentPickerGroup> ChoosableElements;

		// collect unique picker contents, with lowest level one being most important
		// ClassHistory order is  Instance Class ParentClass GrantParentClass so iterating in reverse
		// first Node occurrence is preferred

		TArray<TSharedPtr<FHierarchyInfo>> DataSource = ComponentPickerContext->ClassHierarchy;
		if (Switches::bFilterUniqueNodes)
		{
			Algo::Reverse(DataSource);
		}

		TArray<FName> KnownNames;

		for (const TSharedPtr<FHierarchyInfo>& HierarchyInfo : DataSource)
		{
			if (!HierarchyInfo->GetNodes().Num())
				continue;
			// do not show 'Instanced' category when no instanced choises needed, even if we browsing actor instance
			if ((!ViewSettings.bShowInstanced && !ViewSettings.bShowHidden) && HierarchyInfo->IsInstance())
				continue;

			FComponentPickerGroup Data;
			Data.Category = HierarchyInfo;

			for (const TSharedPtr<FComponentInfo>& Node : HierarchyInfo->GetNodes())
			{
				FName NodeId = Node->GetNodeID();
				if (TestNode(Node) && TestObject(Node->GetComponentTemplate()))
				{
					if (!Switches::bFilterUniqueNodes || (NodeId.IsNone() || KnownNames.Find(NodeId) == INDEX_NONE))
					{
						Data.Elements.Add(Node);
						KnownNames.Add(NodeId);
					}
				}
			}

			if (Data.Elements.Num())
			{
				ChoosableElements.Emplace(MoveTemp(Data));
			}
		}

		// Restore order of found selection items to display
		if (Switches::bFilterUniqueNodes)
		{
			Algo::Reverse(ChoosableElements);
		}

		// Add root component magic item
		if (ViewSettings.bShowRoot)
		{
			TSharedPtr<FComponentInfo> RootInfo = ComponentPickerContext->GetRoot();
			// if (TestNode(RootInfo) && TestObject(RootInfo->GetComponentTemplate()))
			{
				if (ChoosableElements.Num())
				{
					ChoosableElements.Last().Elements.Add(RootInfo);
				}
				else
				{
					FComponentPickerGroup MenuSection;
					MenuSection.Category = DataSource.Last();
					MenuSection.Elements.Add(RootInfo);
					ChoosableElements.Add(MenuSection);
				}
			}
		}

		CachedChoosableElements = MoveTemp(ChoosableElements);
	}
}

TSharedRef<SWidget> FBlueprintComponentReferenceCustomization::BuildComponentSelectionMenu()
{
	FMenuBuilder MenuBuilder(true, nullptr);

	if (CachedChoosableElements.Num())
	{
		for (const FComponentPickerGroup& Element : CachedChoosableElements)
		{
			MenuBuilder.BeginSection(NAME_None, Element.Category->GetDisplayText());

			for(const TSharedPtr<FComponentInfo>& Node : Element.Elements)
			{
				MenuBuilder.AddMenuEntry(
					Node->GetDisplayText(),
					Node->GetTooltipText(),
					FSlateIconFinder::FindIconForClass(Node->GetComponentClass()),
					FUIAction(
						FExecuteAction::CreateSP(this, &FBlueprintComponentReferenceCustomization::OnComponentSelected, Node)
					)
				);
			}

			MenuBuilder.EndSection();
		}
	}
	else
	{
		MenuBuilder.BeginSection(NAME_None, LOCTEXT("ComponentsSectionHeader", "Components"));
		MenuBuilder.AddMenuEntry(
			LOCTEXT("NotFoundComponent", "No elements found"),
			LOCTEXT("NotFoundComponent_Tooltip", "No elements found"),
			FSlateIcon(),
			FUIAction(), NAME_None, EUserInterfaceActionType::None
		);
		MenuBuilder.EndSection();
	}

	return MenuBuilder.MakeWidget();
}

TSharedRef<SWidget> FBlueprintComponentReferenceCustomization::BuildComponentSelectionTable()
{
	return SNew(SComponentPickerTableWidget)
		.Items(CachedChoosableElements)
		.Context(ComponentPickerContext)
		.OnClear(this, &FBlueprintComponentReferenceCustomization::OnClear)
		.OnSelected(this, &FBlueprintComponentReferenceCustomization::OnComponentSelected);
}

void FBlueprintComponentReferenceCustomization::OnMenuOpenChanged(bool bOpen)
{
	if (!bOpen)
	{
		ComponentComboButton->SetMenuContent(SNullWidget::NullWidget);

		CachedChoosableElements.Reset();
	}
}

void FBlueprintComponentReferenceCustomization::OnClear()
{
	SetDefaultValue();
}

void FBlueprintComponentReferenceCustomization::OnNavigateComponent()
{
	if (ComponentPickerContext.IsValid() && CachedComponentNode.IsValid())
	{
		AActor* const SearchActor = ComponentPickerContext.IsValid() ? ComponentPickerContext->GetActor() : nullptr;
		TSharedPtr<FComponentInfo> LocalNode = CachedComponentNode.Pin();

		if (::IsValid(SearchActor) && LocalNode.IsValid())
		{
			FBlueprintComponentReferenceHelper::TryNavigateToComponent(SearchActor, LocalNode);
		}
	}
}

void FBlueprintComponentReferenceCustomization::OnComponentSelected(TSharedPtr<FComponentInfo> Node)
{
	ComponentComboButton->SetIsOpen(false);
	CachedComponentNode = Node;

	// Prepare a clean struct in temp storage
	// write new component reference while resetting all possible custom properties
	// and assign to property

	check(TempPropertyStorage.IsValid());
	PropertyStruct->ClearScriptStruct(TempPropertyStorage.Get());

	FBlueprintComponentReference& Result = *TempPropertyStorage.Get();
	if (Node->GetDesiredMode() == EBlueprintComponentReferenceMode::Property)
	{
		FBlueprintComponentReferenceHelper::SetMode_Private(Result, EBlueprintComponentReferenceMode::Property);
		FBlueprintComponentReferenceHelper::SetValue_Private(Result, Node->GetVariableName());
	}
	else
	{
		FBlueprintComponentReferenceHelper::SetMode_Private(Result, EBlueprintComponentReferenceMode::Path);
		FBlueprintComponentReferenceHelper::SetValue_Private(Result, Node->GetObjectName());
	}

	OnComponentSelected(Result);
}

void FBlueprintComponentReferenceCustomization::OnComponentSelected(FBlueprintComponentReference& NewValue)
{
	SetValue(NewValue);
}

void FBlueprintComponentReferenceCustomization::CloseComboButton()
{
	ComponentComboButton->SetIsOpen(false);
}

void FBlueprintComponentReferenceCustomization::ResetViewSettings()
{
	ViewSettings.ResetSettings();
}

bool FBlueprintComponentReferenceCustomization::TestNode(const TSharedPtr<FComponentInfo>& Node) const
{
	if (!Node.IsValid())
		return false;

	if (!ViewSettings.bShowEditor && Node->IsEditorOnlyComponent())
		return false;

	const EBlueprintComponentReferenceMode Mode = Node->GetDesiredMode();
	if (Mode == EBlueprintComponentReferenceMode::Path)
	{
		return ViewSettings.bShowHidden;
	}
	if (Mode == EBlueprintComponentReferenceMode::Property)
	{
		if (Node->IsInstancedComponent() && ViewSettings.bShowInstanced)
			return true;

		if (Node->IsNativeComponent() && ViewSettings.bShowNative)
			return true;

		if (Node->IsBlueprintComponent() && ViewSettings.bShowBlueprint)
			return true;
	}

	return false;
}

bool FBlueprintComponentReferenceCustomization::TestObject(const UObject* Object) const
{
	if (!IsValid(Object))
	{
		return false;
	}

	const UClass* ObjectClass = Object->GetClass();

	bool bAllowedToSetBasedOnFilter = true;

	if (ViewSettings.AllowedClasses.Num() > 0 && bAllowedToSetBasedOnFilter)
	{
		bAllowedToSetBasedOnFilter = false;
		for (auto& AllowedClass : ViewSettings.AllowedClasses)
		{
			if (UClass* Class = AllowedClass.Get())
			{
				const bool bAllowedClassIsInterface = AllowedClass->HasAnyClassFlags(CLASS_Interface);
				if (ObjectClass->IsChildOf(Class)
					|| (bAllowedClassIsInterface && ObjectClass->ImplementsInterface(Class)))
				{
					bAllowedToSetBasedOnFilter = true;
					break;
				}
			}
		}
	}

	if (ViewSettings.DisallowedClasses.Num() > 0 && bAllowedToSetBasedOnFilter)
	{
		for (auto& DisallowedClass : ViewSettings.DisallowedClasses)
		{
			if (UClass* Class = DisallowedClass.Get())
			{
				const bool bDisallowedClassIsInterface = DisallowedClass->HasAnyClassFlags(CLASS_Interface);
				if (ObjectClass->IsChildOf(Class)
					|| (bDisallowedClassIsInterface && ObjectClass->ImplementsInterface(Class)))
				{
					bAllowedToSetBasedOnFilter = false;
					break;
				}
			}
		}
	}

	if (!ViewSettings.ComponentFilter.IsEmpty() && bAllowedToSetBasedOnFilter)
	{
		if (!FBlueprintComponentReferenceHelper::InvokeComponentFilter(PropertyHandle, ViewSettings.ComponentFilter, Object))
		{
			bAllowedToSetBasedOnFilter = false;
		}
	}

	return bAllowedToSetBasedOnFilter;
}

#if WITH_BCR_DRAG_DROP

/**
 * @see FBlueprintEditor::OnSelectionUpdated
 * @see SKismetInspector::ShowDetailsForObjects
 *
 * IMPORTANT! Proper use of drag&drop requires engine patch because clicking on property or component in SSCSEditor for
 * drag start will cause selection update, which make Details Panel switch to component or variable that is being dragged
 * workaround was suppressing Kismet Inspector object change if Alt key was held at a time of update (as Ctrl is in use by Blueprint Editor and Shift for multiselect)
 *
 */
bool FBlueprintComponentReferenceCustomization::OnVerifyDrag(TSharedPtr<FDragDropOperation> InDragDrop)
{
	if (!InDragDrop->IsOfType<FKismetVariableDragDropAction>())
	{
		return false;
	}

	struct FKismetVariableDragDropAction_Accessor : public FKismetVariableDragDropAction
	{
		friend FBlueprintComponentReferenceCustomization;
	};

	auto VarAction = StaticCastSharedPtr<FKismetVariableDragDropAction_Accessor>(InDragDrop);
	UBlueprint* const Blueprint = VarAction->GetSourceBlueprint();

	if (!ComponentPickerContext.IsValid() || !Blueprint)
	{ // bad context
		return false;
	}

	FObjectProperty* const TestProperty = CastField<FObjectProperty>(VarAction->GetVariableProperty());
	if (!TestProperty || !TestProperty->PropertyClass || !TestProperty->PropertyClass->IsChildOf(UActorComponent::StaticClass()))
	{ // bad property
		return false;
	}

	auto Node = ComponentPickerContext->FindComponentForVariable(TestProperty->GetFName());
	if (!TestNode(Node))
	{ // does not register within picker or not eligible
		return false;
	}

	UActorComponent* TestTarget = Node->GetComponentTemplate();
	if (!TestTarget)
	{
		TestTarget =  TestProperty->PropertyClass->GetDefaultObject<UActorComponent>();
	}

	if (!TestObject(TestTarget))
	{ // does not match class restrictions
		return false;
	}

	return true;
}

FReply FBlueprintComponentReferenceCustomization::OnDropped(const FGeometry& InGeometry, const class FDragDropEvent& InDragDropEvent)
{
	TSharedPtr<FKismetVariableDragDropAction> DragOp = InDragDropEvent.GetOperationAs<FKismetVariableDragDropAction>();
	if (DragOp)
	{
		return OnDrop(InDragDropEvent.GetOperation());
	}
	return FReply::Handled();
}

FReply FBlueprintComponentReferenceCustomization::OnDrop(TSharedPtr<FDragDropOperation> InDragDrop)
{
	TSharedPtr<FKismetVariableDragDropAction> Ptr = StaticCastSharedPtr<FKismetVariableDragDropAction>(InDragDrop);

	auto Choise = ComponentPickerContext->FindComponentForVariable(Ptr->GetVariableProperty()->GetFName());
	if (Choise)
	{
		OnComponentSelected(Choise);
	}

	return FReply::Handled();
}

#endif

#undef LOCTEXT_NAMESPACE
