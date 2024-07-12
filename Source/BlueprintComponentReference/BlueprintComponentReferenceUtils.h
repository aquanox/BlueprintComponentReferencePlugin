// Copyright 2024, Aquanox.

#pragma once

#include "BlueprintComponentReference.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "BlueprintComponentReferenceUtils.generated.h"

namespace CRMeta
{
	// list of allowed classes or interfaces (at least one)
	inline static const FName AllowedClasses = "AllowedClasses";
	// list of disallowed classes or interfaces
	inline static const FName DisallowedClasses = "DisallowedClasses";
	// list of required interfaces to implement (all)
	inline static const FName ImplementsInterface = "ImplementsInterface";
	// disables clear action
	inline static const FName NoClear = "NoClear";
	// disables component selection action
	inline static const FName NoNavigate = "NoNavigate";
	// disables picker
	inline static const FName NoPicker = "NoPicker";
	// ignore SCS components
	inline static const FName ShowBlueprint = "ShowBlueprint";
	// ignore native components
	inline static const FName ShowNative = "ShowNative";
	// ignore instanced components
	inline static const FName ShowInstanced = "ShowInstanced";
	// ignore components without variables
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
	static bool ComponentReference_IsNull(const FBlueprintComponentReference& Reference);

	/**
	 *
	 * @param Reference
	 * @param Actor
	 * @param PropertyName
	 */
	UFUNCTION(BlueprintCallable, Category="Utilities|ComponentReference", DisplayName="Set From Name")
	static void ComponentReference_SetFromName(UPARAM(Ref) FBlueprintComponentReference& Reference, AActor* Actor, FName PropertyName);

	/**
	 *
	 * @param Reference
	 * @param Actor
	 * @param ObjectPath
	 */
	UFUNCTION(BlueprintCallable, Category="Utilities|ComponentReference", DisplayName="Set From Object Path")
	static void ComponentReference_SetFromObjectPath(UPARAM(Ref) FBlueprintComponentReference& Reference, AActor* Actor, FString ObjectPath);

	/**
	 *
	 * @param Reference
	 * @param Component
	 */
	UFUNCTION(BlueprintCallable, Category="Utilities|ComponentReference", DisplayName="Set From Component", meta=(BlueprintInternalUseOnly))
	static void ComponentReference_SetFromComponent(UPARAM(Ref) FBlueprintComponentReference& Reference, UActorComponent* Component);
};

/**
 * Internal struct for blueprint property configuration
 */
USTRUCT(NotBlueprintable, NotBlueprintType, meta=(HiddenByDefault))
struct FBlueprintComponentReferenceMetadata
{
	GENERATED_BODY()
public:
	/** Whether we allow to use Picker feature */
	UPROPERTY(EditAnywhere, Category=MetaData)
	bool bUsePicker	= true;
	/** Whether we allow to use Navigate/Browse feature */
	UPROPERTY(EditAnywhere, Category=MetaData)
	bool bUseNavigate = true;
	/** Whether the asset can be 'None' in this case */
	UPROPERTY(EditAnywhere, Category=MetaData)
	bool bUseClear	= true;

	/** Whether we allow to pick native components */
	UPROPERTY(EditAnywhere, Category=MetaData)
	bool bShowNative = true;
	/** Whether we allow to pick blueprint components */
	UPROPERTY(EditAnywhere, Category=MetaData)
	bool bShowBlueprint = true;
	/** Whether we allow to pick instanced components */
	UPROPERTY(EditAnywhere, Category=MetaData)
	bool bShowInstanced = false;
	/** Whether we allow to pick path-only components */
	UPROPERTY(EditAnywhere, Category=MetaData, DisplayName="Show Hidden")
	bool bShowPathOnly = false;


	/** Classes that can be used with this property */
	UPROPERTY(EditAnywhere, Category=MetaData, meta=(DisplayThumbnail=false, NoBrowse, NoCreate))
	TArray<TSoftClassPtr<UActorComponent>>	AllowedClasses;
	/** Classes that can NOT be used with this property */
	UPROPERTY(EditAnywhere, Category=MetaData, meta=(DisplayThumbnail=false, NoBrowse, NoCreate))
	TArray<TSoftClassPtr<UActorComponent>>	DisallowedClasses;
	/** Interfaces that must be implemented to be eligible for this property */
	//UPROPERTY(EditAnywhere, Category=MetaData, meta=(DisplayThumbnail=false, NoBrowse, NoCreate))
	//TArray<TSoftClassPtr<UInterface>>		RequiredInterfaces;

};
