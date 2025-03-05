// Copyright 2024, Aquanox.

#pragma once

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
 * Internal struct for blueprint property configuration and view settings
 *
 * todo: maybe make a universal mapper for ustruct-metadata view ?
 */
USTRUCT()
struct FBlueprintComponentReferenceMetadata
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
	void LoadSettingsFromProperty_Generic(const FProperty* InProp, void* TargetData, UScriptStruct* TargetType);
	void ApplySettingsToProperty(UBlueprint* InBlueprint, FProperty* InProperty, const FName& InChanged);

private:
	static bool HasMetaDataValue(const FProperty* Property, const FName& InName);
	static TOptional<bool> GetBoolMetaDataOptional(const FProperty* Property, const FName& InName);
	static bool GetBoolMetaDataValue(const FProperty* Property, const FName& InName, bool bDefaultValue);
	static void SetBoolMetaDataValue(FProperty* Property, const FName& InName, TOptional<bool> Value);
	static void GetClassMetadata(const FProperty* Property, const FName& InName, const TFunctionRef<void(UClass*)>& Func);
	static void SetClassMetadata(FProperty* Property, const FName& InName, class UClass* InClass);
	static void GetClassListMetadata(const FProperty* Property, const FName& InName, const TFunctionRef<void(UClass*)>& Func);
	static void SetClassListMetadata(FProperty* Property, const FName& InName, const TFunctionRef<void(TArray<FString>&)>& PathSource);

};
