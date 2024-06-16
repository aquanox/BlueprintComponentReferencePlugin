// Copyright 2024, Aquanox.

#pragma once

#include "CoreMinimal.h"
#include "BlueprintComponentReference.h"
#include "Components/ActorComponent.h"
#include "Engine/Blueprint.h"
#include "Engine/SCS_Node.h"
#include "Templates/TypeHash.h"

/**
 * @see FSCSEditorTreeNodeComponentBase
 */
struct FComponentInfo
{
protected:
	TWeakObjectPtr<UActorComponent> Object;
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

	virtual bool IsNativeComponent() const { return false; }
	virtual bool IsInstancedComponent() const { return false; }
	virtual EBlueprintComponentReferenceMode GetDesiredMode() const;
};
/**
 * @see FSCSEditorTreeNodeComponent
 */
struct FComponentInfo_Default : public FComponentInfo
{
private:
	using Super = FComponentInfo;
protected:
	TWeakObjectPtr<USCS_Node> SCSNode;
	bool bIsInherited = false;
public:
	explicit FComponentInfo_Default(class USCS_Node* InSCSNode, bool bInIsInherited = false);
	explicit FComponentInfo_Default(UActorComponent* Component, bool bInIsInherited = false);

	virtual bool IsNativeComponent() const override;
	virtual USCS_Node* GetSCSNode() const override;
};
/**
 * @see FSCSEditorTreeNodeInstanceAddedComponent
 */
struct FComponentInfo_Instanced : public FComponentInfo
{
private:
	using Super = FComponentInfo;
protected:
	FName InstancedComponentName;
	TWeakObjectPtr<AActor> InstancedComponentOwnerPtr;
public:
	explicit FComponentInfo_Instanced(AActor* Owner, UActorComponent* Component);
	virtual bool IsInstancedComponent() const override { return true; }
	virtual FName GetVariableName() const override;
	virtual FText GetDisplayText() const override;
	virtual FName GetObjectName() const override { return InstancedComponentName; }
};

struct FHierarchyInfo
{
	TArray<TSharedPtr<FComponentInfo>> Nodes;

	virtual ~FHierarchyInfo() = default;
	virtual UClass* GetClassObject() const = 0;
	virtual FText GetDisplayText() const = 0;
	virtual bool IsBlueprint() const { return false; }
	virtual const TArray<TSharedPtr<FComponentInfo>>& GetNodes() const  { return Nodes; }
	virtual bool IsInstance() const { return false; }

	template<typename T = UClass>
	T* GetClass() const { return Cast<T>(GetClassObject()); }
};

struct FHierarchyInstanceInfo : public FHierarchyInfo
{
private:
	using Super = FComponentInfo;
public:
	TWeakObjectPtr<AActor> Source;

	virtual bool IsInstance() const  override { return true; }
	virtual UClass* GetClassObject() const override { return nullptr; }
	virtual FText GetDisplayText() const override { return INVTEXT("Instanced"); }
};

struct FHierarchyClassInfo : public FHierarchyInfo
{
private:
	using Super = FComponentInfo;
public:
	FHierarchyClassInfo() = default;
	virtual ~FHierarchyClassInfo();

	TWeakObjectPtr<UClass>	Source;
	FText					ClassDisplayText;
	bool					bIsBlueprint = false;
	FDelegateHandle			CompileDelegateHandle;

	virtual UClass* GetClassObject() const override { return Source.Get(); }
	virtual FText GetDisplayText() const override { return ClassDisplayText; }
	virtual bool IsBlueprint() const override { return bIsBlueprint; }
};

struct FChooserKey
{
	FName ObjectName;
	FName ClassName;

	friend bool operator==(const FChooserKey& Lhs, const FChooserKey& RHS)
	{
		return Lhs.ObjectName == RHS.ObjectName
			&& Lhs.ClassName == RHS.ClassName;
	}

	friend bool operator!=(const FChooserKey& Lhs, const FChooserKey& RHS)
	{
		return !(Lhs == RHS);
	}

	friend uint32 GetTypeHash(const FChooserKey& Arg)
	{
		return HashCombine(GetTypeHash(Arg.ObjectName), GetTypeHash(Arg.ClassName));
	}
};

struct FChooserContext  : public TSharedFromThis<FChooserContext>
{
	FString							Label;

	TWeakObjectPtr<AActor>			Actor;
	TWeakObjectPtr<UClass>			Class;
	TArray<TSharedPtr<FHierarchyInfo>>		ClassHierarchy;

	AActor* GetActor() const { return Actor.Get(); }
	UClass* GetClass() const { return Class.Get(); }

	TSharedPtr<FComponentInfo> FindComponent(const FBlueprintComponentReference& InName) const;
	TSharedPtr<FComponentInfo> FindComponent(const FName& InName) const;


};

/**
 *
 */
class FBlueprintComponentReferenceHelper
{
protected:

	FDelegateHandle OnReloadCompleteDelegateHandle;
	FDelegateHandle OnReloadReinstancingCompleteDelegateHandle;
	FDelegateHandle OnModulesChangedDelegateHandle;

	float LastCacheCleanup = 0;

	using FInstanceKey = TTuple<FName /* owner */, FName /* actor */, FName /* class */>;
	TMap<FInstanceKey, TSharedPtr<FHierarchyInstanceInfo>> InstanceCache;

	using FClassKey = TTuple<FName /* outer */, FName /* class */>;
	TMap<FClassKey, TSharedPtr<FHierarchyClassInfo>> ClassCache;

public:

	FBlueprintComponentReferenceHelper();
	~FBlueprintComponentReferenceHelper();

	TSharedRef<FChooserContext> CreateChooserContext(FString InLabel, AActor* InActor, UClass* InClass);

	void CleanupStaleData(bool bForce);

	TSharedPtr<FHierarchyInfo> GetOrCreateInstanceData(FString const& InLabel, AActor* InActor);
	TSharedPtr<FHierarchyInfo> GetOrCreateClassData(FString const& InLabel, UClass* InClass);


	/** Construct  */
	static TSharedPtr<FComponentInfo> CreateFromNode(USCS_Node* InComponentNode);
	static TSharedPtr<FComponentInfo> CreateFromInstance(UActorComponent* Component);

	/** */
	static UClass* FindClassByName(const FString& ClassName);

	/** */
	static bool GetHierarchyFromClass(const UClass* InClass, TArray<UClass*, TInlineAllocator<16>>& OutResult);

	// Tries to find a Variable that likely holding instance component.
	static FName FindVariableForInstance(const UActorComponent* InstanceComponent, UClass* ClassToSearch);

	// Tries to find a SCS node that was likely responsible for creating the specified instance component.  Note: This is not always possible to do!
	static USCS_Node* FindSCSNodeForInstance(const UActorComponent* InstanceComponent, UClass* ClassToSearch);

	static bool Matches(const FBlueprintComponentReference& InRef, const FComponentInfo& Value)
	{
		if (InRef.Mode == EBlueprintComponentReferenceMode::ObjectPath)
			return InRef.Value == Value.GetObjectName();
		if (InRef.Mode == EBlueprintComponentReferenceMode::VariableName)
			return InRef.Value == Value.GetVariableName();
		return false;
	}

private:

	void OnReloadComplete(EReloadCompleteReason ReloadCompleteReason);
	void OnReinstancingComplete();
	void OnModulesChanged(FName Name, EModuleChangeReason ModuleChangeReason);
	void OnObjectReloaded(UObject* Object);
	void OnBlueprintCompiled(UBlueprint* Blueprint, TSharedPtr<FHierarchyClassInfo> Entry);
};

DECLARE_LOG_CATEGORY_EXTERN(LogComponentReferenceEditor, Log, All);


template<typename T>
FString FlagsToString(T InValue, TMap<T, FString> const& InFlagMap)
{
	TArray<FString, TInlineAllocator<8>> Result;
	for(auto& FlagMapEntry : InFlagMap)
	{
		if ( ((uint32)InValue & (uint32)FlagMapEntry.Key) != 0 )
		{
			Result.Add(FlagMapEntry.Value);
		}
	}
	return FString::Join(Result, TEXT("|"));
}

static const TMap<EObjectFlags, FString>& GetObjectFlagsMap()
{
	static  TMap<EObjectFlags, FString> Data;
	if (Data.IsEmpty())
	{
#define ADD_FLAG(Name) Data.Add(EObjectFlags::Name, TEXT(#Name))
		ADD_FLAG(RF_NoFlags);
		ADD_FLAG(RF_Public);
		ADD_FLAG(RF_Standalone);
		ADD_FLAG(RF_MarkAsNative);
		ADD_FLAG(RF_Transactional);
		ADD_FLAG(RF_ClassDefaultObject);
		ADD_FLAG(RF_ArchetypeObject);
		ADD_FLAG(RF_Transient);
		ADD_FLAG(RF_MarkAsRootSet);
		ADD_FLAG(RF_TagGarbageTemp);
		ADD_FLAG(RF_NeedInitialization);
		ADD_FLAG(RF_NeedLoad);
		ADD_FLAG(RF_KeepForCooker);
		ADD_FLAG(RF_NeedPostLoad);
		ADD_FLAG(RF_NeedPostLoadSubobjects);
		ADD_FLAG(RF_NewerVersionExists);
		ADD_FLAG(RF_BeginDestroyed);
		ADD_FLAG(RF_FinishDestroyed);
		ADD_FLAG(RF_BeingRegenerated);
		ADD_FLAG(RF_DefaultSubObject);
		ADD_FLAG(RF_WasLoaded);
		ADD_FLAG(RF_TextExportTransient);
		ADD_FLAG(RF_LoadCompleted);
		ADD_FLAG(RF_InheritableComponentTemplate);
		ADD_FLAG(RF_DuplicateTransient);
		ADD_FLAG(RF_StrongRefOnFrame);
		ADD_FLAG(RF_NonPIEDuplicateTransient);
		ADD_FLAG(RF_WillBeLoaded);
		ADD_FLAG(RF_HasExternalPackage);
		ADD_FLAG(RF_HasPlaceholderType);
		ADD_FLAG(RF_MirroredGarbage);
		ADD_FLAG(RF_AllocatedInSharedPage);
#undef ADD_FLAG
	}
	return Data;
}