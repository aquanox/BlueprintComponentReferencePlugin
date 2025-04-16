// Copyright 2024, Aquanox.

#pragma once

#include "BlueprintComponentReference.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Kismet/BlueprintMapLibrary.h"
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
	UFUNCTION(BlueprintCallable, Category="Utilities|ComponentReference|Containers", meta=(DisplayName="Get Referenced Components (Array)", DefaultToSelf="Actor", bKeepNulls=false, AdvancedDisplay=3, DeterminesOutputType="Class", DynamicOutputParam="Components", Keywords="cref"))
	static void GetReferencedComponents(const TArray<FBlueprintComponentReference>& References, AActor* Actor, TSubclassOf<UActorComponent> Class, bool bKeepNulls, TArray<UActorComponent*>& Components);

	/**
	 * Resolve set of component references in specific actor
	 *
	 * @param References Component reference to resolve
	 * @param Actor Target actor
	 * @param Class Expected component class
	 * @param bKeepNulls Preserve order if component resolve failed
	 * @param Components Resolved components
	 */
	UFUNCTION(BlueprintCallable, Category="Utilities|ComponentReference|Containers", meta=(DisplayName="Get Referenced Components (Set)", DefaultToSelf="Actor", DeterminesOutputType="Class", DynamicOutputParam="Components", Keywords="cref"))
	static void GetSetReferencedComponents(const TSet<FBlueprintComponentReference>& References, AActor* Actor, TSubclassOf<UActorComponent> Class, TSet<UActorComponent*>& Components);

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

	/**
	 * Returns true if the array contains the given item
	 *
	 * Uses Component's Owner as Search Target to resolve components. 
	 *
	 * @param	TargetArray		The array to search for the item
	 * @param	ItemToFind		The item to look for
	 * @return	True if the item was found within the array
	 */
	UFUNCTION(BlueprintPure, meta=(DisplayName = "Contains Component (Array)", Category="Utilities|ComponentReference|Containers"))
	static bool Array_ContainsComponent(const TArray<FBlueprintComponentReference>& TargetArray, UActorComponent* ItemToFind);
	
	/**
	 * Returns true if set contains the given item
	 *
	 * Uses Component's Owner as Search Target to resolve components. 
	 *
	 * @param	TargetSet		The set to search for the item
	 * @param	ItemToFind		The item to look for
	 * @return	True if the item was found within the set
	 */
	UFUNCTION(BlueprintPure, meta=(DisplayName = "Contains Component (Set)", Category="Utilities|ComponentReference|Containers"))
	static bool Set_ContainsComponent(const TSet<FBlueprintComponentReference>& TargetSet, UActorComponent* ItemToFind);
	
	/** 
	 * EXPERIMENTAL: Fast search of component within TMap<FBlueprintComponentReference, GenericValue> for blueprint users
	 *
	 * WARNING: It is just a decorated loop due to need to invoke GetComponent on each component reference
	 * to obtain referenced component for comparsion, but should be much faster than doing it in pure blueprint
	 * and there is no need to use blueprint macro library ForEach/ForEachWithBreak
	 * 
	 * Uses Component's Owner as Search Target to resolve components. 
	 *
	 * @param	TargetMap		The map to perform the lookup on
	 * @param	Component		Actor component to search
	 * @param	Value			The value associated with the key, default constructed if key was not found
	 * @return	True if an item was found (False indicates nothing in the map uses the provided key)
	 */
	UFUNCTION(BlueprintPure, CustomThunk, meta=(DisplayName = "Find Component (Map)", Category="Utilities|ComponentReference|Containers", MapParam = "TargetMap", MapValueParam = "Value", AutoCreateRefTerm = "Value", BlueprintInternalUseOnly=true))
	static bool Map_FindComponent(const TMap<int32, int32>& TargetMap, UActorComponent* Component, int32& Value);

	// based on KismetMathLibrary::Map_Find
	// based on KismetMathLibrary::execMap_Find
	DECLARE_FUNCTION(execMap_FindComponent)
	{
		PRAGMA_DISABLE_DEPRECATION_WARNINGS
		
		Stack.MostRecentProperty = nullptr;
		Stack.StepCompiledIn<FMapProperty>(NULL);
		void* MapAddr = Stack.MostRecentPropertyAddress;
		FMapProperty* MapProperty = CastField<FMapProperty>(Stack.MostRecentProperty);
		if (!MapProperty
			|| !MapProperty->KeyProp->IsA(FStructProperty::StaticClass())
			|| !(CastFieldChecked<FStructProperty>(MapProperty->KeyProp)->Struct == StaticStruct<FBlueprintComponentReference>()))
		{
			Stack.bArrayContextFailed = true;
			return;
		}
		
		// Key property is object ptr
		P_GET_OBJECT(UActorComponent, KeyPtr);
		
		// Since Value aren't really an int, step the stack manually
		const FProperty* CurrValueProp = MapProperty->ValueProp;
		const int32 ValuePropertySize = CurrValueProp->ElementSize * CurrValueProp->ArrayDim;
		void* ValueStorageSpace = FMemory_Alloca(ValuePropertySize);
		CurrValueProp->InitializeValue(ValueStorageSpace);
		
		Stack.MostRecentPropertyAddress = nullptr;
		Stack.MostRecentPropertyContainer = nullptr;
		Stack.StepCompiledIn<FProperty>(ValueStorageSpace);
		const FFieldClass* CurrValuePropClass = CurrValueProp->GetClass();
		const FFieldClass* MostRecentPropClass = Stack.MostRecentProperty->GetClass();
		void* ItemPtr;
		// If the destination and the inner type are identical in size and their field classes derive from one another,
		// then permit the writing out of the array element to the destination memory
		if (Stack.MostRecentPropertyAddress != NULL
			&& (ValuePropertySize == Stack.MostRecentProperty->ElementSize * Stack.MostRecentProperty->ArrayDim)
			&& (MostRecentPropClass->IsChildOf(CurrValuePropClass) || CurrValuePropClass->IsChildOf(MostRecentPropClass)))
		{
			ItemPtr = Stack.MostRecentPropertyAddress;
		}
		else
		{
			ItemPtr = ValueStorageSpace;
		}
		
		P_FINISH;
		P_NATIVE_BEGIN;
		*(bool*)RESULT_PARAM = Map_FindComponent_Impl(MapAddr, MapProperty, KeyPtr, ItemPtr);
		P_NATIVE_END;

		CurrValueProp->DestroyValue(ValueStorageSpace);
		
		PRAGMA_ENABLE_DEPRECATION_WARNINGS
	}

private:
	// Implementation of Map_FindComponent
	static bool Map_FindComponent_Impl(const void* TargetMap, const FMapProperty* MapProperty, const void* KeyPtr, void* OutValuePtr);
};
