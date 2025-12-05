// Copyright 2024, Aquanox.

#pragma once

#include "CoreMinimal.h"
#include "BlueprintComponentReference.h"
#include "BlueprintComponentReferenceLibrary.h"
#include "BlueprintComponentReferenceMetadata.h"
#include "Components/ActorComponent.h"
#include "GameFramework/Actor.h"
#include "Engine/World.h"
#include "Engine/Blueprint.h"
#include "Engine/SCS_Node.h"
#include "Templates/TypeHash.h"
#include "Misc/EngineVersionComparison.h"

#if UE_VERSION_OLDER_THAN(5,4,0)
inline static FName GetFNameSafe(const UObject* InField)
{
	if (IsValid(InField))
	{
		return InField->GetFName();
	}
	return NAME_None;
}
#endif


/**
 * @see FSCSEditorTreeNodeComponentBase
 */
struct FComponentInfo
{
protected:
	TWeakObjectPtr<UActorComponent> Object;
	TWeakObjectPtr<UClass>			ObjectClass;
public:
	FComponentInfo() = default;
	virtual ~FComponentInfo() = default;

	virtual UActorComponent* GetComponentTemplate() const;
	virtual UClass* GetComponentClass() const;

	virtual FName GetNodeID() const;
	virtual FName GetVariableName() const;
	virtual FName GetObjectName() const;
	virtual FText GetDisplayText() const;
	virtual FText GetTooltipText() const;
	virtual UBlueprint* GetBlueprint() const;
	virtual USCS_Node* GetSCSNode() const;

	virtual bool IsUnknown() const { return false; }
	virtual bool IsBlueprintComponent() const { return !IsNativeComponent(); }
	virtual bool IsNativeComponent() const { return false; }
	virtual bool IsInstancedComponent() const { return false; }
	virtual bool IsEditorOnlyComponent() const;
	virtual EBlueprintComponentReferenceMode GetDesiredMode() const;

	virtual FString ToString() const;
	virtual bool IsValidInfo() const { return Object.IsValid() && ObjectClass.IsValid(); }
};
/**
 * @see FSCSEditorTreeNodeComponent
 */
struct FComponentInfo_Default : public FComponentInfo
{
private:
	using Super = FComponentInfo;
protected:
	TWeakObjectPtr<USCS_Node>	SCSNode;
	bool						bIsInherited = false;
public:
	explicit FComponentInfo_Default(USCS_Node* InSCSNode, bool bInIsInherited = false);
	explicit FComponentInfo_Default(UActorComponent* Component, bool bInIsInherited = false);

	virtual bool IsNativeComponent() const override;
	virtual USCS_Node* GetSCSNode() const override;

	virtual FString ToString() const override;
	virtual bool IsValidInfo() const override { return Super::IsValidInfo() && SCSNode.IsValid(); }
};
/**
 * @see FSCSEditorTreeNodeInstanceAddedComponent
 */
struct FComponentInfo_Instanced : public FComponentInfo
{
private:
	using Super = FComponentInfo;
protected:
	FName					InstancedComponentName;
	TWeakObjectPtr<AActor>	InstancedComponentOwnerPtr;
public:
	explicit FComponentInfo_Instanced(AActor* Owner, UActorComponent* Component);
	virtual bool IsInstancedComponent() const override { return true; }
	virtual FName GetVariableName() const override;
	virtual FText GetDisplayText() const override;
	virtual FName GetObjectName() const override { return InstancedComponentName; }

	virtual FString ToString() const override;
	virtual bool IsValidInfo() const override { return Super::IsValidInfo() && InstancedComponentOwnerPtr.IsValid(); }
};

struct FComponentInfo_Unknown : public FComponentInfo
{
	EBlueprintComponentReferenceMode Mode;
	FName Value;

	virtual FText GetDisplayText() const override { return FText::FromName(Value); }
	virtual UClass* GetComponentClass() const override { return UActorComponent::StaticClass(); }
	virtual UActorComponent* GetComponentTemplate() const override { return nullptr; }
	virtual FText GetTooltipText() const override { return INVTEXT("Failed to locate component information"); }
	virtual bool IsUnknown() const override { return true; }
	virtual bool IsBlueprintComponent() const override { return true; }
	virtual bool IsNativeComponent() const override { return true; }
	virtual bool IsInstancedComponent() const override { return true; }
	virtual EBlueprintComponentReferenceMode GetDesiredMode() const override { return Mode; }
	virtual FName GetVariableName() const override { return Mode == EBlueprintComponentReferenceMode::Property ? Value : NAME_None; }
	virtual FName GetObjectName() const override { return Mode == EBlueprintComponentReferenceMode::Path ? Value : NAME_None; }
};

struct FComponentInfo_Root : public FComponentInfo_Unknown
{
	FComponentInfo_Root()
	{
		Mode = EBlueprintComponentReferenceMode::Property;
		Value = TEXT("RootComponent");
	}

	virtual FText GetDisplayText() const override { return INVTEXT("Root Component (auto)"); }
	virtual FText GetTooltipText() const override { return INVTEXT("Actor Root Component (auto)"); }
	virtual UClass* GetComponentClass() const override { return USceneComponent::StaticClass(); }
	virtual UActorComponent* GetComponentTemplate() const override { return GetMutableDefault<USceneComponent>(); }
	virtual bool IsUnknown() const override { return false; }
};

struct FHierarchyInfo
{
	TArray<TSharedPtr<FComponentInfo>> Nodes;
	bool bDirty = false;
	//TMulticastDelegate<void()> Cleaner;

	virtual ~FHierarchyInfo();
	// Group items
	virtual const TArray<TSharedPtr<FComponentInfo>>& GetNodes() const  { return Nodes; }
	// Group related class object
	virtual UClass* GetClassObject() const = 0;
	// Group display name
	virtual FText GetDisplayText() const = 0;
	// Is category considered to be a blueprint
	virtual bool IsBlueprint() const { return false; }
	// Is category considered to be an instance-only
	virtual bool IsInstance() const { return false; }
	//
	virtual FString ToString() const;
	//
	virtual bool IsValidInfo() const = 0;

	template<typename T = UClass>
	T* GetClass() const { return Cast<T>(GetClassObject()); }
};

struct FHierarchyClassInfo : public FHierarchyInfo
{
private:
	using Super = FHierarchyInfo;
public:
	FHierarchyClassInfo(UClass* Class);
	virtual ~FHierarchyClassInfo() = default;

	TWeakObjectPtr<UClass>	SourceClass;
	FText					ClassDisplayText;
	bool					bIsBlueprint = false;

	virtual UClass* GetClassObject() const override { return SourceClass.Get(); }
	virtual FText GetDisplayText() const override { return ClassDisplayText; }
	virtual bool IsBlueprint() const override { return bIsBlueprint; }
	virtual bool IsValidInfo() const override { return SourceClass.IsValid(); }

	void OnCompiled(class UBlueprint*);
};

struct FHierarchyInstanceInfo : public FHierarchyInfo
{
private:
	using Super = FHierarchyInfo;
public:
	TWeakObjectPtr<AActor>  SourceActor;
	TWeakObjectPtr<UClass>	SourceClass;
	FText					ClassDisplayText;
	bool					bIsBlueprint = false;

	FHierarchyInstanceInfo(AActor* Actor);

	virtual bool IsInstance() const  override { return true; }
	virtual UClass* GetClassObject() const override { return SourceClass.Get(); }
	virtual FText GetDisplayText() const override { return INVTEXT("Instance"); }
	virtual bool IsBlueprint() const override { return bIsBlueprint; }
	virtual bool IsValidInfo() const override { return  SourceActor.IsValid() && SourceClass.IsValid(); }

	void OnCompiled(class UBlueprint*);
};

struct FComponentPickerContext
{
	FString Label;

	TWeakObjectPtr<AActor> Actor;
	TWeakObjectPtr<UClass> Class;
	TArray<TSharedPtr<FHierarchyInfo>> ClassHierarchy;

	TSharedPtr<FComponentInfo> Root;
	TMap<FString, TSharedPtr<FComponentInfo>> Unknowns;

	AActor* GetActor() const { return Actor.Get(); }
	UClass* GetClass() const { return Class.Get(); }

	/**
	 * Lookup for component information
	 * @param InRef Component reference to resolve
	 * @param bSafeSearch Should return instance of Unknown if no information available
	 * @return Component information
	 */
	TSharedPtr<FComponentInfo> FindComponent(const FBlueprintComponentReference& InRef, bool bSafeSearch);
	TSharedPtr<FComponentInfo> FindComponentForVariable(const FName& InName);

	TSharedPtr<FComponentInfo> GetRoot() /*fake*/;
};

/**
 * BCR customization manager.
 *
 * Holds internal data about hierarchies and components.
 *
 * maybe merge back to module class?
 */
class FBlueprintComponentReferenceHelper : public TSharedFromThis<FBlueprintComponentReferenceHelper>
{
public:
	using FInstanceKey = TTuple<FName /* fn */, FName /* name */, FName /* class */>;
	using FClassKey = TTuple<FName /* fn */, FName /* class */>;

	/**
	 * Test if property is supported by BCR customization
	 */
	static bool IsComponentReferenceProperty(const FProperty* InProperty);

	/**
	 * Test if type is a BCR type
	 */
	static bool IsComponentReferenceType(const UStruct* InStruct);

	/**
	 * Get or create component chooser data source for specific input parameters
	 *
	 * @param InActor Input actor
	 * @param InClass Input class
	 * @param InLabel Debug marker
	 * @return Context instance
	 */
	TSharedPtr<FComponentPickerContext> CreateChooserContext(AActor* InActor, UClass* InClass, const FString& InLabel);

	/**
	 * Cleanup stale hierarchy data
	 */
	void CleanupStaleData(bool bForce = false);

	/**
	 *
	 */
	void MarkBlueprintCacheDirty();

	/**
	 * Collect components info specific to live actor instance
	 *
	 * @param InLabel Actor label, debug purpose only
	 * @param InActor Actor instance to collect information from
	 * @return
	 */
	TSharedPtr<FHierarchyInfo> GetOrCreateInstanceData(FString const& InLabel, AActor* InActor);

	/**
	 * Collect components info specific to class
	 *
	 * @param InLabel Class label, debug purpose only
	 * @param InClass Class instance to collect information from
	 * @return
	 */
	TSharedPtr<FHierarchyInfo> GetOrCreateClassData(FString const& InLabel, UClass* InClass);

	static TSharedPtr<FComponentInfo> CreateFromNode(USCS_Node* InComponentNode);
	static TSharedPtr<FComponentInfo> CreateFromInstance(UActorComponent* Component);

	/** IS it a blueprint property or not */
	static bool IsBlueprintProperty(const FProperty* VariableProperty);

	/** */
	static UClass* FindClassByName(const FString& ClassName);

	/** */
	static bool GetHierarchyFromClass(const UClass* InClass, TArray<UClass*>& OutResult);

	// Tries to find a Variable that likely holding instance component.
	static FName FindVariableForInstance(const UActorComponent* InstanceComponent, UClass* ClassToSearch);

	// Tries to find a SCS node that was likely responsible for creating the specified instance component.  Note: This is not always possible to do!
	static USCS_Node* FindSCSNodeForInstance(const UActorComponent* InstanceComponent, UClass* ClassToSearch);

	static bool IsRootComponentReference(const FBlueprintComponentReference& InRef);

	static bool InvokeComponentFilter(TSharedPtr<class IPropertyHandle> InProperty, const FString& InFilterFn, const UObject* InObj);

	void DebugDumpInstances(const TArray<FString>& Args);
	void DebugDumpClasses(const TArray<FString>& Args);
	void DebugDumpContexts(const TArray<FString> Array);
	void DebugForceCleanup();

private:
	float		LastCacheCleanup = 0;
	bool		bInitializedAtLeastOnce = false;

	TMap<FString, TWeakPtr<FComponentPickerContext>> ActiveContexts;

	TMap<FInstanceKey, TSharedPtr<FHierarchyInstanceInfo>> InstanceCache;

	TMap<FClassKey, TSharedPtr<FHierarchyClassInfo>> ClassCache;
};
