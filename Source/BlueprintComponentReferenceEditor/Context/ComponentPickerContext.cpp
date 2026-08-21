// Copyright 2024, Aquanox.

#include "ComponentPickerContext.h"

#include "BlueprintComponentReferenceHelper.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Misc/CoreDelegates.h"
#include "Modules/ModuleManager.h"
#include "UObject/UObjectIterator.h"
#include "UObject/Package.h"
#include "Misc/PackageName.h"
#include "PropertyHandle.h"
#include "BlueprintEditor.h"

#define LOCTEXT_NAMESPACE "BlueprintComponentReference"


UActorComponent* FComponentInfo::GetComponentTemplate() const
{
	return Object.Get();
}

UClass* FComponentInfo::GetComponentClass() const
{
	if (ObjectClass.IsValid())
	{
		return ObjectClass.Get();
	}
	return Object.IsValid() ? Object->GetClass() : nullptr;
}

FName FComponentInfo::GetNodeID() const
{
	FName ItemName = GetVariableName();
	if (ItemName == NAME_None)
	{
		UActorComponent* ComponentTemplateOrInstance = GetComponentTemplate();
		if (ComponentTemplateOrInstance != nullptr)
		{
			ItemName = ComponentTemplateOrInstance->GetFName();
		}
	}
	return ItemName;
}

// Custom version that will test instances as well as CDO
static FName FComponentEditorUtils_FindVariableNameGivenComponentInstance(const UActorComponent* ComponentInstance)
{
	check(ComponentInstance != nullptr);

	// When names mismatch, try finding a differently named variable pointing to the the component (the mismatch should only be possible for native components)
	auto FindPropertyReferencingComponent = [](const UActorComponent* Component, bool bUseInstance) -> FProperty*
	{
		if (AActor* OwnerActor = Component->GetOwner())
		{
			UClass* OwnerClass = OwnerActor->GetClass();
			AActor* SearchTarget = bUseInstance ? OwnerActor : CastChecked<AActor>(OwnerClass->GetDefaultObject());

			for (TFieldIterator<FObjectProperty> PropIt(OwnerClass, EFieldIteratorFlags::IncludeSuper); PropIt; ++PropIt)
			{
				FObjectProperty* TestProperty = *PropIt;
				if (Component->GetClass()->IsChildOf(TestProperty->PropertyClass))
				{
					void* TestPropertyInstanceAddress = TestProperty->ContainerPtrToValuePtr<void>(SearchTarget);
					UObject* ObjectPointedToByProperty = TestProperty->GetObjectPropertyValue(TestPropertyInstanceAddress);
					if (ObjectPointedToByProperty == Component)
					{
						// This property points to the component archetype, so it's an anchor even if it was named wrong
						return TestProperty;
					}
				}
			}

			// do not lookup in arrays.
			// it will break GetNodeId naming if many components found in one
			/*
			for (TFieldIterator<FArrayProperty> PropIt(OwnerClass, EFieldIteratorFlags::IncludeSuper); PropIt; ++PropIt)
			{
				FArrayProperty* TestProperty = *PropIt;
				void* ArrayPropInstAddress = TestProperty->ContainerPtrToValuePtr<void>(SearchTarget);

				FObjectProperty* ArrayEntryProp = CastField<FObjectProperty>(TestProperty->Inner);
				if ((ArrayEntryProp == nullptr) || !ArrayEntryProp->PropertyClass->IsChildOf<UActorComponent>())
				{
					continue;
				}

				FScriptArrayHelper ArrayHelper(TestProperty, ArrayPropInstAddress);
				for (int32 ComponentIndex = 0; ComponentIndex < ArrayHelper.Num(); ++ComponentIndex)
				{
					UObject* ArrayElement = ArrayEntryProp->GetObjectPropertyValue(ArrayHelper.GetRawPtr(ComponentIndex));
					if (ArrayElement == Component)
					{
						return TestProperty;
					}
				}
			}
			*/
		}

		return nullptr;
	};

	if (AActor* OwnerActor = ComponentInstance->GetOwner())
	{
		// First see if the name just works
		UClass* OwnerActorClass = OwnerActor->GetClass();
		if (FObjectProperty* TestProperty = FindFProperty<FObjectProperty>(OwnerActorClass, ComponentInstance->GetFName()))
		{
			if (ComponentInstance->GetClass()->IsChildOf(TestProperty->PropertyClass))
			{
				return TestProperty->GetFName();
			}
		}

		// Search on CDO
		if (FProperty* ReferencingProp = FindPropertyReferencingComponent(ComponentInstance, false))
		{
			return ReferencingProp->GetFName();
		}
		// Do a limited second search attempt using real Instance
		if (!OwnerActor->HasAnyFlags(RF_ClassDefaultObject))
		{
			if (FProperty* ReferencingProp = FindPropertyReferencingComponent(ComponentInstance, true))
			{
				return ReferencingProp->GetFName();
			}
		}
	}

	if (UActorComponent* Archetype = Cast<UActorComponent>(ComponentInstance->GetArchetype()))
	{
		if (FProperty* ReferencingProp = FindPropertyReferencingComponent(Archetype, false))
		{
			return ReferencingProp->GetFName();
		}
	}

	return NAME_None;
}

static USCS_Node* FComponentEditorUtils_FindSCSNodeForInstance(const UActorComponent* InstanceComponent,UClass* ClassToSearch)
{
	if ((ClassToSearch != nullptr) && InstanceComponent->IsCreatedByConstructionScript())
	{
		for (UClass* TestClass = ClassToSearch; TestClass->ClassGeneratedBy != nullptr; TestClass = TestClass->GetSuperClass())
		{
			if (UBlueprint* TestBP = Cast<UBlueprint>(TestClass->ClassGeneratedBy))
			{
				if (TestBP->SimpleConstructionScript != nullptr)
				{
					if (USCS_Node* Result = TestBP->SimpleConstructionScript->FindSCSNode(InstanceComponent->GetFName()))
					{
						return Result;
					}
				}
			}
		}
	}

	return nullptr;
}

FName FComponentInfo::GetVariableName() const
{
	FName VariableName = NAME_None;

	USCS_Node* SCS_Node = GetSCSNode();
	UActorComponent* ComponentTemplate = GetComponentTemplate();

	if (IsInstancedComponent() && (SCS_Node == nullptr) && (ComponentTemplate != nullptr))
	{
		if (ComponentTemplate->GetOwner())
		{
			SCS_Node = FComponentEditorUtils_FindSCSNodeForInstance(ComponentTemplate, ComponentTemplate->GetOwner()->GetClass());
		}
	}

	if (SCS_Node)
	{
		// Use the same variable name as is obtained by the compiler
		VariableName = SCS_Node->GetVariableName();
	}
	else if (ComponentTemplate)
	{
		// Try to find the component anchor variable name (first looks for an exact match then scans for any matching variable that points to the archetype in the CDO)
		VariableName = FComponentEditorUtils_FindVariableNameGivenComponentInstance(ComponentTemplate);
	}

	return VariableName;
}

FName FComponentInfo::GetObjectName() const
{
	if (UActorComponent* ComponentTemplate = GetComponentTemplate())
	{
		return ComponentTemplate->GetFName();
	}
	return NAME_None;
}

FText FComponentInfo::GetDisplayText() const
{
	FName VariableName = GetVariableName();
	UActorComponent* ComponentTemplate = GetComponentTemplate();

	UBlueprint* Blueprint = GetBlueprint();
	UClass* VariableOwner = (Blueprint != nullptr) ? Blueprint->SkeletonGeneratedClass : nullptr;
	FProperty* VariableProperty = FindFProperty<FProperty>(VariableOwner, VariableName);

	bool bHasValidVarName = (VariableName != NAME_None);

	bool bIsArrayVariable = bHasValidVarName
		&& (VariableOwner != nullptr)
		&& VariableProperty && VariableProperty->IsA<FArrayProperty>();

	FString Value;

	// Only display SCS node variable names in the tree if they have not been autogenerated
	if (bHasValidVarName && !bIsArrayVariable)
	{
		if (IsNativeComponent())
		{
			FStringFormatNamedArguments Args;
			Args.Add(TEXT("VarName"), VariableProperty && VariableProperty->IsNative() ? VariableProperty->GetDisplayNameText().ToString() : VariableName.ToString());
			Args.Add(TEXT("CompName"), ComponentTemplate->GetName());
			Value = FString::Format(TEXT("{VarName} ({CompName})"), Args);
		}
		else
		{
			Value = VariableName.ToString();
		}
	}
	else if ( ComponentTemplate != nullptr )
	{
		Value = ComponentTemplate->GetFName().ToString();
	}
	else
	{
		FString UnnamedString = LOCTEXT("UnnamedToolTip", "Unnamed").ToString();
		FString NativeString = IsNativeComponent() ? LOCTEXT("NativeToolTip", "Native ").ToString() : TEXT("");

		if (ComponentTemplate != nullptr)
		{
			Value = FString::Printf(TEXT("[%s %s%s]"), *UnnamedString, *NativeString, *ComponentTemplate->GetClass()->GetName());
		}
		else
		{
			Value = FString::Printf(TEXT("[%s %s]"), *UnnamedString, *NativeString);
		}
	}

	return FText::FromString(Value);
}

FText FComponentInfo::GetTooltipText() const
{
	FString Value;
	if (UClass* Class = GetComponentClass())
	{
		Value += FString::Printf(TEXT("Class: %s"), *Class->GetName());
	}
	return FText::FromString(Value);
}

UBlueprint* FComponentInfo::GetBlueprint() const
{
	if (const USCS_Node* SCS_Node = GetSCSNode())
	{
		if (const USimpleConstructionScript* SCS = SCS_Node->GetSCS())
		{
			return SCS->GetBlueprint();
		}
	}
	else if (const UActorComponent* ActorComponent = GetComponentTemplate())
	{
		if (const AActor* Actor = ActorComponent->GetOwner())
		{
			return UBlueprint::GetBlueprintFromClass(Actor->GetClass());
		}
	}

	return nullptr;
}


USCS_Node* FComponentInfo::GetSCSNode() const
{
	return nullptr;
}

bool FComponentInfo::IsEditorOnlyComponent() const
{
	UActorComponent* Template = GetComponentTemplate();
	return Template != nullptr && Template->bIsEditorOnly;
}

EBlueprintComponentReferenceMode FComponentInfo::GetDesiredMode() const
{
	return !GetVariableName().IsNone() ? EBlueprintComponentReferenceMode::Property : EBlueprintComponentReferenceMode::Path;
}

FString FComponentInfo::ToString() const
{
	FString FlagsString;
	if (IsNativeComponent()) FlagsString += TEXT("Native ");
	if (IsInstancedComponent()) FlagsString += TEXT("Instanced ");
	if (IsEditorOnlyComponent()) FlagsString += TEXT("Editor ");

	return FString::Printf(
			TEXT("Component ID:[%s] V:[%s] P:[%s] F:[%s] %s"),
			*GetNodeID().ToString(), *GetVariableName().ToString(), *GetObjectName().ToString(), *FlagsString, *GetDisplayText().ToString()
		);
}

FComponentInfo_Default::FComponentInfo_Default(USCS_Node* SCSNode, bool bInIsInherited) : SCSNode(SCSNode)
{
	SubobjectName = SCSNode->GetVariableName();
	Object = SCSNode->ComponentTemplate;
	ObjectClass = SCSNode->ComponentClass;
}

FComponentInfo_Default::FComponentInfo_Default(UActorComponent* Component, bool bInherited)
{
	SubobjectName = Component ? Component->GetFName() : FName();
	Object = Component;
	ObjectClass = Component ? Component->GetClass() : nullptr;

	AActor* Owner = Component->GetOwner();
	if (Owner != nullptr)
	{
		ensureMsgf(Owner->HasAnyFlags(RF_ClassDefaultObject|RF_ArchetypeObject), TEXT("Use a different node class for instanced components"));
	}
}

bool FComponentInfo_Default::IsNativeComponent() const
{
	return GetSCSNode() == nullptr && GetComponentTemplate() != nullptr;
}

USCS_Node* FComponentInfo_Default::GetSCSNode() const
{
	return SCSNode.Get();
}

FString FComponentInfo_Default::ToString() const
{
	return Super::ToString();
}

FComponentInfo_Instanced::FComponentInfo_Instanced(AActor* Owner, UActorComponent* Component)
{
	InstancedComponentName = Component->GetFName();
	InstancedComponentOwnerPtr = Owner;
	Object = Component;
	ObjectClass = Component->GetClass();
}

FName FComponentInfo_Instanced::GetVariableName() const
{
	FName BaseName = Super::GetVariableName();
	//if (BaseName.IsNone())
	//{ // not always correct, fallback to path mode
	//	return InstancedComponentName;
	//}
	return BaseName;
}

FText FComponentInfo_Instanced::GetDisplayText() const
{
	return FText::FromName(InstancedComponentName);
}

FString FComponentInfo_Instanced::ToString() const
{
	return Super::ToString();
}

// =====================================================================

FString FHierarchyInfo::ToString() const
{
	TStringBuilder<256> Buffer;
	Buffer.Appendf(TEXT("Hierarchy of %s (%s)\n"),
		*GetNameSafe(GetClassObject()), IsInstance() ? TEXT("Instance") : TEXT("Default"));
	for (auto& Node : GetNodes())
	{
		Buffer.Appendf(TEXT("%s\n"), *Node->ToString());
	}
	return Buffer.ToString();
}

void FHierarchyClassInfo::OnCompiled(class UBlueprint*)
{
	bDirty = true;
}

FHierarchyInstanceInfo::FHierarchyInstanceInfo(AActor* Actor): SourceActor(Actor)
{
	ensureAlways(::IsValid(Actor));
	SourceClass = Actor->GetClass();
	ClassDisplayText = SourceClass->GetDisplayNameText();
}

void FHierarchyInstanceInfo::OnCompiled(class UBlueprint*)
{
	bDirty = true;
}

FHierarchyClassInfo::FHierarchyClassInfo(UClass* Class) : SourceClass(Class)
{
	ensureAlways(::IsValid(Class));
	ClassDisplayText = Class->GetDisplayNameText();
}

// =====================================================================


TSharedPtr<FComponentInfo> FComponentPickerContext::FindComponent(const FBlueprintComponentReference& InRef, bool bSafeSearch)
{
	if (InRef.IsNull())
	{
		return nullptr;
	}

	// Dealing with root component magic reference
	if (FBlueprintComponentReferenceHelper::IsRootComponentReference(InRef))
	{
		return GetRoot();
	}

	auto DoesReferenceMatch = [&](const FComponentInfo& Value) -> bool
	{
		switch (InRef.GetMode())
		{
		case EBlueprintComponentReferenceMode::Property:
			return Value.GetVariableName() == InRef.GetValue();
		case EBlueprintComponentReferenceMode::Path:
			return Value.GetObjectName() == InRef.GetValue();
		default:
			return false;
		}
	};

	// Search across component hierarchy
	for (const TSharedPtr<FHierarchyInfo>& ClassDetails : ClassHierarchy)
	{
		for (const TSharedPtr<FComponentInfo>& Node : ClassDetails->GetNodes())
		{
			if (DoesReferenceMatch(*Node))
			{
				return Node;
			}
		}
	}

	// Dealing with unknown component reference
	if (bSafeSearch)
	{
		const FString SearchKey = InRef.ToString();
		if (TSharedPtr<FComponentInfo> Unknown = Unknowns.FindRef(SearchKey))
		{
			return Unknown;
		}

		auto Unknown = MakeShared<FComponentInfo_Unknown>();
		Unknown->Mode = InRef.GetMode();
		Unknown->Value = InRef.GetValue();
		Unknowns.Add(SearchKey, Unknown);
		return Unknown;
	}

	return nullptr;
}

TSharedPtr<FComponentInfo> FComponentPickerContext::FindComponentForVariable(const FName& InName)
{
	return FindComponent(FBlueprintComponentReference(EBlueprintComponentReferenceMode::Property, InName), false);
}

TSharedPtr<FComponentInfo> FComponentPickerContext::GetRoot()
{
	if (!Root.IsValid())
	{
		Root = MakeShared<FComponentInfo_Root>();
	}
	return Root;
}

UActorComponent* FComponentPickerContext::FindActualComponentTemplate(UActorComponent* Origin) const
{
	if (Actor.IsValid() && Actor->IsTemplate())
	{
		TInlineComponentArray<UActorComponent*, 32> Components;
		Actor->GetComponents(Components);

		UActorComponent** Found = Algo::FindByPredicate(Components, [&](const UActorComponent* Obj)
		{
			return Obj->GetFName() == Origin->GetFName();
		});
		return Found ? *Found : nullptr;
	}

	return Origin;
}

// =====================================================================

#undef LOCTEXT_NAMESPACE
