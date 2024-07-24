// Copyright 2024, Aquanox.

#pragma once

#include "BlueprintComponentReference.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "BlueprintComponentReferenceUtils.generated.h"

namespace CRMeta
{
	// basic
	inline static const FName AllowedClasses = "AllowedClasses";
	inline static const FName DisallowedClasses = "DisallowedClasses";
	// filters
	inline static const FName MustImplement = "MustImplement";
	inline static const FName MustHaveTag = "RequiredTag";
	inline static const FName ComponentFilter = "ComponentFilter";
	// display flags
	inline static const FName NoClear = "NoClear";
	inline static const FName NoNavigate = "NoNavigate";
	inline static const FName NoPicker = "NoPicker";
	// filter flags
	inline static const FName ShowBlueprint = "ShowBlueprint";
	inline static const FName ShowNative = "ShowNative";
	inline static const FName ShowInstanced = "ShowInstanced";
	inline static const FName ShowPathOnly = "ShowPathOnly";
}

/**
 * Various helper functions to interact with blueprint components
 */
UCLASS(MinimalAPI)
class UBlueprintComponentReferenceUtils : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	/**
	 * Resolve component reference in specified actor.
	 *
	 * @param Reference Reference to resolve
	 * @param Actor Target actor
	 * @param Component Resolved component
	 * @return True if component found, False otherwise
	 */
	UFUNCTION(BlueprintPure, Category="Utilities|ComponentReference", meta=( DefaultToSelf="Actor"))
	static UPARAM(DisplayName="Success") bool GetReferencedComponent(const FBlueprintComponentReference& Reference, AActor* Actor, UActorComponent*& Component);

	/**
	 * Resolve component reference in specified actor.
	 *
	 * @param Reference Reference to resolve
	 * @param Actor Target actor
	 * @param Class Expected component class
	 * @param Component Resolved component
	 * @return True if component found, False otherwise
	 */
	UFUNCTION(BlueprintPure, Category="Utilities|ComponentReference", meta=( DefaultToSelf="Actor",  DeterminesOutputType="Class", DynamicOutputParam="Component"))
	static UPARAM(DisplayName="Success") bool GetReferencedComponentOfType(const FBlueprintComponentReference& Reference, AActor* Actor, TSubclassOf<UActorComponent> Class, UActorComponent*& Component);


	/**
	 * Resolve component reference in specified actor.
	 *
	 * @param Reference Reference to resolve
	 * @param Actor Target actor
	 * @param Component Resolved component
	 * @return True if component found, False otherwise
	 */
	UFUNCTION(BlueprintCallable, Category="Utilities|ComponentReference", meta=( DefaultToSelf="Actor"))
	static UPARAM(DisplayName="Success") bool TryGetReferencedComponent(const FBlueprintComponentReference& Reference, AActor* Actor, UActorComponent*& Component);

	/**
	 * Resolve component reference in specified actor.
	 *
	 * @param Reference Reference to resolve
	 * @param Actor Target actor
	 * @param Class Expected component class
	 * @param Component Resolved component
	 * @return True if component found, False otherwise
	 */
	UFUNCTION(BlueprintCallable, Category="Utilities|ComponentReference", meta=(DefaultToSelf="Actor", DeterminesOutputType="Class", DynamicOutputParam="Component"))
	static UPARAM(DisplayName="Success") bool TryGetReferencedComponentOfType(const FBlueprintComponentReference& Reference, AActor* Actor, TSubclassOf<UActorComponent> Class, UActorComponent*& Component);

	/**
	 * Resolve array of component references in specific actor
	 *
	 * @param References References to resolve
	 * @param Actor Target actor
	 * @param Components Resolved components
	 * @param bAllowNull Allow adding nulls if resolve failed
	 */
	UFUNCTION(BlueprintCallable, Category="Utilities|ComponentReference", meta=( DefaultToSelf="Actor", bAllowNull = false))
	static void TryGetReferencedComponents(const TArray<FBlueprintComponentReference>& References, AActor* Actor, bool bAllowNull, TArray<UActorComponent*>& Components);

	/**
	 * Resolve array of component references in specific actor
	 *
	 * @param References References to resolve
	 * @param Actor Target actor
	 * @param Class Expected component class
	 * @param Components Resolved components
	 * @param bAllowNull Allow adding nulls if resolve failed
	 */
	UFUNCTION(BlueprintCallable, Category="Utilities|ComponentReference", meta=(DefaultToSelf="Actor", bAllowNull = false, DeterminesOutputType="Class", DynamicOutputParam="Components"))
	static void TryGetReferencedComponentsOfType(const TArray<FBlueprintComponentReference>& References, AActor* Actor, TSubclassOf<UActorComponent> Class, bool bAllowNull, TArray<UActorComponent*>& Components);

	/**
	 * Does the component reference has any value set
	 *
	 * @param Reference Input reference
	 * @return True if reference has any value set
	 */
	UFUNCTION(BlueprintCallable, Category="Utilities|ComponentReference", DisplayName="Is Null")
	static bool IsNullReference(const FBlueprintComponentReference& Reference);

	/**
	 * Make a literal component reference
	 *
	 * @param Mode Referencing mode
	 * @param Value Search term
	 * @return constructed reference
	 */
	//UFUNCTION(BlueprintPure, Category="Utilities|ComponentReference", meta=(BlueprintThreadSafe, Mode = "EBlueprintComponentReferenceMode::VariableName"))
	static FBlueprintComponentReference MakeLiteralComponentReference(EBlueprintComponentReferenceMode Mode, FName Value);

	/**
	 * Break a literal component reference into parts
	 *
	 * @param Reference Reference
	 * @param Mode Referencing mode
	 * @param Value Search term
	 */
	//UFUNCTION(BlueprintPure, Category="Utilities|ComponentReference", meta=(BlueprintThreadSafe))
	static void BreakLiteralComponentReference(const FBlueprintComponentReference& Reference, EBlueprintComponentReferenceMode& Mode, FName& Value);

	/** Returns true if the values are equal (A == B) */
	UFUNCTION(BlueprintPure, meta=(DisplayName="Equal (ComponentReference)", CompactNodeTitle="==", BlueprintThreadSafe), Category="Utilities|ComponentReference")
	static bool EqualEqual_ComponentReference( const FBlueprintComponentReference& A, const FBlueprintComponentReference& B );

	/** Returns true if the values are not equal (A != B) */
	UFUNCTION(BlueprintPure, meta=(DisplayName="Not Equal (ComponentReference)", CompactNodeTitle="!=", BlueprintThreadSafe), Category="Utilities|ComponentReference")
	static bool NotEqual_ComponentReference( const FBlueprintComponentReference& A, const FBlueprintComponentReference& B );

	/** Convert reference to readable string */
	UFUNCTION(BlueprintPure, Category="Utilities|ComponentReference", meta=(DisplayName = "To String (ComponentReference)", CompactNodeTitle = "->", BlueprintAutocast))
	static FString Conv_ComponentReferenceToString(const FBlueprintComponentReference& Reference);
};

/**
 * Internal struct for blueprint property configuration
 */
USTRUCT(NotBlueprintable, NotBlueprintType, meta=(HiddenByDefault))
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
	/** Interfaces that must be implemented to be eligible for this property */
	//UPROPERTY(EditAnywhere, Category=Metadata, meta=(DisplayThumbnail=false, NoBrowse, NoCreate))
	//TSoftClassPtr<UInterface>		MustImplement;
	/** Interfaces have specific tag */
	//UPROPERTY(EditAnywhere, Category=Metadata)
	//FGameplayTag					MustHaveTag;
	/** Filtered interfaces will trigger external filter function */
	//UPROPERTY(EditAnywhere, Category=Metadata)
	//FName					ComponentFilter;

};
