// Copyright 2024, Aquanox.

#pragma once

#include "CoreMinimal.h"
#include "BlueprintComponentReference.h"
#include "BlueprintComponentReferenceUtils.h"
#include "Components/ActorComponent.h"
#include "Engine/Blueprint.h"
#include "Engine/SCS_Node.h"
#include "Templates/TypeHash.h"
#include "Misc/EngineVersionComparison.h"

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

	static bool IsComponentReferenceProperty(const FProperty* InProperty);
	static bool IsComponentReferenceType(const UStruct* InStruct);

	TSharedPtr<FComponentPickerContext> CreateChooserContext(AActor* InActor, UClass* InClass, const FString& InLabel);

	void CleanupStaleData(bool bForce);

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

	template<typename T>
	static void ResetSettings(T& Settings);
	template<typename T>
	static void LoadSettingsFromProperty(T& Settings, const FProperty* InProp);
	template<typename T>
	static void ApplySettingsToProperty(T& Settings, UBlueprint* InBlueprint, FProperty* InProperty, const FName& InChanged);

	static bool HasMetaDataValue(const FProperty* Property, const FName& InName);
	static TOptional<bool> GetBoolMetaDataOptional(const FProperty* Property, const FName& InName);
	static bool GetBoolMetaDataValue(const FProperty* Property, const FName& InName, bool bDefaultValue);
	static void SetBoolMetaDataValue(FProperty* Property, const FName& InName, TOptional<bool> Value);
	static void GetClassListMetadata(const FProperty* Property, const FName& InName, const TFunctionRef<void(UClass*)>& Func);
	static void SetClassListMetadata(FProperty* Property, const FName& InName, const TFunctionRef<void(TArray<FString>&)>& PathSource);

private:

	void OnReloadComplete(EReloadCompleteReason ReloadCompleteReason);
	void OnReinstancingComplete();
	void OnModulesChanged(FName Name, EModuleChangeReason ModuleChangeReason);
	void OnObjectReloaded(UObject* Object);
	void OnBlueprintCompiled(UBlueprint* Blueprint, TSharedPtr<FHierarchyClassInfo> Entry);
};

template <typename T>
void FBlueprintComponentReferenceHelper::ResetSettings(T& Settings)
{
	static const FBlueprintComponentReferenceMetadata DefaultValues;

	Settings.AllowedClasses.Empty();
	Settings.DisallowedClasses.Empty();
	//Settings.RequiredInterfaces.Empty();

	Settings.bUsePicker = DefaultValues.bUsePicker;

	Settings.bUseNavigate = DefaultValues.bUseNavigate;
	Settings.bUseClear = DefaultValues.bUseClear;

	Settings.bShowNative = DefaultValues.bShowNative;
	Settings.bShowBlueprint = DefaultValues.bShowBlueprint;
	Settings.bShowInstanced = DefaultValues.bShowInstanced;
	Settings.bShowPathOnly = DefaultValues.bShowPathOnly;
}

template <typename T>
void FBlueprintComponentReferenceHelper::LoadSettingsFromProperty(T& Settings, const FProperty* InProp)
{
	// picker
	Settings.bUsePicker = !HasMetaDataValue(InProp, CRMeta::NoPicker);
	// actions
	Settings.bUseNavigate = !HasMetaDataValue(InProp, CRMeta::NoNavigate);
	Settings.bUseClear = !(InProp->PropertyFlags & CPF_NoClear) && !HasMetaDataValue(InProp, CRMeta::NoClear);
	// filters
	Settings.bShowNative = GetBoolMetaDataValue(InProp, CRMeta::ShowNative, Settings.bShowNative);
	Settings.bShowBlueprint = GetBoolMetaDataValue(InProp, CRMeta::ShowBlueprint, Settings.bShowBlueprint);
	Settings.bShowInstanced = GetBoolMetaDataValue(InProp, CRMeta::ShowInstanced,  Settings.bShowInstanced);
	Settings.bShowPathOnly = GetBoolMetaDataValue(InProp, CRMeta::ShowPathOnly,  Settings.bShowPathOnly);

	GetClassListMetadata(InProp, CRMeta::AllowedClasses, [&](UClass* InClass)
	{
		Settings.AllowedClasses.AddUnique(InClass);
	});

	GetClassListMetadata(InProp, CRMeta::DisallowedClasses, [&](UClass* InClass)
	{
		Settings.DisallowedClasses.AddUnique(InClass);
	});

	//GetClassListMetadata(InProp, CRMeta::ImplementsInterface, [&](UClass* InClass)
	//{
	//		Settings.RequiredInterfaces.Add(InClass);
	//});
}

template <typename T>
void FBlueprintComponentReferenceHelper::ApplySettingsToProperty(T& Settings, UBlueprint* InBlueprint, FProperty* InProperty, const FName& InChanged)
{
	auto BoolToString = [](bool b)
	{
		return b ? TEXT("true") : TEXT("false");
	};

	auto ArrayToString = [](const TArray<TSoftClassPtr<UActorComponent>>& InArray)
	{
		TArray<FString> Paths;
		for (auto& Class : InArray)
		{
			if (!Class.IsNull() && Class.IsValid())
			{
				Paths.AddUnique(Class->GetClassPathName().ToString());
			}
		}
		return FString::Join(Paths, TEXT(","));
	};

	auto SetMetaData = [InProperty, InBlueprint](const FName& InName, const FString& InValue)
	{
		if (InProperty)
		{
			if (::IsValid(InBlueprint))
			{
				for (FBPVariableDescription& VariableDescription : InBlueprint->NewVariables)
				{
					if (VariableDescription.VarName == InProperty->GetFName())
					{
						if (!InValue.IsEmpty())
						{
							InProperty->SetMetaData(InName, *InValue);
							VariableDescription.SetMetaData(InName, InValue);
						}
						else
						{
							InProperty->RemoveMetaData(InName);
							VariableDescription.RemoveMetaData(InName);
						}

						InBlueprint->Modify();
					}
				}
			}
		}
	};

	auto SetMetaDataFlag = [InProperty, InBlueprint](const FName& InName, bool InValue)
	{
		if (InProperty)
		{
			if (::IsValid(InBlueprint))
			{
				for (FBPVariableDescription& VariableDescription : InBlueprint->NewVariables)
				{
					if (VariableDescription.VarName == InProperty->GetFName())
					{
						if (InValue)
						{
							InProperty->SetMetaData(InName, TEXT(""));
							VariableDescription.SetMetaData(InName, TEXT(""));
						}
						else
						{
							InProperty->RemoveMetaData(InName);
							VariableDescription.RemoveMetaData(InName);
						}

						InBlueprint->Modify();
					}
				}
			}
		}
	};

	if (InChanged.IsNone() || InChanged == GET_MEMBER_NAME_CHECKED(FBlueprintComponentReferenceMetadata, bUsePicker))
	{
		SetMetaDataFlag(CRMeta::NoPicker, !Settings.bUsePicker);
	}
	if (InChanged.IsNone() || InChanged == GET_MEMBER_NAME_CHECKED(FBlueprintComponentReferenceMetadata, bUseNavigate))
	{
		SetMetaDataFlag(CRMeta::NoNavigate, !Settings.bUseNavigate);
	}
	if (InChanged.IsNone() || InChanged == GET_MEMBER_NAME_CHECKED(FBlueprintComponentReferenceMetadata, bUseClear))
	{
		SetMetaDataFlag(CRMeta::NoClear, !Settings.bUseClear);
	}

	if (InChanged.IsNone() || InChanged == GET_MEMBER_NAME_CHECKED(FBlueprintComponentReferenceMetadata, bShowNative))
	{
		SetMetaData(CRMeta::ShowNative, BoolToString(Settings.bShowNative));
	}
	if (InChanged.IsNone() || InChanged == GET_MEMBER_NAME_CHECKED(FBlueprintComponentReferenceMetadata, bShowBlueprint))
	{
		SetMetaData(CRMeta::ShowBlueprint, BoolToString(Settings.bShowBlueprint));
	}
	if (InChanged.IsNone() || InChanged == GET_MEMBER_NAME_CHECKED(FBlueprintComponentReferenceMetadata, bShowInstanced))
	{
		SetMetaData(CRMeta::ShowInstanced, BoolToString(Settings.bShowInstanced));
	}
	if (InChanged.IsNone() || InChanged == GET_MEMBER_NAME_CHECKED(FBlueprintComponentReferenceMetadata, bShowPathOnly))
	{
		SetMetaData(CRMeta::ShowPathOnly, BoolToString(Settings.bShowPathOnly));
	}
	if (InChanged.IsNone() || InChanged == GET_MEMBER_NAME_CHECKED(FBlueprintComponentReferenceMetadata, AllowedClasses))
	{
		SetMetaData(CRMeta::AllowedClasses, ArrayToString(Settings.AllowedClasses));
	}
	if (InChanged.IsNone() || InChanged == GET_MEMBER_NAME_CHECKED(FBlueprintComponentReferenceMetadata, DisallowedClasses))
	{
		SetMetaData(CRMeta::DisallowedClasses, ArrayToString(Settings.DisallowedClasses));
	}
}
