#pragma once

#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/SHeaderRow.h"
#include "Widgets/Views/STableRow.h"
#include "BlueprintComponentReferenceHelper.h"
#include "Styling/SlateStyle.h"
#include "Textures/SlateIcon.h"
#include "Widgets/Views/STreeView.h"

struct FComponentTreeItem : public TSharedFromThis<FComponentTreeItem>
{
	TSharedPtr<FHierarchyInfo> CategoryInfo;
	TSharedPtr<FComponentInfo> ComponentInfo;
	FSlateIcon ComponentIcon;
	TArray<TSharedPtr<FComponentTreeItem>> Children;

	bool bIsCategory = false;
	bool bIsExpandable = false;
	bool bIsExpanded = false;

	bool IsComponent() const { return !bIsCategory; }
	bool IsCategory() const { return bIsCategory; }
	bool CanHaveChildren() const { return bIsCategory; }

};
using FComponentTreeItemPtr = TSharedPtr<FComponentTreeItem>;

class SComponentPickerTreeItem : public STableRow<FComponentTreeItemPtr>
{
	using Super = STableRow<FComponentTreeItemPtr>;
public:
	SLATE_BEGIN_ARGS(SComponentPickerTreeItem) { }
		SLATE_ARGUMENT(FComponentTreeItemPtr, TreeItem)
		SLATE_ATTRIBUTE(FText, HighlightText)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, TSharedRef<STableViewBase> OwnerTableView);

	FText GetDisplayText() const;
	FText GetTooltipText() const;
	EVisibility GetIconVisibility() const;
	const FSlateBrush* GetIcon() const;

	virtual FReply OnMouseButtonDoubleClick(const FGeometry& InMyGeometry, const FPointerEvent& InMouseEvent) override;
private:
	TSharedPtr<STableViewBase> Table;
	TSharedPtr<FComponentTreeItem> Item;
};

using SComponentPickerTreeView = STreeView<FComponentTreeItemPtr>;

class SComponentPickerTableWidget : public SCompoundWidget
{
public:
	using FOnClear = TDelegate<void()>;
	using FOnSelected = TDelegate<void(TSharedPtr<FComponentInfo>)>;

	SLATE_BEGIN_ARGS(SComponentPickerTableWidget) {}
		SLATE_ARGUMENT(TSharedPtr<FComponentPickerContext>, Context)
		SLATE_ARGUMENT(TArray<FComponentPickerGroup>, Items)
		SLATE_EVENT(FOnClear, OnClear)
		SLATE_EVENT(FOnSelected, OnSelected)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	TSharedRef<ITableRow> GenerateTreeRow(FComponentTreeItemPtr Item, const TSharedRef<STableViewBase>& TableViewBase);
	void GatherChildrenForRow(FComponentTreeItemPtr Item, TArray<FComponentTreeItemPtr>& Children);
	void TreeRowSelected(FComponentTreeItemPtr Item, ESelectInfo::Type Type);
	void TreeRowExpanded(FComponentTreeItemPtr Item, bool State);

	void SetFilterText(const FText& Text);
	FText GetFilterText() const { return FilterText; }
private:
	TSharedPtr<FComponentPickerContext> Context;
	TSharedPtr<FComponentPickerFilter> Filter;
	FText FilterText;

	TArray<FComponentPickerGroup> DataSource;

	FOnSelected OnClear;
	FOnSelected OnSelected;

	TSharedPtr<SComponentPickerTreeView> TreeView;
	TArray<TSharedPtr<FComponentTreeItem>> TreeItems;
	TSharedPtr< SHeaderRow > HeaderRowWidget;

	struct FTreeViewStatePersistence
	{
		TMap<FName, bool> State;

		bool GetState(FComponentTreeItemPtr InPtr) const;
		void SetState(FComponentTreeItemPtr InPtr, bool bValue);
	};
	static FTreeViewStatePersistence PersistenceManager;
};
