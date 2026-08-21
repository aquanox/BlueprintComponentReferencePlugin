#include "ComponentPickerContext.h"

#include "ComponentPickerContextFactory.h"
#include "BlueprintComponentReferenceEditor.h"
#include "BlueprintComponentReferenceHelper.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Kismet2/ComponentEditorUtils.h"
#include "Misc/CoreDelegates.h"
#include "Modules/ModuleManager.h"
#include "UObject/UObjectIterator.h"
#include "UObject/Package.h"
#include "Misc/EngineVersionComparison.h"
#include "Misc/PackageName.h"
#include "HAL/IConsoleManager.h"
#include "PropertyHandle.h"
#include "BlueprintEditor.h"

static bool GBCRCacheEnabled = true;
static FAutoConsoleVariableRef BCR_CacheEnabled_Var(
	TEXT("BCR.CacheEnabled"), GBCRCacheEnabled,
	TEXT("Enable BCR caching of instance and class data")
);

TSharedPtr<FComponentPickerContext> FLegacyContextFactory::CreateChooserContext(AActor* InActor, UClass* InClass, const FString& InLabel)
{
	bInitializedAtLeastOnce = true;

	CleanupStaleData(true);

	if (!IsValid(InActor) && !IsValid(InClass))
	{ // we called from bad context that has no knowledge of owning class or blueprint
		return nullptr;
	}

	TSharedRef<FComponentPickerContext> Ctx = MakeShared<FComponentPickerContext>();
	Ctx->DebugLabel = InLabel;
	Ctx->Actor = InActor;
	Ctx->Class = InClass;
	Ctx->Blueprint = InClass->IsInBlueprint() ? Cast<UBlueprint>(InClass->ClassGeneratedBy) : nullptr;

	ActiveContexts.Emplace(InLabel, Ctx);

	UE_LOG(LogComponentReferenceEditor, Verbose, TEXT("%s Build for %s of type %s"), *Ctx->DebugLabel, *GetNameSafe(InActor), *GetNameSafe(InClass));

	if (!InActor->IsTemplate())
	{
		if (auto InstanceData = GetOrCreateInstanceData(Ctx, InLabel, InActor))
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
			if (auto ClassData = GetOrCreateClassData(Ctx, InLabel, Class))
			{
				Ctx->ClassHierarchy.Add(ClassData);
			}
		}
	}

	return Ctx;
}

TSharedPtr<FComponentInfo> FLegacyContextFactory::CreateFromNode(TSharedRef<FComponentPickerContext> InCtx, USCS_Node* InComponentNode)
{
	check(InComponentNode);
	return MakeShared<FComponentInfo_Default>(InComponentNode);
}

TSharedPtr<FComponentInfo> FLegacyContextFactory::CreateFromInstance(TSharedRef<FComponentPickerContext> InCtx, UActorComponent* InComponent)
{
	check(InComponent);

	AActor* Owner = InComponent->GetOwner();
	if (IsValid(Owner) && !Owner->IsTemplate())
	{
		return MakeShared<FComponentInfo_Instanced>(Owner, InComponent);
	}

	if (auto ActualTemplate = InCtx->FindActualComponentTemplate(InComponent))
	{
		InComponent = ActualTemplate;
	}
	return MakeShared<FComponentInfo_Default>(InComponent);
}

bool FLegacyContextFactory::GetHierarchyFromClass(const UClass* InClass, TArray<UClass*>& OutResult)
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

FName FLegacyContextFactory::FindVariableForInstance(const UActorComponent* InstanceComponent, UClass* ClassToSearch)
{
	return FComponentEditorUtils::FindVariableNameGivenComponentInstance(InstanceComponent);
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

void FLegacyContextFactory::CleanupStaleData(bool bForce)
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
void FLegacyContextFactory::MarkBlueprintCacheDirty()
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

TSharedPtr<FHierarchyInfo> FLegacyContextFactory::GetOrCreateInstanceData(TSharedRef<FComponentPickerContext> InCtx, const FString& InLabel, AActor* InActor)
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
			UE_LOG(LogComponentReferenceEditor, Verbose, TEXT("%s register INS node %s"), *InLabel, *FBlueprintComponentReferenceHelper::BuildComponentDebugInfo(Object));
			Entry->Nodes.Add(CreateFromInstance(InCtx, Object));
		}
	}

	return Entry;
}

TSharedPtr<FHierarchyInfo> FLegacyContextFactory::GetOrCreateClassData(TSharedRef<FComponentPickerContext> InCtx, const FString& InLabel, UClass* InClass)
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
	if (auto* BPClass = Cast<UBlueprintGeneratedClass>(Entry->GetClassObject()))
	{
		Entry->bIsBlueprint = true;

		for (USCS_Node* SCSNode : BPClass->SimpleConstructionScript->GetAllNodes())
		{
			UActorComponent* Template = SCSNode->GetActualComponentTemplate(BPClass);
			UE_LOG(LogComponentReferenceEditor, Verbose, TEXT("%s register BPR node %s"), *InLabel, *FBlueprintComponentReferenceHelper::BuildComponentDebugInfo(Template));

			Entry->Nodes.Add(CreateFromNode(InCtx, SCSNode));
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
	else if (auto* NtClass = Entry->GetClassObject())
	{
		TInlineComponentArray<UActorComponent*> Components;
		NtClass->GetDefaultObject<AActor>()->GetComponents(Components);

		for (UActorComponent* Object : Components)
		{
			UE_LOG(LogComponentReferenceEditor, Verbose, TEXT("%s register NAT node %s"), *InLabel, *FBlueprintComponentReferenceHelper::BuildComponentDebugInfo(Object));

			Entry->Nodes.Add(CreateFromInstance(InCtx, Object));
		}
	}

	return Entry;
}

void FLegacyContextFactory::DebugForceCleanup()
{
	CleanupStaleData(true);
}

static void DumpHierarchy(FHierarchyInfo& InHierarchy)
{
	UE_LOG(LogComponentReferenceEditor, Log, TEXT("%s"), *InHierarchy.ToString());
}

void FLegacyContextFactory::DebugDumpInstances(const TArray<FString>& Args)
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

void FLegacyContextFactory::DebugDumpClasses(const TArray<FString>& Args)
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

void FLegacyContextFactory::DebugDumpContexts(const TArray<FString> Args)
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

