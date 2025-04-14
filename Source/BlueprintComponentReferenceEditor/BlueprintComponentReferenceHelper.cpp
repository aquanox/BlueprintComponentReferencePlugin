// Copyright 2024, Aquanox.

#include "BlueprintComponentReferenceHelper.h"

#include "BlueprintComponentReferenceEditor.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Kismet2/ComponentEditorUtils.h"
#include "Misc/CoreDelegates.h"
#include "Modules/ModuleManager.h"
#include "UObject/UObjectIterator.h"
#include "UObject/PropertyOptional.h"
#include "Misc/EngineVersionComparison.h"
#include "HAL/IConsoleManager.h"

#define LOCTEXT_NAMESPACE "BlueprintComponentReference"

static bool GBCRCacheEnabled = true;

#if ALLOW_CONSOLE
static FAutoConsoleVariableRef BCR_CacheEnabled_Var(
	TEXT("BCR.CacheEnabled"), GBCRCacheEnabled,
	TEXT("Enable BCR caching of instance and class data")
);
#endif

static FBlueprintComponentReferenceHelper::FInstanceKey MakeInstanceKey(const AActor* InActor)
{
	FString FullKey = InActor ? FObjectPropertyBase::GetExportPath(InActor) : TEXT("");
	return FBlueprintComponentReferenceHelper::FInstanceKey { FName(*FullKey), GetFNameSafe(InActor), GetFNameSafe(InActor->GetClass()) };
}

static FBlueprintComponentReferenceHelper::FClassKey MakeClassKey(const UClass* InClass)
{
	FString FullKey = InClass ? FObjectPropertyBase::GetExportPath(InClass) : TEXT("");
	return FBlueprintComponentReferenceHelper::FClassKey { FName(*FullKey), GetFNameSafe(InClass) };
}

inline static FString BuildComponentInfo(const UActorComponent* Obj)
{
	TStringBuilder<256> Base;
	Base.Appendf(TEXT("%p:%s %s"), Obj, *Obj->GetName(), *Obj->GetClass()->GetName());
	Base.Appendf(TEXT(" Flags=%d"), Obj->GetFlags());
	Base.Appendf(TEXT(" Method=%s"), *StaticEnum<EComponentCreationMethod>()->GetNameStringByValue((int64)Obj->CreationMethod));
	return Base.ToString();
}

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

FName FComponentInfo::GetVariableName() const
{
	FName VariableName = NAME_None;

	USCS_Node* SCS_Node = GetSCSNode();
	UActorComponent* ComponentTemplate = GetComponentTemplate();

	if (IsInstancedComponent() && (SCS_Node == nullptr) && (ComponentTemplate != nullptr))
	{
		if (ComponentTemplate->GetOwner())
		{
			SCS_Node = FBlueprintComponentReferenceHelper::FindSCSNodeForInstance(ComponentTemplate, ComponentTemplate->GetOwner()->GetClass());
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
	Object = SCSNode->ComponentTemplate;
	ObjectClass = SCSNode->ComponentClass;
}

FComponentInfo_Default::FComponentInfo_Default(UActorComponent* Component, bool bInherited)
{
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

FHierarchyInfo::~FHierarchyInfo()
{
	//Cleaner.Broadcast();
}

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

UClass* FBlueprintComponentReferenceHelper::FindClassByName(const FString& ClassName)
{
	if (ClassName.IsEmpty())
		return nullptr;

	UClass* Class =  UClass::TryFindTypeSlow<UClass>(ClassName, EFindFirstObjectOptions::EnsureIfAmbiguous);
	if (!Class)
	{
		Class = LoadObject<UClass>(nullptr, *ClassName);
	}
	return Class;
}

bool FBlueprintComponentReferenceHelper::GetHierarchyFromClass(const UClass* InClass, TArray<UClass*>& OutResult)
{
	OutResult.Reset();

	bool bNoErrors = true;
	UClass* CurrentClass = const_cast<UClass*>(InClass);
	while (CurrentClass)
	{
		OutResult.Add(CurrentClass);

		if (CurrentClass == AActor::StaticClass())
			break;

		if (UBlueprintGeneratedClass* CurrentBlueprintClass = Cast<UBlueprintGeneratedClass>(CurrentClass))
		{
			UBlueprint* BP = UBlueprint::GetBlueprintFromClass(CurrentBlueprintClass);

			if (BP)
			{
				bNoErrors &= (BP->Status != BS_Error);
			}

			// If valid, use stored ParentClass rather than the actual UClass::GetSuperClass(); handles the case when the class has not been recompiled yet after a reparent operation.
			if (BP && BP->ParentClass)
			{
				CurrentClass = Cast<UClass>(BP->ParentClass);
			}
			else
			{
				check(CurrentClass);
				CurrentClass = CurrentClass->GetSuperClass();
			}
		}
		else
		{
			check(CurrentClass);
			CurrentClass = CurrentClass->GetSuperClass();
		}
	}
	return bNoErrors;
}

FName FBlueprintComponentReferenceHelper::FindVariableForInstance(const UActorComponent* InstanceComponent, UClass* ClassToSearch)
{
	return FComponentEditorUtils::FindVariableNameGivenComponentInstance(InstanceComponent);
}

USCS_Node* FBlueprintComponentReferenceHelper::FindSCSNodeForInstance(const UActorComponent* InstanceComponent,UClass* ClassToSearch)
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

bool FBlueprintComponentReferenceHelper::DoesReferenceMatch(const FBlueprintComponentReference& InRef, const FComponentInfo& Value)
{
	switch (InRef.Mode)
	{
	case EBlueprintComponentReferenceMode::Property:
		return Value.GetVariableName() == InRef.Value;
	case EBlueprintComponentReferenceMode::Path:
		return Value.GetObjectName() == InRef.Value;
	default:
		return false;
	}
}

TSharedPtr<FComponentInfo> FComponentPickerContext::FindComponent(const FBlueprintComponentReference& InRef, bool bSafeSearch) const
{
	if (InRef.IsNull())
	{
		return nullptr;
	}

	// Search across component hierarchy
	for (const auto& ClassDetails : ClassHierarchy)
	{
		for (const auto& Node : ClassDetails->GetNodes())
		{
			if ((FBlueprintComponentReferenceHelper::DoesReferenceMatch(InRef, *Node)))
			{
				return Node;
			}
		}
	}
	
	// Dealing with unknown component reference
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

TSharedPtr<FComponentInfo> FComponentPickerContext::FindComponentForVariable(const FName& InName) const
{
	return FindComponent(FBlueprintComponentReference(EBlueprintComponentReferenceMode::Property, InName), false);
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
		bDoesMatch = IsComponentReferenceProperty(AsMap->ValueProp);
	}

	return bDoesMatch;
}

bool FBlueprintComponentReferenceHelper::IsComponentReferenceType(const UStruct* InStruct)
{
	// todo: should handle child structs?
	return InStruct == FBlueprintComponentReference::StaticStruct();
}

TSharedPtr<FComponentPickerContext> FBlueprintComponentReferenceHelper::CreateChooserContext(AActor* InActor, UClass* InClass, const FString& InLabel)
{
	bInitializedAtLeastOnce = true;

	CleanupStaleData(true);

	if (!IsValid(InActor) && !IsValid(InClass))
	{ // we called from bad context that has no knowledge of owning class or blueprint
		return nullptr;
	}

	TSharedRef<FComponentPickerContext> Ctx = MakeShared<FComponentPickerContext>();
	Ctx->Label = InLabel;
	Ctx->Actor = InActor;
	Ctx->Class = InClass;

	ActiveContexts.Emplace(InLabel, Ctx);

	UE_LOG(LogComponentReferenceEditor, Verbose, TEXT("%s Build for %s of type %s"), *Ctx->Label, *GetNameSafe(InActor), *GetNameSafe(InClass));

	if (!InActor->IsTemplate())
	{
		if (auto InstanceData = GetOrCreateInstanceData(InLabel, InActor))
		{
			Ctx->ClassHierarchy.Add(InstanceData);
		}
	}

	/**
	 * Record class hierarchy recursively.
 	 */
	{
		TArray<UClass*> Classes;
		GetHierarchyFromClass(Ctx->Class.Get(), Classes);

		for (UClass* Class : Classes)
		{
			if (auto ClassData = GetOrCreateClassData(InLabel, Class))
			{
				Ctx->ClassHierarchy.Add(ClassData);
			}
		}
	}

	return Ctx;
}

template<typename Map>
inline void CleanupStaleDataImpl(Map& InMap)
{
	for(auto It = InMap.CreateIterator(); It; ++It)
	{
		if (!It->Value.IsValid() || !It->Value->IsValidInfo())
		{
			It.RemoveCurrent();
			continue;
		}

		bool bHasGoneBad = false;
		for (auto& Ptr : It->Value->Nodes)
		{
			if (!Ptr.IsValid() || !Ptr->IsValidInfo())
			{
				bHasGoneBad = true;
				break;
			}
		}

		if (bHasGoneBad)
		{
			It.RemoveCurrent();
			break;
		}
	}
}

void FBlueprintComponentReferenceHelper::CleanupStaleData(bool bForce)
{
	if (!bForce && (FPlatformTime::Seconds() - LastCacheCleanup ) < 0.2f)
	{
		return;
	}

	for (auto It = ActiveContexts.CreateIterator(); It; ++It)
	{
		if (!It->Value.IsValid())
		{
			It.RemoveCurrent();
			continue;
		}
	}

	if (GBCRCacheEnabled && bInitializedAtLeastOnce)
	{
		CleanupStaleDataImpl(InstanceCache);
		CleanupStaleDataImpl(ClassCache);
	}

	LastCacheCleanup = FPlatformTime::Seconds();
}

/**
 * mark all blueprint related data as dirty and be recreated on next access
 */
void FBlueprintComponentReferenceHelper::MarkBlueprintCacheDirty()
{
	if (GBCRCacheEnabled && bInitializedAtLeastOnce)
	{
		for (auto& Pair : InstanceCache)
		{
			if (Pair.Value.IsValid() && Pair.Value->IsBlueprint())
			{
				Pair.Value->bDirty = true;
			}
		}

		for (auto& Pair : ClassCache)
		{
			if (Pair.Value.IsValid() && Pair.Value->IsBlueprint())
			{
				Pair.Value->bDirty = true;
			}
		}
	}
}


TSharedPtr<FHierarchyInfo> FBlueprintComponentReferenceHelper::GetOrCreateInstanceData(FString const& InLabel, AActor* InActor)
{
	// disabled due to problems tracking level editor actor change in a simple way
	constexpr bool bEnableInstanceDataCache = false;

	ensureAlways(!InActor->IsTemplate());

	TSharedPtr<FHierarchyInstanceInfo>  Entry;

	if (GBCRCacheEnabled && bEnableInstanceDataCache)
	{
		const FInstanceKey EntryKey = MakeInstanceKey(InActor);

		if (auto* FoundExisting = InstanceCache.Find(EntryKey))
		{
			Entry = *FoundExisting;
			if (!Entry->bDirty)
			{
				return Entry;
			}
		}
		// Create fresh entry
		Entry = InstanceCache.Emplace(EntryKey, MakeShared<FHierarchyInstanceInfo>(InActor));
	}
	else
	{
		Entry = MakeShared<FHierarchyInstanceInfo>(InActor);
	}

	check(Entry.IsValid());

	if (UBlueprintGeneratedClass* BP = Cast<UBlueprintGeneratedClass>(InActor->GetClass()))
	{
		Entry->bIsBlueprint = true;

		if (GBCRCacheEnabled && bEnableInstanceDataCache)
		{ // track blueprint for modifications
			if (UBlueprint* BPA = Cast<UBlueprint>(BP->ClassGeneratedBy))
			{
				BPA->OnCompiled().AddSP(Entry.ToSharedRef(), &FHierarchyInstanceInfo::OnCompiled);
			}
		}

		// todo: need find a way to track level actor change
	}

	TInlineComponentArray<UActorComponent*> Components;
	InActor->GetComponents(Components);

	for (UActorComponent* Object : Components)
	{
		if (Object->CreationMethod == EComponentCreationMethod::Instance)
		{
			UE_LOG(LogComponentReferenceEditor, Verbose, TEXT("%s register INS node %s"), *InLabel, *BuildComponentInfo(Object));
			Entry->Nodes.Add(CreateFromInstance(Object));
		}
	}

	return Entry;
}

TSharedPtr<FHierarchyInfo> FBlueprintComponentReferenceHelper::GetOrCreateClassData(FString const& InLabel, UClass* InClass)
{
	ensureAlways(::IsValid(InClass));

	TSharedPtr<FHierarchyClassInfo>  Entry;

	if (GBCRCacheEnabled)
	{
		const FClassKey EntryKey = MakeClassKey(InClass);

		if (auto* FoundExisting = ClassCache.Find(EntryKey))
		{
			Entry = *FoundExisting;
			if (!Entry->bDirty)
			{
				return Entry;
			}
		}
		// Create fresh entry instead of reusing existing one, old delegate regs will be invalid
		Entry = ClassCache.Emplace(EntryKey, MakeShared<FHierarchyClassInfo>(InClass));
	}
	else
	{
		Entry = MakeShared<FHierarchyClassInfo>(InClass);
	}

	check(Entry.IsValid());

	/**
	 * If we looking a blueprint - skim its construction script for components
	 */
	if (auto* BPClass = Entry->GetClass<UBlueprintGeneratedClass>())
	{
		Entry->bIsBlueprint = true;

		for(USCS_Node* SCSNode : BPClass->SimpleConstructionScript->GetAllNodes())
		{
			auto Template = SCSNode->GetActualComponentTemplate(BPClass);
			UE_LOG(LogComponentReferenceEditor, Verbose, TEXT("%s register BPR node %s"), *InLabel, *BuildComponentInfo(Template));

			Entry->Nodes.Add(CreateFromNode(SCSNode));
		}

		if (GBCRCacheEnabled)
		{ // track blueprint changes to refresh related information
			if (UBlueprint* BPA = Cast<UBlueprint>(BPClass->ClassGeneratedBy))
			{
				BPA->OnCompiled().AddSP(Entry.ToSharedRef(), &FHierarchyClassInfo::OnCompiled);
			}
		}
	}
	/**
	 * If we looking a native class - look in default subobjects
	 */
	else if (auto* NtClass = Entry->GetClass<UClass>())
	{
		TInlineComponentArray<UActorComponent*> Components;
		NtClass->GetDefaultObject<AActor>()->GetComponents(Components);

		for (UActorComponent* Object : Components)
		{
			UE_LOG(LogComponentReferenceEditor, Verbose, TEXT("%s register NAT node %s"), *InLabel, *BuildComponentInfo(Object));

			Entry->Nodes.Add(CreateFromInstance(Object));
		}
	}

	return Entry;
}

TSharedPtr<FComponentInfo> FBlueprintComponentReferenceHelper::CreateFromNode(USCS_Node* InComponentNode)
{
	check(InComponentNode);
	return MakeShared<FComponentInfo_Default>(InComponentNode);
}

TSharedPtr<FComponentInfo> FBlueprintComponentReferenceHelper::CreateFromInstance(UActorComponent* InComponent)
{
	check(InComponent);

	AActor* Owner = InComponent->GetOwner();
	if (IsValid(Owner) && !Owner->IsTemplate())
	{
		return MakeShared<FComponentInfo_Instanced>(Owner, InComponent);
	}

	return MakeShared<FComponentInfo_Default>(InComponent);
}

bool FBlueprintComponentReferenceHelper::IsBlueprintProperty(const FProperty* VariableProperty)
{
	if(UClass* const VarSourceClass = VariableProperty ? VariableProperty->GetOwner<UClass>() : nullptr)
	{
		return (VarSourceClass->ClassGeneratedBy != nullptr);
	}
	return false;
}

void FBlueprintComponentReferenceHelper::DebugForceCleanup()
{
	CleanupStaleData(true);
}

static void DumpHierarchy(FHierarchyInfo& InHierarchy)
{
	UE_LOG(LogComponentReferenceEditor, Log, TEXT("%s"), *InHierarchy.ToString());
}

void FBlueprintComponentReferenceHelper::DebugDumpInstances(const TArray<FString>& Args)
{
	if (Args.Num() == 0)
	{
		for (auto& InstanceCacheEntry : InstanceCache)
		{
			auto& Key = InstanceCacheEntry.Key;
			UE_LOG(LogComponentReferenceEditor, Log, TEXT("Instance [%s %s %s]"),
				*Key.Get<0>().ToString(), *Key.Get<1>().ToString(), *Key.Get<2>().ToString());
		}
	}
	else if (Args.Num() == 1)
	{
		FName Selector = *Args[0];
		for (auto& CacheEntry : InstanceCache)
		{
			auto& Key = CacheEntry.Key;
			if (Key.Get<1>() == Selector || Key.Get<2>() == Selector)
			{
				FString Dump = CacheEntry.Value->ToString();
				UE_LOG(LogComponentReferenceEditor, Log, TEXT("Instance [%s %s %s]:\n%s"),
						*Key.Get<0>().ToString(), *Key.Get<1>().ToString(), *Key.Get<2>().ToString(), *Dump);

				DumpHierarchy(*CacheEntry.Value);
			}
		}
	}
}

void FBlueprintComponentReferenceHelper::DebugDumpClasses(const TArray<FString>& Args)
{
	if (Args.Num() == 0)
	{
		for (auto& CacheEntry : ClassCache)
		{
			auto& Key = CacheEntry.Key;
			UE_LOG(LogComponentReferenceEditor, Log, TEXT("Class [%s %s]"), *Key.Get<0>().ToString(), *Key.Get<1>().ToString());
		}
	}
	else if (Args.Num() == 1)
	{
		FName Selector = *Args[0];
		for (auto& CacheEntry : ClassCache)
		{
			auto& Key = CacheEntry.Key;
			if (Key.Get<1>() == Selector)
			{
				FString Dump = CacheEntry.Value->ToString();
				UE_LOG(LogComponentReferenceEditor, Log, TEXT("Class [%s %s]:\n%s"), *Key.Get<0>().ToString(), *Key.Get<1>().ToString(), *Dump);

				DumpHierarchy(*CacheEntry.Value);
			}
		}
	}
}

void FBlueprintComponentReferenceHelper::DebugDumpContexts(const TArray<FString> Args)
{
	if (Args.Num() == 0)
	{
		for (auto& CacheEntry : ActiveContexts)
		{
			auto& Key = CacheEntry.Key;
			UE_LOG(LogComponentReferenceEditor, Log, TEXT("Context [%s]"), *Key);
		}
	}
	else if (Args.Num() == 1)
	{
		for (auto& CacheEntry : ActiveContexts)
		{
			auto& Key = CacheEntry.Key;
			if (!Key.Contains(Args[0]))
				continue;

			if (CacheEntry.Value.IsValid())
			{
				auto Pinned = CacheEntry.Value.Pin();

				UE_LOG(LogComponentReferenceEditor, Log, TEXT("Context [%s]"), *Key);

                for (const TSharedPtr<FHierarchyInfo>& ErrorHist : Pinned->ClassHierarchy)
                {
                	DumpHierarchy(*ErrorHist);
                }
			}
		}
	}
}


#undef LOCTEXT_NAMESPACE
