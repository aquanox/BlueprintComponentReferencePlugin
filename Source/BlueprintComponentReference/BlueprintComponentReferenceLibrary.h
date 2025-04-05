// Copyright 2024, Aquanox.

#pragma once

#include "BlueprintComponentReference.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Components/ActorComponent.h"
#include "BlueprintComponentReferenceLibrary.generated.h"

UENUM()
enum class EComponentSearchResult : uint8
{
	Found,
	NotFound
};

/**
 * Helper functions to interact with component references from blueprints
 */
UCLASS()
class BLUEPRINTCOMPONENTREFERENCE_API UBlueprintComponentReferenceLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	/**
	 * Resolve component reference in specified actor (impure).
	 *
	 * @param Reference Component reference to resolve
	 * @param Actor Target actor
	 * @param Class Expected component class (optional)
	 * @param Result Output pin selector
	 * @param Component Resolved component
	 */
	UFUNCTION(BlueprintCallable, Category="Utilities|ComponentReference", meta=(DisplayName="Find Referenced Component", DefaultToSelf="Actor",  DeterminesOutputType="Class", DynamicOutputParam="Component", ExpandEnumAsExecs="Result", Keywords="GetReferencedComponent cref"))
	static void TryGetReferencedComponent(const FBlueprintComponentReference& Reference, AActor* Actor, TSubclassOf<UActorComponent> Class, EComponentSearchResult& Result, UActorComponent*& Component);

	/**
	 * Resolve component reference in specified actor (pure).
	 *
	 * @param Reference Component reference to resolve
	 * @param Actor Target actor
	 * @param Class Expected component class
	 * @param Component Resolved component
	 * @return True if component found, False otherwise
	 */
	UFUNCTION(BlueprintPure, Category="Utilities|ComponentReference", meta=( DefaultToSelf="Actor", DeterminesOutputType="Class", DynamicOutputParam="Component", Keywords="cref"))
	static UPARAM(DisplayName="Success") bool GetReferencedComponent(const FBlueprintComponentReference& Reference, AActor* Actor, TSubclassOf<UActorComponent> Class, UActorComponent*& Component);

	/**
	 * Resolve array of component references in specific actor
	 *
	 * @param References Component reference to resolve
	 * @param Actor Target actor
	 * @param Class Expected component class
	 * @param bKeepNulls Preserve order if component resolve failed
	 * @param Components Resolved components
	 */
	UFUNCTION(BlueprintCallable, Category="Utilities|ComponentReference", meta=(DefaultToSelf="Actor", bKeepNulls=false, AdvancedDisplay=3, DeterminesOutputType="Class", DynamicOutputParam="Components", Keywords="cref"))
	static void GetReferencedComponents(const TArray<FBlueprintComponentReference>& References, AActor* Actor, TSubclassOf<UActorComponent> Class, bool bKeepNulls, TArray<UActorComponent*>& Components);

	/**
	 * Does the component reference have no value set?
	 *
	 * @param Reference Input reference
	 * @return True if reference have no value set
	 */
	UFUNCTION(BlueprintPure, Category="Utilities|ComponentReference", meta=(DisplayName="Is Null Component Reference", BlueprintThreadSafe, Keywords="cref"))
	static bool IsNullComponentReference(const FBlueprintComponentReference& Reference);

	/**
	 * Does the component reference have any value set?
	 *
	 * @param Reference Input reference
	 * @return True if reference have any value set
	 */
	UFUNCTION(BlueprintPure, Category="Utilities|ComponentReference", meta=(DisplayName="Is Valid Component Reference", BlueprintThreadSafe, Keywords="cref"))
	static bool IsValidComponentReference(const FBlueprintComponentReference& Reference);

	/**
	 * Reset reference variable value to none
	 *
	 * @param Reference Input reference
	 */
	UFUNCTION(BlueprintCallable, Category="Utilities|ComponentReference", meta=(DisplayName="Invalidate Component Reference", Keywords="cref"))
	static void InvalidateComponentReference(UPARAM(Ref) FBlueprintComponentReference& Reference);

	/** Returns true if the values are equal (A == B) */
	UFUNCTION(BlueprintPure, Category="Utilities|ComponentReference", meta=(DisplayName="Equal (BlueprintComponentReference)", CompactNodeTitle="==", BlueprintThreadSafe, Keywords = "== equal cref"))
	static bool EqualEqual_ComponentReference( const FBlueprintComponentReference& A, const FBlueprintComponentReference& B );

	/** Returns true if the values are not equal (A != B) */
	UFUNCTION(BlueprintPure, Category="Utilities|ComponentReference", meta=(DisplayName="Not Equal (BlueprintComponentReference)", CompactNodeTitle="!=", BlueprintThreadSafe, Keywords = "!= not equal cref"))
	static bool NotEqual_ComponentReference( const FBlueprintComponentReference& A, const FBlueprintComponentReference& B );

	/** Convert reference to readable string */
	UFUNCTION(BlueprintPure, Category="Utilities|ComponentReference", meta=(DisplayName="To String (BlueprintComponentReference)", CompactNodeTitle = "->", Keywords="cast convert cref", BlueprintThreadSafe, BlueprintAutocast))
	static FString Conv_ComponentReferenceToString(const FBlueprintComponentReference& Reference);
};
