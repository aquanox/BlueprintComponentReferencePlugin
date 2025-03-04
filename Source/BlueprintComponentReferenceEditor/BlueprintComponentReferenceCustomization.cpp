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

#define LOCTEXT_NAMESPACE "BlueprintComponentReferenceCustomization"


TSharedRef<IPropertyTypeCustomization> FBlueprintComponentReferenceCustomization::MakeInstance()
{
	return MakeShared<FBlueprintComponentReferenceCustomization>();
}

void FBlueprintComponentReferenceCustomization::CustomizeHeader(TSharedRef<IPropertyHandle> InPropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	PropertyHandle = InPropertyHandle;

	ClassHelper = FBCREditorModule::GetReflectionHelper();
	check(ClassHelper.IsValid());

	ComponentPickerContext.Reset();
	CachedComponentNode.Reset();
	CachedPropertyAccess = FPropertyAccess::Fail;

	// this will disable use of default "Reset To Defaults" for this header
	InPropertyHandle->MarkResetToDefaultCustomized(true);

	ViewSettings.ResetSettings();

	FStructProperty* const Property = CastFieldChecked<FStructProperty>(InPropertyHandle->GetProperty());
	if (Property && ensureAlways(FBlueprintComponentReferenceHelper::IsComponentReferenceType(Property->Struct)))
	{
		ViewSettings.LoadSettingsFromProperty(InPropertyHandle->GetMetaDataProperty());

		BuildComboBox();

		InPropertyHandle->SetOnPropertyValueChanged(FSimpleDelegate::CreateSP(this, &FBlueprintComponentReferenceCustomization::OnPropertyValueChanged, Property->GetFName()));
		OnPropertyValueChanged(Property->GetFName());

		TSharedPtr<SHorizontalBox> ValueContent;
		SAssignNew(ValueContent, SHorizontalBox)
		+SHorizontalBox::Slot()
		.FillWidth(1.0f)
		[
			ComponentComboButton.ToSharedRef()
		];

		if (ViewSettings.bUseNavigate)
		{
			ValueContent->AddSlot()
			.AutoWidth()
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			.Padding(2.0f, 1.0f)
			[
				PropertyCustomizationHelpers::MakeBrowseButton(
					FSimpleDelegate::CreateSP(this, &FBlueprintComponentReferenceCustomization::OnNavigateComponent),
					LOCTEXT( "NavigateButtonToolTipText", "Select Component in Component Editor"),
					/* enabled = */ true, /* actor icon = */ true
				)
			];
		}

		if (ViewSettings.bUseClear)
		{
			ValueContent->AddSlot()
			.AutoWidth()
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			.Padding(2.0f, 1.0f)
			[
				PropertyCustomizationHelpers::MakeClearButton(
					FSimpleDelegate::CreateSP(this, &FBlueprintComponentReferenceCustomization::OnClear),
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
		.HAlign(HAlign_Fill)
		[
			ValueContent.ToSharedRef()
		]
		.IsEnabled(MakeAttributeSP(this, &FBlueprintComponentReferenceCustomization::CanEdit));
	}
}

void FBlueprintComponentReferenceCustomization::CustomizeChildren(TSharedRef<IPropertyHandle> InStructPropertyHandle, IDetailChildrenBuilder& StructBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
	uint32 NumberOfChild = 0;
	if (InStructPropertyHandle->GetNumChildren(NumberOfChild) == FPropertyAccess::Success)
	{
		for (uint32 Index = 0; Index < NumberOfChild; ++Index)
		{
			TSharedRef<IPropertyHandle> ChildPropertyHandle = InStructPropertyHandle->GetChildHandle(Index).ToSharedRef();

			ChildPropertyHandle->MarkResetToDefaultCustomized(true);

			ChildPropertyHandle->SetOnPropertyValueChanged(
				FSimpleDelegate::CreateSP(this, &FBlueprintComponentReferenceCustomization::OnPropertyValueChanged,
					ChildPropertyHandle->GetProperty()->GetFName()
				)
			);

			StructBuilder.AddProperty(ChildPropertyHandle)
				.ShowPropertyButtons(!ViewSettings.bUsePicker)
				.ShouldAutoExpand(!ViewSettings.bUsePicker)
				.IsEnabled(MakeAttributeSP(this, &FBlueprintComponentReferenceCustomization::CanEditChildren));
		}
	}
}

FString FBlueprintComponentReferenceCustomization::GetLoggingContextString() const
{
	TStringBuilder<32> Buffer;
	if (PropertyHandle.IsValid())
	{
		Buffer.Append(PropertyHandle->GeneratePathToProperty());
	}
	else
	{
		Buffer.Append(TEXT("Invalid"));
	}
	return Buffer.ToString();
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
			.Font( FAppStyle::GetFontStyle( "PropertyWindow.NormalFont" ) )
			.Text(this, &FBlueprintComponentReferenceCustomization::OnGetComponentName)
			.ToolTipText(this, &FBlueprintComponentReferenceCustomization::OnGetComponentTooltip)
		]
	];

	SAssignNew(ComponentComboButton, SComboButton)
		.OnGetMenuContent(this, &FBlueprintComponentReferenceCustomization::OnGetMenuContent)
		.OnMenuOpenChanged(this, &FBlueprintComponentReferenceCustomization::OnMenuOpenChanged)
		.ContentPadding(FMargin(2,2,2,1))
		.Visibility(ViewSettings.bUsePicker ? EVisibility::Visible : EVisibility::Collapsed)
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
			UE_LOG(LogComponentReferenceEditor, Log, TEXT("%s -> ExactClass=%s"), *GetLoggingContextString(), *ViewSettings.ActorClass.ToString());
			
			OuterActorClass = ViewSettings.ActorClass.Get();
		}
		else
		{
			UE_LOG(LogComponentReferenceEditor, Log, TEXT("%s -> ExactClass=%s (loading)"), *GetLoggingContextString(), *ViewSettings.ActorClass.ToString());
			
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
			UE_LOG(LogComponentReferenceEditor, Log, TEXT("%s DetermineOuterActor: TestObject=%s"), *GetLoggingContextString(), *GetNameSafe(OuterActor));
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
			// only support regular blueprints (not anim or others)
			if (UBlueprintGeneratedClass* Class = ExactCast<UBlueprintGeneratedClass>(OuterObject))
			{
				UE_LOG(LogComponentReferenceEditor, Log, TEXT("%s ->  Class=%s"), *GetLoggingContextString(), *GetNameSafe(Class));
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
				UE_LOG(LogComponentReferenceEditor, Log, TEXT("%s ->  FunctionClass=%s"), *GetLoggingContextString(), *GetNameSafe(OwnerClass));

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

	UE_LOG(LogComponentReferenceEditor, Log, TEXT("%s DetermineOuterActor: Located Actor=%s BP=%s"), *GetLoggingContextString(), *GetNameSafe(OuterActor), *GetNameSafe(OuterActorClass));

	if (!ComponentPickerContext.IsValid()
		|| ComponentPickerContext->GetActor() != OuterActor
		|| ComponentPickerContext->GetClass() != OuterActorClass)
	{
		ComponentPickerContext = ClassHelper->CreateChooserContext(OuterActor, OuterActorClass, GetLoggingContextString());
	}

	if (!ComponentPickerContext.IsValid())
	{
		UE_LOG(LogComponentReferenceEditor, Warning, TEXT("%s Failed to determine context"), *GetLoggingContextString());
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
	UE_LOG(LogComponentReferenceEditor, Log, TEXT("%s SetValue %s"), *GetLoggingContextString(), *Value.ToString());

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

void FBlueprintComponentReferenceCustomization::OnPropertyValueChanged(FName Source)
{
	UE_LOG(LogComponentReferenceEditor, Log, TEXT("%s OnPropertyValueChanged"), *GetLoggingContextString());

	CachedComponentNode.Reset();
	DetermineContext();

	FBlueprintComponentReference TmpComponentReference;
	CachedPropertyAccess = GetValue(TmpComponentReference);
	if (CachedPropertyAccess == FPropertyAccess::Success)
	{
		if (!IsComponentReferenceValid(TmpComponentReference))
		{ // reset property
			CachedComponentNode.Reset();
			if (!TmpComponentReference.IsNull())
			{
				SetValue(FBlueprintComponentReference());
			}
		}
		else if (ComponentPickerContext.IsValid())
		{ 
			TSharedPtr<FComponentInfo> Found = ComponentPickerContext->FindComponent(TmpComponentReference);
			if (Found.IsValid())
			{
				CachedComponentNode = Found;
			}
			else
			{  // reset property
				CachedComponentNode.Reset();
				if (!TmpComponentReference.IsNull())
				{
					SetValue(FBlueprintComponentReference());
				}
			}
		}
	}
}

bool FBlueprintComponentReferenceCustomization::CanEdit() const
{
	if (PropertyHandle.IsValid())
	{
		return !PropertyHandle->IsEditConst();
	}
	return ViewSettings.bUsePicker;
}

bool FBlueprintComponentReferenceCustomization::CanEditChildren() const
{
	if (!ViewSettings.bUsePicker)
	{
		return CanEdit();
	}
	return CanEdit() && !ComponentPickerContext.IsValid();
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

	if (!ComponentPickerContext.IsValid())
	{
		DetermineContext();
	}

	TMap<FString, TArray<TSharedRef<FComponentInfo>>> ChoosableElements;
	if (ComponentPickerContext.IsValid())
	{
		for (auto& HierarchyInfo : ComponentPickerContext->ClassHierarchy)
		{
			if (HierarchyInfo->GetNodes().IsEmpty())
				continue;

			TArray<TSharedRef<FComponentInfo>> LocalArray;
			for(auto& Node : HierarchyInfo->GetNodes())
			{
				if (TestNode(Node) && TestObject(Node->GetComponentTemplate()))
				{
					LocalArray.Add(Node.ToSharedRef());
				}
			}

			if (!LocalArray.IsEmpty())
			{
				ChoosableElements.Emplace(HierarchyInfo->GetDisplayText().ToString(), MoveTemp(LocalArray));
			}
		}
	}

	if (!ChoosableElements.IsEmpty())
	{
		for (auto& Element : ChoosableElements)
		{
			MenuBuilder.BeginSection(NAME_None, FText::FromString(Element.Key));

			for(auto& Node : Element.Value)
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

void FBlueprintComponentReferenceCustomization::OnNavigateComponent()
{
	TSharedPtr<FComponentInfo> LocalNode = CachedComponentNode.Pin();
	if (!LocalNode.IsValid() || !ComponentPickerContext.IsValid())
	{
		return;
	}

	AActor* const SearchActor = ComponentPickerContext.IsValid() ? ComponentPickerContext->GetActor() : nullptr;

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
				BlueprintEditor =  static_cast<FBlueprintEditor*>(AssetEditorSubsystem->FindEditorForAsset(Blueprint, false));
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

void FBlueprintComponentReferenceCustomization::OnComponentSelected(TSharedRef<FComponentInfo> Node)
{
	ComponentComboButton->SetIsOpen(false);

	CachedComponentNode = Node;

	FBlueprintComponentReference Result;
	// Todo: desired mode override by metadata, unless really desired 
	if (Node->GetDesiredMode() == EBlueprintComponentReferenceMode::Property)
	{
		Result = FBlueprintComponentReference(EBlueprintComponentReferenceMode::Property, Node->GetVariableName());
	}
	else
	{
		Result = FBlueprintComponentReference(EBlueprintComponentReferenceMode::Path, Node->GetObjectName());
	}

	SetValue(Result);
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

		if (!Node->IsNativeComponent() && ViewSettings.bShowBlueprint)
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

	return bAllowedToSetBasedOnFilter;
}

#undef LOCTEXT_NAMESPACE
