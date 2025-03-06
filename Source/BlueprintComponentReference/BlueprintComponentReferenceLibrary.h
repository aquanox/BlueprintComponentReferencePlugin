// Copyright 2024, Aquanox.

#pragma once

#include "BlueprintComponentReference.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Components/ActorComponent.h"
#include "BlueprintComponentReferenceLibrary.generated.h"

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
	 * @param Reference Reference to resolve
	 * @param Actor Target actor
	 * @param Component Resolved component
	 * @param Class Expected component class
	 * @return True if component found, False otherwise
	 */
	UFUNCTION(BlueprintCallable, Category="Utilities|ComponentReference", meta=( DefaultToSelf="Actor",  DeterminesOutputType="Class", DynamicOutputParam="Component", AdvancedDisplay=2))
	static UPARAM(DisplayName="Success") bool TryGetReferencedComponent(const FBlueprintComponentReference& Reference, AActor* Actor, TSubclassOf<UActorComponent> Class, UActorComponent*& Component);

	/**
	 * Resolve component reference in specified actor (pure).
	 *
	 * @param Reference Reference to resolve
	 * @param Actor Target actor
	 * @param Class Expected component class
	 * @param Component Resolved component
	 * @return True if component found, False otherwise
	 */
	UFUNCTION(BlueprintPure, Category="Utilities|ComponentReference", meta=( DefaultToSelf="Actor",  DeterminesOutputType="Class", DynamicOutputParam="Component", AdvancedDisplay=2))
	static UPARAM(DisplayName="Success") bool GetReferencedComponent(const FBlueprintComponentReference& Reference, AActor* Actor, TSubclassOf<UActorComponent> Class, UActorComponent*& Component);
	
	/**
	 * Resolve array of component references in specific actor
	 *
	 * @param References References to resolve
	 * @param Actor Target actor
	 * @param Class Expected component class
	 * @param Components Resolved components
	 * @param bKeepNulls Preserve order if component resolve failed
	 */
	UFUNCTION(BlueprintCallable, Category="Utilities|ComponentReference", meta=(DefaultToSelf="Actor", bAllowNull = false, AdvancedDisplay=2, DeterminesOutputType="Class", DynamicOutputParam="Components"))
	static void GetReferencedComponents(const TArray<FBlueprintComponentReference>& References, AActor* Actor, TSubclassOf<UActorComponent> Class, bool bKeepNulls, TArray<UActorComponent*>& Components);

	/**
	 * Does the component reference has any value set
	 *
	 * @param Reference Input reference
	 * @return True if reference has any value set
	 */
	UFUNCTION(BlueprintPure, Category="Utilities|ComponentReference", DisplayName="Is Null")
	static bool IsNullComponentReference(const FBlueprintComponentReference& Reference);
	
	/**
	 * Reset reference variable value to none
	 *
	 * @param Reference Input reference
	 */
	UFUNCTION(BlueprintCallable, Category="Utilities|ComponentReference", DisplayName="Invalidate")
	static void InvalidateComponentReference(UPARAM(Ref) FBlueprintComponentReference& Reference);

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
