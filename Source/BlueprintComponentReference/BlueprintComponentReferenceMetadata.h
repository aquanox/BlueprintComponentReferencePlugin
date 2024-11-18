// Copyright 2024, Aquanox.

#pragma once

#include "Components/ActorComponent.h"
#include "BlueprintComponentReferenceMetadata.generated.h"

struct BLUEPRINTCOMPONENTREFERENCE_API FCRMetadataKey
{
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
	static const FName ShowPathOnly;
};

/**
 * Internal struct for blueprint property configuration
 */
USTRUCT()
struct FBlueprintComponentReferenceMetadata
{
	GENERATED_BODY()
public:
	/** Allow to use Picker feature */
	UPROPERTY(EditAnywhere, Category=Metadata)
	bool bUsePicker	= true;
	/** Allow to use Navigate/Browse feature */
	UPROPERTY(EditAnywhere, Category=Metadata)
	bool bUseNavigate = true;
	/** Allow reference to be reset */
	UPROPERTY(EditAnywhere, Category=Metadata)
	bool bUseClear	= true;

	/** Allow to pick native components */
	UPROPERTY(EditAnywhere, Category=Metadata)
	bool bShowNative = true;
	/** Allow to pick blueprint components */
	UPROPERTY(EditAnywhere, Category=Metadata)
	bool bShowBlueprint = true;
	/** Allow to pick instanced components */
	UPROPERTY(EditAnywhere, Category=Metadata)
	bool bShowInstanced = false;
	/** Allow to pick path-only/hidden components */
	UPROPERTY(EditAnywhere, Category=Metadata, DisplayName="Show Hidden")
	bool bShowPathOnly = false;

	/** Classes or interfaces that can be used with this property */
	UPROPERTY(EditAnywhere, Category=Metadata, NoClear, meta=(DisplayThumbnail=false, NoElementDuplicate, AllowAbstract, NoBrowse, NoCreate, DisallowCreateNew))
	TArray<TSoftClassPtr<UActorComponent>>	AllowedClasses;
	/** Classes or interfaces that can NOT be used with this property */
	UPROPERTY(EditAnywhere, Category=Metadata, NoClear, meta=(DisplayThumbnail=false, NoElementDuplicate, AllowAbstract, NoBrowse, NoCreate, DisallowCreateNew))
	TArray<TSoftClassPtr<UActorComponent>>	DisallowedClasses;
};
