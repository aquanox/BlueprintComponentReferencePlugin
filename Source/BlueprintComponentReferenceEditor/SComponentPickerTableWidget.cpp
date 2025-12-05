#include "SComponentPickerTableWidget.h"

#include "PropertyCustomizationHelpers.h"
#include "SlateStyleHelper.h"
#include "Styling/SlateIconFinder.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Images/SImage.h"

#define LOCTEXT_NAMESPACE "SComponentPickerTableWidget"

SComponentPickerTableWidget::FTreeViewStatePersistence SComponentPickerTableWidget::PersistenceManager;

struct FComponentPickerTableWidgetMetrics
{
	// todo: pick better initial sizes on larger res
	static constexpr int32 MenuMinHeight = 200;
	static constexpr int32 MenuMaxHeight = 800;
	static constexpr int32 MenuMinWidth = 300;
	static constexpr int32 MenuMaxWidth = 500;

	static constexpr int32 RowHeight = 20;
	static constexpr int32 IconSize = 16;
	static FMargin IconPadding() { return FMargin(0.f, 1.f, 6.f, 1.f); }
};

void SComponentPickerTableWidget::Construct(const FArguments& InArgs)
{
	Context = InArgs._Context;
	OnSelected = InArgs._OnSelected;
	DataSource = InArgs._Items;
	FilterText = FText::GetEmpty();

	TreeItems.Reserve(DataSource.Num());
	for (const FComponentPickerGroup& Group : DataSource)
	{
		auto TreeItem = MakeShared<FComponentTreeItem>();
		TreeItem->CategoryInfo = Group.Category;
		TreeItem->bIsCategory = true;
		TreeItem->bIsExpandable = true;
		TreeItem->bIsExpanded = true;
		TreeItem->ComponentIcon = FSlateIconFinder::FindIconForClass(Group.Category->GetClass());

		for (const TSharedPtr<FComponentInfo>& Element : Group.Elements)
		{
			auto ChildTreeItem = MakeShared<FComponentTreeItem>();
			ChildTreeItem->CategoryInfo = Group.Category;
			ChildTreeItem->ComponentInfo = Element;
			ChildTreeItem->ComponentIcon = FSlateIconFinder::FindIconForClass(Element->GetComponentClass());

			TreeItem->Children.Add(MoveTemp(ChildTreeItem));
		}

		TreeItems.Add(MoveTemp(TreeItem));
	}

	// Build the actual subsystem browser view panel
	ChildSlot
	[
		SNew(SBox)
		.MinDesiredHeight(FComponentPickerTableWidgetMetrics::MenuMinHeight)
		.MaxDesiredHeight(FComponentPickerTableWidgetMetrics::MenuMaxHeight)
		.MinDesiredWidth(FComponentPickerTableWidgetMetrics::MenuMinWidth)
		.MaxDesiredWidth(FComponentPickerTableWidgetMetrics::MenuMaxWidth)
		[
			SNew(SOverlay)

			+ SOverlay::Slot()
			[
				SNew(SBorder).BorderImage(FSlateStyleHelper::GetBrush("Brushes.Recessed"))
			]

			+ SOverlay::Slot()
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(SBorder)
					.BorderImage(FSlateStyleHelper::GetBrush(TEXT("ToolPanel.GroupBorder")))
					[
						SNew(SHorizontalBox)

						// Filter box
						+ SHorizontalBox::Slot()
						.FillWidth(1)
						[
							SNew(SSearchBox)
							.ToolTipText(LOCTEXT("FilterSearchToolTip", "Type here to search Components"))
							.HintText(LOCTEXT("FilterSearchHint", "Search Components"))
							.OnTextChanged(this, &SComponentPickerTableWidget::SetFilterText)
						]

						// Options?
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.Padding(2, 2, 4, 2)
						[
							SNullWidget::NullWidget
						]
					]
				]
				+ SVerticalBox::Slot()
				.FillHeight(1)
				.Padding(2, 4, 2, 4)
				[
					SNew(SBorder)
					.BorderImage(FSlateStyleHelper::GetBrush(TEXT("ToolPanel.GroupBorder")))
					[
						SNew(SVerticalBox)

						// Table
						+ SVerticalBox::Slot()
						.FillHeight(1)
						.Padding(0, 0, 0, 2)
						[
							SAssignNew(TreeView, SComponentPickerTreeView)
							.TreeItemsSource(&TreeItems)
							.SelectionMode(ESelectionMode::Single)
							.OnGenerateRow(this, &SComponentPickerTableWidget::GenerateTreeRow)
							.OnGetChildren(this, &SComponentPickerTableWidget::GatherChildrenForRow)
							.OnSelectionChanged(this, &SComponentPickerTableWidget::TreeRowSelected)
							.OnExpansionChanged(this,  &SComponentPickerTableWidget::TreeRowExpanded)
						]

						// View options
						+ SVerticalBox::Slot()
						.Padding(0, 0, 0, 2)
						.AutoHeight()
						[
							SNew(SHorizontalBox)
							.Visibility(EVisibility::Visible)
						]
					]
				]
			]
		]
	];

	for (const TSharedPtr<FComponentTreeItem>& Item : TreeItems)
	{
		TreeView->SetItemExpansion(Item, PersistenceManager.GetState(Item));
	}
}

TSharedRef<ITableRow> SComponentPickerTableWidget::GenerateTreeRow(FComponentTreeItemPtr Item, const TSharedRef<STableViewBase>& TableViewBase)
{
	return SNew(SComponentPickerTreeItem, TableViewBase)
		.TreeItem(Item)
		.HighlightText(this, &SComponentPickerTableWidget::GetFilterText);
}

void SComponentPickerTableWidget::GatherChildrenForRow(FComponentTreeItemPtr Item, TArray<FComponentTreeItemPtr>& Children)
{
	if (Item->IsCategory())
	{
		for (const TSharedPtr<FComponentTreeItem>& Child : Item->Children)
		{
			if (!FilterText.IsEmpty() && !Child->ComponentInfo->GetDisplayText().ToString().Contains(FilterText.ToString()))
				continue;

			Children.Add(Child);
		}
	}
}

void SComponentPickerTableWidget::TreeRowSelected(FComponentTreeItemPtr Item, ESelectInfo::Type Type)
{
	if (Type == ESelectInfo::OnMouseClick && Item && Item->IsComponent())
	{
		OnSelected.ExecuteIfBound(Item->ComponentInfo);
	}
}

void SComponentPickerTableWidget::TreeRowExpanded(FComponentTreeItemPtr Item, bool State)
{
	PersistenceManager.SetState(Item, State);
}

void SComponentPickerTableWidget::SetFilterText(const FText& Text)
{
	FilterText = Text;
	TreeView->RequestListRefresh();
}

void SComponentPickerTreeItem::Construct(const FArguments& InArgs, TSharedRef<STableViewBase> OwnerTableView)
{
	Table = OwnerTableView;
	Item = InArgs._TreeItem;

	auto Args = Super::FArguments()
    .Style(&FSlateStyleHelper::Get().GetWidgetStyle<FTableRowStyle>("SceneOutliner.TableViewRow"))
    .ShowSelection(true)
    .Content()
	[
		SNew(SBox)
		.MinDesiredHeight(FComponentPickerTableWidgetMetrics::RowHeight)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.Padding(FComponentPickerTableWidgetMetrics::IconPadding())
			.AutoWidth()
			[
				SNew(SBox)
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				.HeightOverride(FComponentPickerTableWidgetMetrics::IconSize)
				.WidthOverride(FComponentPickerTableWidgetMetrics::IconSize)
				[
					SNew(SImage).Image(this, &SComponentPickerTreeItem::GetIcon)
				]
			]

			+ SHorizontalBox::Slot()
			.Padding(0, 4, 6, 4)
			.FillWidth(1)
			[
				SNew(STextBlock)
				.Text(this, &SComponentPickerTreeItem::GetDisplayText)
				.ToolTipText(this, &SComponentPickerTreeItem::GetTooltipText)
				.HighlightText(InArgs._HighlightText)
			]
		]
	];

	Super::Construct(Args, OwnerTableView);
}

FText SComponentPickerTreeItem::GetDisplayText() const
{
	return Item->IsComponent() ? Item->ComponentInfo->GetDisplayText() : Item->CategoryInfo->GetDisplayText();
}

FText SComponentPickerTreeItem::GetTooltipText() const
{
	return Item->IsComponent() ? Item->ComponentInfo->GetTooltipText() : Item->CategoryInfo->GetDisplayText();
}

EVisibility SComponentPickerTreeItem::GetIconVisibility() const
{
	return Item->ComponentIcon.IsSet() ? EVisibility::Visible : EVisibility::Collapsed;
}

const FSlateBrush* SComponentPickerTreeItem::GetIcon() const
{
	return Item->ComponentIcon.IsSet() ? Item->ComponentIcon.GetIcon() : FSlateStyleHelper::GetDefaultBrush();
}

FReply SComponentPickerTreeItem::OnMouseButtonDoubleClick(const FGeometry& InMyGeometry, const FPointerEvent& InMouseEvent)
{
	return Super::OnMouseButtonDoubleClick(InMyGeometry, InMouseEvent);
}

bool SComponentPickerTableWidget::FTreeViewStatePersistence::GetState(FComponentTreeItemPtr InPtr) const
{
	if (!InPtr->IsCategory()) return true;
	auto* Value = State.Find(GetFNameSafe(InPtr->CategoryInfo->GetClassObject()));
	return Value ? *Value : true;
}

void SComponentPickerTableWidget::FTreeViewStatePersistence::SetState(FComponentTreeItemPtr InPtr, bool bValue)
{
	if (!InPtr->IsCategory()) return;
	State.Add(GetFNameSafe(InPtr->CategoryInfo->GetClassObject()), bValue);
}

#undef LOCTEXT_NAMESPACE
