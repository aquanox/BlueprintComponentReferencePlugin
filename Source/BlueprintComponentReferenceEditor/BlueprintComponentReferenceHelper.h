// Copyright 2024, Aquanox.

#pragma once

#include "CoreMinimal.h"
#include "BlueprintComponentReference.h"
#include "Engine/World.h"
#include "Misc/EngineVersionComparison.h"
#include "Context/ComponentPickerContext.h"

#if UE_VERSION_OLDER_THAN(5,4,0)
inline static FName GetFNameSafe(const UObject* InField)
{
	if (IsValid(InField))
	{
		return InField->GetFName();
	}
	return NAME_None;
}
#endif

/**
 * BCR customization manager.
 *
 */
class BLUEPRINTCOMPONENTREFERENCEEDITOR_API FBlueprintComponentReferenceHelper
{
public:
	/**
	 * Test if property is supported by BCR customization
	 */
	static bool IsComponentReferenceProperty(const FProperty* InProperty);

	/**
	 * Test if type is a BCR type
	 */
	static bool IsComponentReferenceType(const UStruct* InStruct);

	/**
	 * Get or create component chooser data source for specific input parameters
	 *
	 * @param InActor Input actor
	 * @param InClass Input class
	 * @param InLabel Debug marker
	 * @return Context instance
	 */
	static TSharedPtr<FComponentPickerContext> CreateChooserContext(AActor* InActor, UClass* InClass, const FString& InLabel);

	/** IS it a blueprint property or not */
	static bool IsBlueprintProperty(const FProperty* VariableProperty);

	/** */
	static UClass* FindClassByName(const FString& ClassName);

	static bool IsRootComponentReference(const FBlueprintComponentReference& InRef);

	static bool InvokeComponentFilter(TSharedPtr<class IPropertyHandle> InProperty, const FString& InFilterFn, const UObject* InObj);

	static FString BuildComponentDebugInfo(const UActorComponent* Obj);

	/**
	 * Attempt to switch to blueprint editor and highlight or select component in component view
	 */
	static void TryNavigateToComponent(AActor* Actor,  TSharedPtr<FComponentInfo> Info);

	static void SetMode_Private(FBlueprintComponentReference& Reference, EBlueprintComponentReferenceMode NewMode)
	{
		Reference.Mode = NewMode;
	}

	static void SetValue_Private(FBlueprintComponentReference& Reference, FName NewValue)
	{
		Reference.Value = NewValue;
	}

private:


};
