#pragma once

#include "ComponentPickerContext.h"
#include "Templates/SharedPointer.h"

class FComponentPickerContextFactory : public TSharedFromThis<FComponentPickerContextFactory>
{
public:
	FComponentPickerContextFactory() = default;
	virtual ~FComponentPickerContextFactory() = default;

	// Construct new chooser context given the parameters
	virtual TSharedPtr<FComponentPickerContext> CreateChooserContext(AActor* InActor, UClass* InClass, const FString& InLabel) = 0;

	// Handle console command
	virtual void HandleCommand(const FName& InName, const TArray<FString>& InArgs) {}
	// Request to validate internal cache
	virtual void ValidateCache(bool bIncludeBlueprints)  {  }
};

/**
 * Holds internal data about hierarchies and components.
 */
class BLUEPRINTCOMPONENTREFERENCEEDITOR_API FLegacyContextFactory : public FComponentPickerContextFactory
{
	using FInstanceKey = TTuple<FName /* fn */, FName /* name */, FName /* class */>;
	using FClassKey = TTuple<FName /* fn */, FName /* class */>;
public:
	/**
	 * Get or create component chooser data source for specific input parameters
	 *
	 * @param InActor Input actor
	 * @param InClass Input class
	 * @param InLabel Debug marker
	 * @return Context instance
	 */
	virtual TSharedPtr<FComponentPickerContext> CreateChooserContext(AActor* InActor, UClass* InClass, const FString& InLabel) override;

	/**
	 * Collect components info specific to live actor instance
	 *
	 * @param InLabel Actor label, debug purpose only
	 * @param InActor Actor instance to collect information from
	 * @return
	 */
	TSharedPtr<FHierarchyInfo> GetOrCreateInstanceData(TSharedRef<FComponentPickerContext> InCtx, const FString& InLabel, AActor* InActor);

	/**
	 * Collect components info specific to class
	 *
	 * @param InLabel Class label, debug purpose only
	 * @param InClass Class instance to collect information from
	 * @return
	 */
	TSharedPtr<FHierarchyInfo> GetOrCreateClassData(TSharedRef<FComponentPickerContext> InCtx, const FString& InLabel, UClass* InClass);

	TSharedPtr<FComponentInfo> CreateFromNode(TSharedRef<FComponentPickerContext> InCtx, USCS_Node* InComponentNode);
	TSharedPtr<FComponentInfo> CreateFromInstance(TSharedRef<FComponentPickerContext> InCtx, UActorComponent* Component);

	static FInstanceKey MakeInstanceKey(const AActor* InActor)
	{
		FString FullKey = InActor ? FObjectPropertyBase::GetExportPath(InActor, nullptr, nullptr, PPF_None) : TEXT("");
		FName ActorKey = ::IsValid(InActor) ? InActor->GetFName() : NAME_None;
		FName ClassKey = ::IsValid(InActor) ? InActor->GetClass()->GetFName() : NAME_None;
		return FInstanceKey { FName(*FullKey), ActorKey, ClassKey };
	}

	static FClassKey MakeClassKey(const UClass* InClass)
	{
		FString FullKey = InClass ? FObjectPropertyBase::GetExportPath(InClass, nullptr, nullptr, PPF_None) : TEXT("");
		FName ClassKey = ::IsValid(InClass) ? InClass->GetFName() : NAME_None;
		return FClassKey { FName(*FullKey), ClassKey };
	}

	/** */
	bool GetHierarchyFromClass(const UClass* InClass, TArray<UClass*>& OutResult);

	// Tries to find a Variable that likely holding instance component.
	FName FindVariableForInstance(const UActorComponent* InstanceComponent, UClass* ClassToSearch);

	/**
	 * Cleanup stale hierarchy data
	 */
	void CleanupStaleData(bool bForce = false);

	/**
	 *
	 */
	void MarkBlueprintCacheDirty();

	virtual void HandleCommand(const FName& InName, const TArray<FString>& InArgs) override
	{
		if (InName == "DebugDumpInstances") DebugDumpInstances(InArgs);
		if (InName == "DebugDumpClasses") DebugDumpClasses(InArgs);
		if (InName == "DebugDumpContexts") DebugDumpContexts(InArgs);
		if (InName == "DebugForceCleanup") DebugForceCleanup();
	}

	void DebugDumpInstances(const TArray<FString>& Args);
	void DebugDumpClasses(const TArray<FString>& Args);
	void DebugDumpContexts(const TArray<FString> Array);
	void DebugForceCleanup();

	virtual void ValidateCache(bool bIncludeBlueprints)
	{
		CleanupStaleData();
		if (bIncludeBlueprints)
		{
			MarkBlueprintCacheDirty();
		}
	}
private:
	float LastCacheCleanup = 0;
	bool bInitializedAtLeastOnce = false;


	TMap<FString, TWeakPtr<FComponentPickerContext>> ActiveContexts;
	TMap<FInstanceKey, TSharedPtr<FHierarchyInstanceInfo>> InstanceCache;
	TMap<FClassKey, TSharedPtr<FHierarchyClassInfo>> ClassCache;
};

/**
 * Context backed by USubobjectDataSubsystem
 */
struct BLUEPRINTCOMPONENTREFERENCEEDITOR_API FSubsystemComponentPickerContext : public FComponentPickerContextBase
{
	// todo: implement me
};

