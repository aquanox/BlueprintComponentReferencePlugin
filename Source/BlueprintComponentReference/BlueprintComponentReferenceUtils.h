// Copyright 2024, Aquanox.

#pragma once

#include "BlueprintComponentReference.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "BlueprintComponentReferenceUtils.generated.h"

namespace CRMeta
{
	inline static const FName AllowedClasses = "AllowedClasses";
	inline static const FName DisallowedClasses = "DisallowedClasses";
	inline static const FName ImplementsInterface = "ImplementsInterface";
	inline static const FName NoClear = "NoClear";
	inline static const FName NoNavigate = "NoNavigate";
	inline static const FName NoPicker = "NoPicker";
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
	 * Find component in specified actor.
	 *
	 * @param Reference
	 * @param Actor
	 * @param Component
	 * @return
	 */
	UFUNCTION(BlueprintCallable, Category="Utilities|ComponentReference", meta=( DefaultToSelf="Actor"))
	static UPARAM(DisplayName="Success") bool ResolveComponentReference(const FBlueprintComponentReference& Reference, AActor* Actor, UActorComponent*& Component);

	/**
	 * Find component of a specific type in specified actor.
	 *
	 * @param Reference
	 * @param Actor
	 * @param Class
	 * @param Component
	 * @return
	 */
	UFUNCTION(BlueprintCallable, Category="Utilities|ComponentReference", meta=(DefaultToSelf="Actor", DeterminesOutputType="Class", DynamicOutputParam="Component"))
	static UPARAM(DisplayName="Success") bool ResolveComponentReferenceOfType(const FBlueprintComponentReference& Reference, AActor* Actor, TSubclassOf<UActorComponent> Class, UActorComponent*& Component);

	/**
	 * Find component array in specified actor.
	 *
	 * @param References
	 * @param Actor
	 * @param Components
	 */
	UFUNCTION(BlueprintCallable, Category="Utilities|ComponentReference", meta=( DefaultToSelf="Actor"))
	static void ResolveComponentReferenceArray(const TArray<FBlueprintComponentReference>& References, AActor* Actor, TArray<UActorComponent*>& Components);

	/**
	 * Find component array of a specific type in specified actor.
	 *
	 * @param References
	 * @param Actor
	 * @param Class
	 * @param Components
	 */
	UFUNCTION(BlueprintCallable, Category="Utilities|ComponentReference", meta=(DefaultToSelf="Actor", DeterminesOutputType="Class", DynamicOutputParam="Components"))
	static void ResolveComponentReferenceArrayOfType(const TArray<FBlueprintComponentReference>& References, AActor* Actor, TSubclassOf<UActorComponent> Class, TArray<UActorComponent*>& Components);

	/**
	 * Does the component reference has any value set
	 *
	 * @param Reference
	 * @return
	 */
	UFUNCTION(BlueprintCallable, Category="Utilities|ComponentReference", DisplayName="Is Null")
	static bool IsNullReference(const FBlueprintComponentReference& Reference);

	/**
	 *
	 * @param Reference
	 * @param Actor
	 * @param PropertyName
	 */
	UFUNCTION(BlueprintCallable, Category="Utilities|ComponentReference", DisplayName="Set From Name")
	static void SetReferencFromName(UPARAM(Ref) FBlueprintComponentReference& Reference, AActor* Actor, FName PropertyName);

	/**
	 *
	 * @param Reference
	 * @param Actor
	 * @param ObjectPath
	 */
	UFUNCTION(BlueprintCallable, Category="Utilities|ComponentReference", DisplayName="Set From Object Path")
	static void SetReferenceFromObjectPath(UPARAM(Ref) FBlueprintComponentReference& Reference, AActor* Actor, FString ObjectPath);

	/**
	 *
	 * @param Reference
	 * @param Component
	 */
	UFUNCTION(BlueprintCallable, Category="Utilities|ComponentReference", DisplayName="Set From Component", meta=(BlueprintInternalUseOnly))
	static void SetReferenceFromComponent(UPARAM(Ref) FBlueprintComponentReference& Reference, UActorComponent* Component);
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
	UPROPERTY(EditAnywhere, Category=MetaData)
	bool bUsePicker	= true;
	/** Allow to use Navigate/Browse feature */
	UPROPERTY(EditAnywhere, Category=MetaData)
	bool bUseNavigate = true;
	/** Allow reference to be reset */
	UPROPERTY(EditAnywhere, Category=MetaData)
	bool bUseClear	= true;

	/** Allow to pick native components */
	UPROPERTY(EditAnywhere, Category=MetaData)
	bool bShowNative = true;
	/** Allow to pick blueprint components */
	UPROPERTY(EditAnywhere, Category=MetaData)
	bool bShowBlueprint = true;
	/** Allow to pick instanced components */
	UPROPERTY(EditAnywhere, Category=MetaData)
	bool bShowInstanced = false;
	/** Allow to pick path-only/hidden components */
	UPROPERTY(EditAnywhere, Category=MetaData, DisplayName="Show Hidden")
	bool bShowPathOnly = false;

	/** Classes or interfaces that can be used with this property */
	UPROPERTY(EditAnywhere, Category=MetaData, NoClear, meta=(DisplayThumbnail=false, NoElementDuplicate, AllowAbstract, NoBrowse, NoCreate, DisallowCreateNew))
	TArray<TSoftClassPtr<UActorComponent>>	AllowedClasses;
	/** Classes or interfaces that can NOT be used with this property */
	UPROPERTY(EditAnywhere, Category=MetaData, NoClear, meta=(DisplayThumbnail=false, NoElementDuplicate, AllowAbstract, NoBrowse, NoCreate, DisallowCreateNew))
	TArray<TSoftClassPtr<UActorComponent>>	DisallowedClasses;
	/** Interfaces that must be implemented to be eligible for this property */
	//UPROPERTY(EditAnywhere, Category=MetaData, meta=(DisplayThumbnail=false, NoBrowse, NoCreate))
	//TArray<TSoftClassPtr<UInterface>>		RequiredInterfaces;

};
