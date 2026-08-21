// Copyright 2024, Aquanox.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Class.h"
#include "GameFramework/Actor.h"
#include "Components/ActorComponent.h"
#include "Engine/Blueprint.h"
#include "Engine/SCS_Node.h"
#include "Templates/TypeHash.h"
#include "BlueprintComponentReference.h"
#include "Misc/EngineVersionComparison.h"

/**
 * @see FSCSEditorTreeNodeComponentBase
 */
struct BLUEPRINTCOMPONENTREFERENCEEDITOR_API FComponentInfo
{
protected:
	FName SubobjectName;
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
struct BLUEPRINTCOMPONENTREFERENCEEDITOR_API FComponentInfo_Default : public FComponentInfo
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
struct BLUEPRINTCOMPONENTREFERENCEEDITOR_API FComponentInfo_Instanced : public FComponentInfo
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

/**
 *
 */
struct BLUEPRINTCOMPONENTREFERENCEEDITOR_API FComponentInfo_Unknown : public FComponentInfo
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

/**
 *
 */
struct BLUEPRINTCOMPONENTREFERENCEEDITOR_API FComponentInfo_Root : public FComponentInfo_Unknown
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

/**
 *
 */
struct BLUEPRINTCOMPONENTREFERENCEEDITOR_API FHierarchyInfo
{
	TArray<TSharedPtr<FComponentInfo>> Nodes;
	bool bDirty = false;

	virtual ~FHierarchyInfo() = default;
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

};

/**
 *
 */
struct BLUEPRINTCOMPONENTREFERENCEEDITOR_API FHierarchyClassInfo : public FHierarchyInfo
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

/**
 *
 */
struct BLUEPRINTCOMPONENTREFERENCEEDITOR_API FHierarchyInstanceInfo : public FHierarchyInfo
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

/**
 *
 */
struct FComponentPickerGroup
{
	TSharedPtr<FHierarchyInfo>			Category;
	TArray<TSharedPtr<FComponentInfo>>	Elements;
};

/**
 * Represents component picker context
 */
struct BLUEPRINTCOMPONENTREFERENCEEDITOR_API FComponentPickerContextBase
{

};

/**
 * Legacy context with manual subobject traversing
 */
struct BLUEPRINTCOMPONENTREFERENCEEDITOR_API FComponentPickerContext : public FComponentPickerContextBase
{
	FString DebugLabel;

	TWeakObjectPtr<AActor> Actor;
	TWeakObjectPtr<UClass> Class;
	TWeakObjectPtr<UBlueprint> Blueprint;

	TArray<TSharedPtr<FHierarchyInfo>> ClassHierarchy;

	TSharedPtr<FComponentInfo> Root;
	TMap<FString, TSharedPtr<FComponentInfo>> Unknowns;

	AActor* GetActor() const { return Actor.Get(); }
	UClass* GetClass() const { return Class.Get(); }
	UBlueprint* GetBlueprint() const { return Blueprint.Get(); }

	/**
	 * Lookup for component information
	 * @param InRef Component reference to resolve
	 * @param bSafeSearch Should return instance of Unknown if no information available
	 * @return Component information
	 */
	TSharedPtr<FComponentInfo> FindComponent(const FBlueprintComponentReference& InRef, bool bSafeSearch);
	TSharedPtr<FComponentInfo> FindComponentForVariable(const FName& InName);

	TSharedPtr<FComponentInfo> GetRoot();

	UActorComponent* FindActualComponentTemplate(UActorComponent* Origin) const;
};

