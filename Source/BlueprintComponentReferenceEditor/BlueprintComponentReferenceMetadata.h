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
	// advanced filtering
	static const FName ComponentFilter;
};

/**
 * Internal struct for blueprint property configuration and view settings
 */
USTRUCT()
struct BLUEPRINTCOMPONENTREFERENCEEDITOR_API FBlueprintComponentReferenceMetadata
{
	GENERATED_BODY()
public:
	/**
	 * Enables component picker
	 */
	UPROPERTY(EditAnywhere, Category=Metadata, meta=(MDSpecifier="NoPicker", MDHandler="!Flag"))
	bool bUsePicker	= true;
	/**
	 * Enables Navigate to Component button
	 */
	UPROPERTY(EditAnywhere, Category=Metadata, meta=(MDSpecifier="NoNavigate", MDHandler="!Flag"))
	bool bUseNavigate = true;
	/**
	 * Enables Reset/Clear button
	 */
	UPROPERTY(EditAnywhere, Category=Metadata, meta=(MDSpecifier="NoClear", MDHandler="!Flag"))
	bool bUseClear	= true;

	/**
	 * Enforces specific actor class to collect components from, usually used when automatic discovery is not possible.
	 *
	 * Important note: prefer native actor classes over blueprints to avoid loading unnesessary assets
	 */
	UPROPERTY(EditAnywhere, Category=Metadata, meta=(MDSpecifier="ActorClass", MDHandler="Class", AllowAbstract=true, NoBrowse, NoCreate, DisallowCreateNew))
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

	/**
	 * ActorComponent classes or interfaces that can be referenced by this property
	 *
	 * Important note: prefer native actor classes over blueprints to avoid loading unnesessary assets
	 */
	UPROPERTY(EditAnywhere, DisplayName="Allowed Classes", Category=Metadata, NoClear, meta=(MDSpecifier="AllowedClasses", MDHandler="ClassList", DisplayThumbnail=false, NoElementDuplicate, AllowAbstract=true, NoBrowse, NoCreate, DisallowCreateNew))
	TArray<TSubclassOf<UActorComponent>>	AllowedClasses;
	/**
	 * ActorComponent classes or interfaces that can NOT be referenced by this property
	 *
	 * Important note: prefer native actor classes over blueprints to avoid loading unnesessary assets
	 */
	UPROPERTY(EditAnywhere, DisplayName="Disallowed Classes", Category=Metadata, NoClear, meta=(MDSpecifier="DisallowedClasses", MDHandler="ClassList", DisplayThumbnail=false, NoElementDuplicate, AllowAbstract=true, NoBrowse, NoCreate, DisallowCreateNew))
	TArray<TSubclassOf<UActorComponent>>	DisallowedClasses;

	DECLARE_DELEGATE_RetVal_OneParam(bool, FComponentFilterFunc, const UActorComponent*);

	/**
	 * Specifies custom component filtering function.
	 * 
	 * Must be of signature
	 * `bool MyFunction(const UActorComponent* InComponent);`
	 */
	UPROPERTY(EditAnywhere, DisplayName="Component Filter", Category=Metadata, meta=(MDSpecifier="ComponentFilter", MDHandler="String"))
	FString ComponentFilter;

public:
	virtual ~FBlueprintComponentReferenceMetadata() = default;
	virtual void ResetSettings();
	virtual void LoadSettingsFromProperty(const FProperty* InProp);
	virtual void ApplySettingsToProperty(UBlueprint* InBlueprint, FProperty* InProperty, const FName& InChanged);
};

class UBlueprint;

/**
 * An utility class that converts a typed struct container into property metadata and vise-versa and other experiments
 */
struct BLUEPRINTCOMPONENTREFERENCEEDITOR_API FMetadataMarshaller
{
	static bool HasMetaDataValue(const FProperty* Property, const FName& InName);

	static void SetMetaDataValue(UBlueprint* Blueprint, FProperty* Property, const FName& InName, TOptional<FString> InValue);

	static TOptional<FString> GetStringMetaDataValue(const FProperty* Property, const FName& InName);
	
	static TOptional<bool> GetBoolMetaDataValue(const FProperty* Property, const FName& InName);

	static void GetClassMetadata(const FProperty* Property, const FName& InName, const TFunctionRef<void(UClass*)>& Func);

	static void GetClassListMetadata(const FProperty* Property, const FName& InName, const TFunctionRef<void(UClass*)>& Func);

};
