// Copyright 2024, Aquanox.

#include "BlueprintComponentReferenceHelper.h"

#include "BlueprintComponentReferenceEditor.h"
#include "BlueprintComponentReferenceMetadata.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Misc/CoreDelegates.h"
#include "Modules/ModuleManager.h"
#include "UObject/UObjectIterator.h"
#include "UObject/Package.h"
#include "Misc/EngineVersionComparison.h"
#include "Misc/PackageName.h"
#include "PropertyHandle.h"
#include "BlueprintEditor.h"
#include "BlueprintEditorTabs.h"
#include "Context/ComponentPickerContextFactory.h"
#include "Subsystems/AssetEditorSubsystem.h"

#define LOCTEXT_NAMESPACE "BlueprintComponentReference"

UClass* FBlueprintComponentReferenceHelper::FindClassByName(const FString& ClassName)
{
	if (ClassName.IsEmpty())
		return nullptr;
	UClass* ResultClass = nullptr;
#if UE_VERSION_OLDER_THAN(5, 1, 0)
	if (FPackageName::IsShortPackageName(ClassName))
	{
		ResultClass = FindObject<UClass>(ANY_PACKAGE, *ClassName);
	}
	else
	{
		ResultClass = FindObject<UClass>(nullptr, *ClassName);
	}
#else
	ResultClass =  UClass::TryFindTypeSlow<UClass>(ClassName, EFindFirstObjectOptions::EnsureIfAmbiguous);
	if (!ResultClass)
	{
		ResultClass = LoadObject<UClass>(nullptr, *ClassName);
	}
#endif
	return ResultClass;
}

bool FBlueprintComponentReferenceHelper::IsRootComponentReference(const FBlueprintComponentReference& InRef)
{
	static const FBlueprintComponentReference RootPropertyName(EBlueprintComponentReferenceMode::Property, TEXT("RootComponent"));
	return InRef == RootPropertyName;
}

bool FBlueprintComponentReferenceHelper::InvokeComponentFilter(TSharedPtr<class IPropertyHandle> InProperty, const FString& InFilterFn, const UObject* InObj)
{
	if (!InFilterFn.IsEmpty())
	{
		TArray<UObject*> ObjectList;
		InProperty->GetOuterObjects(ObjectList);

		FName CallableName (*InFilterFn);

		// Check for external function references
		if (InFilterFn.Contains(TEXT(".")))
		{
			ObjectList.Empty();
#if UE_VERSION_OLDER_THAN(5, 7, 0)
			const UFunction* FilterFunction = FindObject<UFunction>(nullptr, *InFilterFn, true);
#else
            const UFunction* FilterFunction = FindObject<UFunction>(nullptr, *InFilterFn, EFindObjectFlags::ExactClass);
#endif
			if (FilterFunction && FilterFunction->HasAnyFunctionFlags(FUNC_Static))
			{
				UObject* FilterOwnerCDO = FilterFunction->GetOuterUClass()->GetDefaultObject();
				CallableName = FilterFunction->GetFName();
				ObjectList.Add(FilterOwnerCDO);
			}
		}

		if (ObjectList.Num())
		{
			FEditorScriptExecutionGuard ScriptExecutionGuard;

			for (UObject* Object : ObjectList)
			{
				if (Object && Object->FindFunction(CallableName) != nullptr)
				{
					FBlueprintComponentReferenceMetadata::FComponentFilterFunc Func;
					Func.BindUFunction(Object, CallableName);
					return Func.Execute(Cast<const UActorComponent>(InObj));
				}
			}
		}
	}

	return true;
}

FString FBlueprintComponentReferenceHelper::BuildComponentDebugInfo(const UActorComponent* Obj)
{
	if (!::IsValid(Obj))
		return "null";

	TStringBuilder<256> Base;
	Base.Appendf(TEXT("%p:%s %s"), Obj, *Obj->GetName(), *Obj->GetClass()->GetName());
	Base.Appendf(TEXT(" Flags=%d"), Obj->GetFlags());
	Base.Appendf(TEXT(" Method=%s"), *StaticEnum<EComponentCreationMethod>()->GetNameStringByValue((int64)Obj->CreationMethod));
	return Base.ToString();
}

bool FBlueprintComponentReferenceHelper::IsComponentReferenceProperty(const FProperty* InProperty)
{
	bool bDoesMatch = false;

	if (auto AsStruct = CastField<FStructProperty>(InProperty))
	{
		bDoesMatch = IsComponentReferenceType(AsStruct->Struct);
	}
	else if (auto AsArray = CastField<FArrayProperty>(InProperty))
	{
		bDoesMatch = IsComponentReferenceProperty(AsArray->Inner);
	}
	else if (auto AsSet = CastField<FSetProperty>(InProperty))
	{
		bDoesMatch = IsComponentReferenceProperty(AsSet->ElementProp);
	}
	else if (auto AsMap = CastField<FMapProperty>(InProperty))
	{
		bDoesMatch = IsComponentReferenceProperty(AsMap->KeyProp) || IsComponentReferenceProperty(AsMap->ValueProp);
	}

	return bDoesMatch;
}

bool FBlueprintComponentReferenceHelper::IsComponentReferenceType(const UStruct* InStruct)
{
	return InStruct && InStruct->IsChildOf(FBlueprintComponentReference::StaticStruct());
}

TSharedPtr<FComponentPickerContext> FBlueprintComponentReferenceHelper::CreateChooserContext(AActor* InActor, UClass* InClass, const FString& InLabel)
{
	return FBCREditorModule::GetContextFactory()->CreateChooserContext(InActor, InClass, InLabel);
}

bool FBlueprintComponentReferenceHelper::IsBlueprintProperty(const FProperty* VariableProperty)
{
	if (UClass* const VarSourceClass = VariableProperty ? VariableProperty->GetOwner<UClass>() : nullptr)
	{
		return (VarSourceClass->ClassGeneratedBy != nullptr);
	}
	return false;
}

void FBlueprintComponentReferenceHelper::TryNavigateToComponent(AActor* SearchActor, TSharedPtr<FComponentInfo> LocalNode)
{
#if !UE_VERSION_OLDER_THAN(5, 0, 0)
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
#endif
}


#undef LOCTEXT_NAMESPACE
