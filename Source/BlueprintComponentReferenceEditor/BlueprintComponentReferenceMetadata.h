// Copyright 2024, Aquanox.

#pragma once

#include "UObject/SoftObjectPtr.h"
#include "Templates/SubclassOf.h"
#include "Components/ActorComponent.h"
#include "MetadataCore/MetadataMarshallerContainer.h"

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
	static const FName ComponentViewMode;
	// filter flags
	static const FName ShowBlueprint;
	static const FName ShowNative;
	static const FName ShowInstanced;
	static const FName ShowHidden;
	static const FName ShowEditor;
	static const FName ShowRoot;
	// advanced filtering
	static const FName ComponentFilter;
};

UENUM()
enum class EBlueprintComponentReferenceViewMode
{
	// Shortcut for default mode determined in runtime
	Default,
	// Disable picker
	Off,
	// Use menu style picker
	Menu,
	// Use table style picker (class -> components)
	Table,
};

/**
 * Internal struct for blueprint property configuration and view settings
 */
USTRUCT()
struct BLUEPRINTCOMPONENTREFERENCEEDITOR_API FBlueprintComponentReferenceMetadata : public FMetadataContainerBase
{
	GENERATED_BODY()
public:
	/**
	 * Type of component picker interface
	 */
	UPROPERTY(EditAnywhere, Category=Metadata, meta=(MDSpecifier="ComponentViewMode", MDHandler="Enum"))
	EBlueprintComponentReferenceViewMode ComponentViewMode = EBlueprintComponentReferenceViewMode::Default;
	/**
	 * Enables Navigate to Component button
	 */
	UPROPERTY(EditAnywhere, Category=Metadata, meta=(MDSpecifier="NoNavigate", MDHandler="Flag"))
	bool bDisableNavigate = false;
	/**
	 * Enables Reset/Clear button
	 */
	UPROPERTY(EditAnywhere, Category=Metadata, meta=(MDSpecifier="NoClear", MDHandler="Flag"))
	bool bDisableClear	= false;

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
	 * Allow to pick actor root component
	 * Note: type can be guaranteed only in runtime, used only as a shortcut "whatever root is" or when migrating
	 */
	UPROPERTY(EditAnywhere, Category=Metadata, DisplayName="Show Root", meta=(MDSpecifier="ShowRoot", MDHandler="Bool"))
	bool bShowRoot = false;

	/**
	 * ActorComponent classes or interfaces that can be referenced by this property
	 *
	 * Important note: prefer native actor classes over blueprints to avoid loading unnesessary assets
	 */
	UPROPERTY(EditAnywhere, DisplayName="Allowed Classes", Category=Metadata, NoClear, meta=(MDSpecifier="AllowedClasses", MDHandler="ClassList", DisplayThumbnail=false, NoElementDuplicate, AllowAbstract=true, NoBrowse, NoCreate, DisallowCreateNew))
	TArray<TSoftClassPtr<UActorComponent>>	AllowedClasses;
	/**
	 * ActorComponent classes or interfaces that can NOT be referenced by this property
	 *
	 * Important note: prefer native actor classes over blueprints to avoid loading unnesessary assets
	 */
	UPROPERTY(EditAnywhere, DisplayName="Disallowed Classes", Category=Metadata, NoClear, meta=(MDSpecifier="DisallowedClasses", MDHandler="ClassList", DisplayThumbnail=false, NoElementDuplicate, AllowAbstract=true, NoBrowse, NoCreate, DisallowCreateNew))
	TArray<TSoftClassPtr<UActorComponent>>	DisallowedClasses;

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
	virtual void ResetSettings() override;
	virtual void LoadSettings(const FMetadataSettingsSource& Source) override;
	virtual void ApplySettings(FMetadataSettingsSource& Source, const FName& InChanged) override;

	bool UsePicker() const { return ComponentViewMode != EBlueprintComponentReferenceViewMode::Off; }

};
