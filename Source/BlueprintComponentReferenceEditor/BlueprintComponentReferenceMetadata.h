// Copyright 2024, Aquanox.

#pragma once

#include "UObject/SoftObjectPtr.h"
#include "Templates/SubclassOf.h"
#include "Components/ActorComponent.h"

#include "BlueprintComponentReferenceMetadata.generated.h"

struct FCRMetadataKey
{
	//
	static const FName ActorClass;
	// basic
	static const FName AllowedClasses;
	static const FName DisallowedClasses;
	// display flags
	static const FName NoClear;
	static const FName NoNavigate;
	static const FName NoPicker;
	// filter flags
	static const FName ShowBlueprint;
	static const FName ShowNative;
	static const FName ShowInstanced;
	static const FName ShowHidden;
	static const FName ShowEditor;
};

/**
 * Container type for metadata that can be loaded/applied to property using FMetadataMarshaller
 */
USTRUCT()
struct FMetadataContainerBase
{
	GENERATED_BODY()
};

/**
 * Internal struct for blueprint property configuration and view settings
 *
 */
USTRUCT()
struct FBlueprintComponentReferenceMetadata : public FMetadataContainerBase
{
	GENERATED_BODY()
public:
	/** Allow to use Picker feature */
	UPROPERTY(EditAnywhere, Category=Metadata, meta=(MDSpecifier="NoPicker", MDHandler="InverseBool"))
	bool bUsePicker	= true;
	/** Allow to use Navigate/Browse feature */
	UPROPERTY(EditAnywhere, Category=Metadata, meta=(MDSpecifier="NoNavigate", MDHandler="InverseBool"))
	bool bUseNavigate = true;
	/** Allow reference to be reset */
	UPROPERTY(EditAnywhere, Category=Metadata, meta=(MDSpecifier="NoClear", MDHandler="InverseBool"))
	bool bUseClear	= true;

	/** Enforce specific Actor class to collect components from, leave empty for automatic discovery */ 
	UPROPERTY(EditAnywhere, Category=Metadata, meta=(MDSpecifier="ActorClass", MDHandler="Class", AllowAbstract=true))
	TSoftClassPtr<AActor> ActorClass;

	/** Allow to pick native components */
	UPROPERTY(EditAnywhere, Category=Metadata, meta=(MDSpecifier="ShowNative", MDHandler="Bool"))
	bool bShowNative = true;
	/** Allow to pick blueprint components */
	UPROPERTY(EditAnywhere, Category=Metadata, meta=(MDSpecifier="ShowBlueprint", MDHandler="Bool"))
	bool bShowBlueprint = true;
	/** Allow to pick instanced components */
	UPROPERTY(EditAnywhere, Category=Metadata, meta=(MDSpecifier="ShowInstanced", MDHandler="Bool"))
	bool bShowInstanced = false;
	/** Allow to pick path-only/hidden components */
	UPROPERTY(EditAnywhere, Category=Metadata, DisplayName="Show Hidden", meta=(MDSpecifier="ShowHidden", MDHandler="Bool"))
	bool bShowHidden = false;
	/** Allow to pick editor-only components */
	UPROPERTY(EditAnywhere, Category=Metadata, DisplayName="Show Editor", meta=(MDSpecifier="ShowEditor", MDHandler="Bool"))
	bool bShowEditor = true;

	/** Classes or interfaces that can be used with this property */
	UPROPERTY(EditAnywhere, DisplayName="Allowed Classes", Category=Metadata, NoClear, meta=(MDSpecifier="AllowedClasses", MDHandler="ClassList", DisplayThumbnail=false, NoElementDuplicate, AllowAbstract, NoBrowse, NoCreate, DisallowCreateNew))
	TArray<TSubclassOf<UActorComponent>>	AllowedClasses;
	/** Classes or interfaces that can NOT be used with this property */
	UPROPERTY(EditAnywhere, DisplayName="Disallowed Classes", Category=Metadata, NoClear, meta=(MDSpecifier="DisallowedClasses", MDHandler="ClassList", DisplayThumbnail=false, NoElementDuplicate, AllowAbstract, NoBrowse, NoCreate, DisallowCreateNew))
	TArray<TSubclassOf<UActorComponent>>	DisallowedClasses;

public:

	void ResetSettings();
	void LoadSettingsFromProperty(const FProperty* InProp);
	void ApplySettingsToProperty(UBlueprint* InBlueprint, FProperty* InProperty, const FName& InChanged);
};

class UBlueprint;

/**
 * An utility class that converts a typed struct container into property metadata and vise-versa 
 */
struct FMetadataMarshaller
{
	struct FStructData // using FStructData = FStructView; 
	{
		const UScriptStruct* ScriptStruct = nullptr;
		uint8* StructMemory = nullptr;

		FStructData() = default;
		FStructData(const UScriptStruct* ScriptStruct, uint8* StructMemory)
			: ScriptStruct(ScriptStruct),
			  StructMemory(StructMemory)
		{
		}
	};

	/**
	 * Load metadata from property to container
	 * @tparam T Container type
	 * @param Container Container instance
	 * @param InProp Property to read metadata from
	 */
	template<typename T = FMetadataContainerBase>
	void LoadFromProperty(T& Container, const FProperty* InProp)
	{
		LoadInternal(FStructData(T::StaticStruct(), &Container), InProp);
	}

	/**
	 * Apply metadata from a container onto property
	 * @tparam T Container type
	 * @param Container Container instance
	 * @param InBlueprint Target blueprint. Nullable
	 * @param InProperty Target property to apply metadata.
	 * @param InChanged Name of the metadata specifier that changed for single update, optional.
	 */
	template<typename T = FMetadataContainerBase>
	void ApplyToProperty(T& Container, UBlueprint* InBlueprint, FProperty* InProperty, const FName& InChanged)
	{
		ApplyInternal(FStructData(T::StaticStruct(), &Container), InBlueprint, InProperty, InChanged);
	}

	/**
	 * Returns instance of a default metadata marshaller 
	 * @return Metadata marshaller instance 
	 */
	static FMetadataMarshaller& Get();
	
	static bool HasMetaDataValue(const FProperty* Property, const FName& InName);

	static TOptional<bool> GetBoolMetaDataOptional(const FProperty* Property, const FName& InName);
	static bool GetBoolMetaDataValue(const FProperty* Property, const FName& InName, bool bDefaultValue);
	static void SetBoolMetaDataValue(FProperty* Property, const FName& InName, TOptional<bool> Value);

	// todo: int group

	// todo: string group

	// todo: float group
	
	static void GetClassMetadata(const FProperty* Property, const FName& InName, const TFunctionRef<void(UClass*)>& Func);
	static void SetClassMetadata(FProperty* Property, const FName& InName, class UClass* InClass);
	
	static void GetClassListMetadata(const FProperty* Property, const FName& InName, const TFunctionRef<void(UClass*)>& Func);
	static void SetClassListMetadata(FProperty* Property, const FName& InName, const TFunctionRef<void(TArray<FString>&)>& PathSource);

private:
	FMetadataMarshaller();

	void LoadInternal(FStructData Container, const FProperty* InProperty);
	void ApplyInternal(FStructData Container, UBlueprint* InBlueprint, FProperty* InProperty, const FName& InChanged);

	/**
	 * @param Container Container
	 * @param ContainerProperty Property within container
	 * @param Property Property to read data from
	 * @param Name Name of metadata specifier
	 */
	using FLoaderDelegate = TDelegate<void(const FStructData& Container, const FProperty* ContainerProperty, const FProperty* Property, FName Name)>;

	/**
	 * @param Container Container to store data
	 * @param ContainerProperty Property within container to store data
	 * @param Blueprint Blueprint that owns property
	 * @param Property Property to update
	 * @param Name Name of metadata specifier
	 */
	using FApplierDelegate =  TDelegate<void(const FStructData& Container, const FProperty* ContainerProperty, UBlueprint* Blueprint, FProperty* Property, FName Name)>;
	
	struct FMetadataTypeHandler
	{
		FLoaderDelegate		Loader;
		FApplierDelegate	Applier;
	};

	TMap<FName, FMetadataTypeHandler> RegisteredHandlers; 
};
