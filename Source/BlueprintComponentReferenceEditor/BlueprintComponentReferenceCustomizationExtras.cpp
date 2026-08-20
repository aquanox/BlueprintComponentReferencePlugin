#include "BlueprintComponentReferenceCustomizationExtras.h"

#include "Components/MeshComponent.h"
#include "PropertyCustomizationHelpers.h"

#define LOCTEXT_NAMESPACE "BlueprintComponentReferenceCustomization"

#if defined(WITH_BCR_EXTRAS) && WITH_BCR_EXTRAS

DEFINE_LOG_CATEGORY_STATIC(LogMSRCustomization, All, All);

namespace
{
	static const FName FIELD_Mode("Mode");
	static const FName FIELD_Value("Value");
	static const FName FIELD_SocketName("SocketName");
}

FMeshSocketReferenceCustomization::FMeshSocketReferenceCustomization()
{
	StructFilter.BindLambda([](const UScriptStruct* Struct)
	{
		return Struct->IsChildOf(FMeshSocketReference::StaticStruct());
	});
}

void FMeshSocketReferenceCustomization::CustomizeHeaderImpl(TSharedRef<IPropertyHandle> InPropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& PTCUtils)
{
	ComponentComboButton = BuildComboBox();

	SocketComboButton = BuildSocketCombo();
	UpdateSocketComboData();

	TSharedPtr<SHorizontalBox> ValueContent;
	SAssignNew(ValueContent, SHorizontalBox)
	+SHorizontalBox::Slot()
	.FillWidth(1.0f)
	[
		SNew(SVerticalBox)
		+SVerticalBox::Slot()
		.Padding(2, 0, 0, 0)
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
		]
		+SVerticalBox::Slot()
		.Padding(0, 2, 0, 2)
		[
			SocketComboButton.ToSharedRef()
		]

	];

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

void FMeshSocketReferenceCustomization::CustomizeChildImpl(TSharedRef<IPropertyHandle> InPropertyHandle, TSharedRef<IPropertyHandle> InChildPropertyHandle, IDetailChildrenBuilder& StructBuilder, IPropertyTypeCustomizationUtils& PTCUtils)
{
	Super::CustomizeChildImpl(InPropertyHandle, InChildPropertyHandle, StructBuilder, PTCUtils);
}

void FMeshSocketReferenceCustomization::OnComponentSelected(FBlueprintComponentReference& NewValue)
{
	Super::OnComponentSelected(NewValue);

	UpdateSocketComboData();
}

void FMeshSocketReferenceCustomization::OnPropertyValueChanged(FName Source)
{
	Super::OnPropertyValueChanged(Source);

	if (Source != FIELD_SocketName)
	{
		UpdateSocketComboData();
	}
}

TSharedPtr<SWidget> FMeshSocketReferenceCustomization::BuildSocketCombo()
{
	auto SocketNameHandle = PropertyHandle->GetChildHandle(FIELD_SocketName);

	FPropertyComboBoxArgs Args;
	Args.PropertyHandle = SocketNameHandle;
	Args.OnGetStrings.BindSP(this, &FMeshSocketReferenceCustomization::SocketCombo_OnGetStrings);
	//Args.OnValueSelected.BindSP(this, &FMeshSocketReferenceCustomization::SocketCombo_OnSelect);
	Args.ShowSearchForItemCount = 10;
	return PropertyCustomizationHelpers::MakePropertyComboBox(Args);
}

void FMeshSocketReferenceCustomization::UpdateSocketComboData()
{
	SocketDataSource.Empty();
	if (auto Pinned = CachedComponentNode.Pin())
	{
		SocketDataSource.Add(MakeShared<FString>("None"));
		if (UMeshComponent* Component = Cast<UMeshComponent>(Pinned->GetComponentTemplate()))
		{
			TArray<FName> Names = Component->GetAllSocketNames();
			Algo::Transform(Names, SocketDataSource, [](FName A) { return MakeShared<FString>(A.ToString()); });
		}
	}
}

void FMeshSocketReferenceCustomization::SocketCombo_OnGetStrings(TArray<TSharedPtr<FString>>& Strings, TArray<TSharedPtr<SToolTip>>& Tooltips, TArray<bool>& Restricted) const
{
	Strings.Reserve(SocketDataSource.Num());
	Tooltips.Reserve(SocketDataSource.Num());
	Restricted.Reserve(SocketDataSource.Num());

	for (TSharedPtr<FString> Item : SocketDataSource)
	{
		Strings.Emplace( Item );
		Tooltips.Emplace(nullptr);
		Restricted.Emplace(false);
	}
}

void FMeshSocketReferenceCustomization::SocketCombo_OnSelect(const FString& Value)
{

}

#endif

#undef LOCTEXT_NAMESPACE
