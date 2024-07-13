// Copyright 2024, Aquanox.

#pragma once

#include "Engine/TimerHandle.h"
#include "Components/ActorComponent.h"
#include "BlueprintComponentReference.generated.h"

class FOutputDevice;

/**
 * Defines method which ComponentReference resolves the component from actor
 */
UENUM()
enum class EBlueprintComponentReferenceMode
{
	/**
	 * Undefined referencing mode
	 */
	None UMETA(Hidden),
	/**
	 * Referencing via FProperty
	 */
	VariableName UMETA(DisplayName="Variable"),
	/**
	 * Referencing via subobject path
	 */
	ObjectPath UMETA(DisplayName="Object Path"),
};

/**
 * Struct that allows referencing actor components within blueprint.
 *
 * Supported meta-specifiers:
 *
 * - AllowedClasses=/Script/Engine.ClassA,/Script/Engine.Class.B
 *		Specifies list of allowed base component types
 *
 * - DisallowedClasses=/Script/Engine.ClassA,/Script/Engine.Class.B
 *		Specifies list of disallowed base component types
 *
 * - NoPicker
 *		Disables component picker functions, allowing direct editing
 *		Default: false
 *
 * - NoClear
 *		Disables "Clear" action, that resets value to default state
 *		Default: false
 *
 * - NoNavigate
 *		Disables "Navigate" action, that attempts to select component in Components View window
 *		Default: false
 *
 *	- ShowNative=bool
 *		Should include native (C++ CreateDefaultSubobject) components in list?
 *		Default: true
 *
 * - ShowBlueprint=bool
 *		Should include blueprint (Simple Construction Script) components in list?
 *		Default: true
 *
 * - ShowInstanced=bool
 *		Should include instanced components in list?
 *		Default: false
 *
 * - ShowPathOnly=bool
 *		Should include instanced components that have no variable bound to?
 *		Default: false
 *
 */
USTRUCT(BlueprintType)
struct BLUEPRINTCOMPONENTREFERENCE_API FBlueprintComponentReference
{
	GENERATED_BODY()
public:
	/**
	 * Default constructor
	 */
	FBlueprintComponentReference();
	/**
	 * Construct reference from smart path.
	 *
	 * If mode not specified "Variable" mode is used.
	 */
	explicit FBlueprintComponentReference(const FString& InValue);
	/**
	 * Construct reference manually
	 */
	explicit FBlueprintComponentReference(EBlueprintComponentReferenceMode InMode, const FName& InValue);

	/**
	 * Set reference value from string.
	 * Value may be represented as a pair "mode:value" or "value"
	 *
	 * @param InValue string
	 */
	void SetValueFromString(const FString& InValue);

	/**
	 * Get reference value as string
	 *
	 * @param bIncludeMode should include mode prefix
	 *
	 * @return string
	 */
	FString GetValueString(bool bIncludeMode = true) const;

	/**
	 * Get current component selection mode
	 */
	EBlueprintComponentReferenceMode GetMode() const
	{
		return Mode;
	}

	/**
	 * Reset reference to default (None) state
	 */
	void Reset();

	/**
	 * Get the actual component pointer from this reference
	 *
	 * @param SearchActor Actor to perform search in
	 */
	UActorComponent* GetComponent(AActor* SearchActor) const;

	/** Get the actual component pointer from this reference */
	template<typename T>
	T* GetComponent(AActor* SearchActor) const
	{
		return Cast<T>(GetComponent(SearchActor));
	}

	/**
	 * Does this reference have any value set
	 */
	bool IsNull() const;

	bool ImportTextItem(const TCHAR*& Buffer, int32 PortFlags, UObject* Parent, FOutputDevice* ErrorText);

	bool ExportTextItem(FString& ValueStr, const FBlueprintComponentReference& DefaultValue, UObject* Parent, int32 PortFlags, UObject* ExportRootScope) const;

	bool SerializeFromMismatchedTag(const FPropertyTag& Tag, FStructuredArchive::FSlot Slot);


protected:
	friend class FBlueprintComponentReferenceCustomization;
	friend class FBlueprintComponentReferenceHelper;

	UPROPERTY(EditAnywhere, Category=Component)
	EBlueprintComponentReferenceMode Mode;

	UPROPERTY(EditAnywhere, Category=Component)
	FName Value;
};

template<>
struct TStructOpsTypeTraits<FBlueprintComponentReference> : TStructOpsTypeTraitsBase2<FBlueprintComponentReference>
{
	enum
	{
		WithExportTextItem = false,
		WithImportTextItem = false,
		WithStructuredSerializeFromMismatchedTag = true,
	};
};
