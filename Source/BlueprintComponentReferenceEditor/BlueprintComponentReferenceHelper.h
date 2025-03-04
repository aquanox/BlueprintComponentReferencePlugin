﻿// Copyright 2024, Aquanox.

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

	virtual bool IsNativeComponent() const { return false; }
	virtual bool IsInstancedComponent() const { return false; }
	virtual bool IsEditorOnlyComponent() const;
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
	TWeakObjectPtr<USCS_Node>	SCSNode;
	bool						bIsInherited = false;
public:
	explicit FComponentInfo_Default(USCS_Node* InSCSNode, bool bInIsInherited = false);
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
	FName					InstancedComponentName;
	TWeakObjectPtr<AActor>	InstancedComponentOwnerPtr;
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

struct FComponentPickerContext
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
 * BCR customization manager.
 *
 * Holds internal data about hierarchies and components.
 *
 * maybe merge back to module class?
 */
class FBlueprintComponentReferenceHelper : public TSharedFromThis<FBlueprintComponentReferenceHelper> 
{
public:
	float LastCacheCleanup = 0;

	using FInstanceKey = TTuple<FName /* owner */, FName /* actor */, FName /* class */>;
	TMap<FInstanceKey, TSharedPtr<FHierarchyInstanceInfo>> InstanceCache;

	using FClassKey = TTuple<FName /* outer */, FName /* class */>;
	TMap<FClassKey, TSharedPtr<FHierarchyClassInfo>> ClassCache;

	FBlueprintComponentReferenceHelper();
	
	static bool IsComponentReferenceProperty(const FProperty* InProperty);
	static bool IsComponentReferenceType(const UStruct* InStruct);

	TSharedPtr<FComponentPickerContext> CreateChooserContext(AActor* InActor, UClass* InClass, const FString& InLabel);

	void CleanupStaleData(bool bForce = false);
	
	TSharedPtr<FHierarchyInfo> GetOrCreateInstanceData(FString const& InLabel, AActor* InActor);
	TSharedPtr<FHierarchyInfo> GetOrCreateClassData(FString const& InLabel, UClass* InClass);


	/** Construct  */
	static TSharedPtr<FComponentInfo> CreateFromNode(USCS_Node* InComponentNode);
	static TSharedPtr<FComponentInfo> CreateFromInstance(UActorComponent* Component);

	/** IS it a blueprint property or not */
	static bool IsBlueprintProperty(const FProperty* VariableProperty);

	/** */
	static UClass* FindClassByName(const FString& ClassName);

	/** */
	static bool GetHierarchyFromClass(const UClass* InClass, TArray<UClass*, TInlineAllocator<16>>& OutResult);

	// Tries to find a Variable that likely holding instance component.
	static FName FindVariableForInstance(const UActorComponent* InstanceComponent, UClass* ClassToSearch);

	// Tries to find a SCS node that was likely responsible for creating the specified instance component.  Note: This is not always possible to do!
	static USCS_Node* FindSCSNodeForInstance(const UActorComponent* InstanceComponent, UClass* ClassToSearch);

	static bool DoesReferenceMatch(const FBlueprintComponentReference& InRef, const FComponentInfo& Value);

private:
	void OnBlueprintCompiled(UBlueprint* Blueprint, TSharedPtr<FHierarchyClassInfo> Entry);
};
