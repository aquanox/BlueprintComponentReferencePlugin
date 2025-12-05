// Copyright 2024, Aquanox.

#pragma once

#include "UObject/SoftObjectPtr.h"
#include "Components/ActorComponent.h"
#include "BlueprintComponentReference.generated.h"

/**
 * Defines method which ComponentReference resolves the component from actor
 */
UENUM()
enum class EBlueprintComponentReferenceMode : uint8
{
	/**
	 * Undefined referencing mode
	 */
	None UMETA(Hidden),
	/**
	 * Referencing via property
	 */
	Property UMETA(DisplayName="Property Name"),
	/**
	 * Referencing via object path
	 */
	Path UMETA(DisplayName="Object Path")
};

/**
 * Struct that allows referencing actor components within blueprint.
 *
 * Component picker behavior customized via metadata specifiers.
 *
 *
 * Supported use cases:
 * - Class/Struct member property
 * - Blueprint or Local Blueprint Function variable
 * - Array property
 * - Set property
 * - Map property as Key or Value
 *
 * <p>Example code</p>
 * @code
 *	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ShowBlueprint=True, ShowNative=False, NoNavigate))
 *	FBlueprintComponentReference MyProperty;
 * @endcode
 *
 * <p>Component display and filtering:</p>
 *
 *	- ShowNative=bool <br/>
 *		Should include native (C++ CreateDefaultSubobject) components in list?
 *		Default: True
 *
 * - ShowBlueprint=bool <br/>
 *		Should include blueprint (Simple Construction Script) components in list?
 *		Default: True
 *
 * - ShowInstanced=bool <br/>
 *		Should include instanced components in list?
 *		Default: False
 *
 * - ShowHidden=bool <br/>
 *		Should include instanced components that have no variable bound to?
 *		Default: False
 *
 * - ShowEditor=bool <br/>
 *		Should include editor-only components?
 *		Default: True
 *
 * - ShowRoot=bool <br/>
 *		Should include actor root component magic reference?
 *		Note: It is always USceneComponent, actual type can be determined only in runtime.
 *		Default: False
 *
 * - AllowedClasses="/Script/Engine.ClassA,/Script/Engine.Class.B" <br/>
 *		Specifies list of allowed base component types
 *
 * - DisallowedClasses="/Script/Engine.ClassA,/Script/Engine.Class.B" <br/>
 *		Specifies list of disallowed base component types
 *
 *
 *	<p>Miscellaneious:</p>
 *
 * - ActorClass="/Script/Module.ClassName" <br/>
 *      Specifies actor class that would be used as a source to suggest components in dropdown <br/>
 *      This specifier is used only in scenarios when automatic discovery of actor type is not possible (Data Asset member)
 *      or there is a need to enforce specific actor type <br/>
 *      Prefer native actor classes over blueprints to avoid loading unnesessary assets.
 *
 * - NoClear <br/>
 *		Disables "Clear" action, that resets value to default state
 *		Default: False
 *
 * - NoNavigate <br/>
 *		Disables "Navigate" action, that attempts to select component in Components View window
 *		Default: False
 *
 * - NoPicker  <br/>
 *		Disables component picker functions, enables direct editing of inner properties.
 *		Default: False
 *
 */
USTRUCT(BlueprintType, meta=(DisableSplitPin))
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
	bool ParseString(const FString& InValue);

	/**
	 * Get reference value as string
	 *
	 *
	 * @return string
	 */
	FString ToString() const;

	/**
	 * Get current component selection mode
	 */
	EBlueprintComponentReferenceMode GetMode() const
	{
		return Mode;
	}

	/**
	 * Get current component value
	 */
	const FName& GetValue() const
	{
		return Value;
	}

	/**
	 * Get the actual component pointer from this reference
	 *
	 * @param SearchActor Actor to perform search in
	 */
	UActorComponent* GetComponent(AActor* SearchActor) const;

	/**
	 * Get the actual component pointer from this reference
	 *
	 * @param SearchActor Actor to perform search in
	 */
	template<typename T>
	T* GetComponent(AActor* SearchActor) const
	{
		return Cast<T>(GetComponent(SearchActor));
	}

	/**
	 * Does this reference have any value set
	 */
	bool IsNull() const;

	/**
	 * Reset reference value to none
	 */
	void Invalidate();

	/**
	 * Handle type migration when reading serialized data
	 */
	bool SerializeFromMismatchedTag(const FPropertyTag& Tag, FStructuredArchive::FSlot Slot);

	bool operator==(const FBlueprintComponentReference& Rhs) const
	{
		return Mode == Rhs.Mode && Value == Rhs.Value;
	}

	bool operator!=(const FBlueprintComponentReference& Rhs) const
	{
		return !(*this == Rhs);
	}

	friend uint32 GetTypeHash(const FBlueprintComponentReference& A)
	{
		return HashCombine(GetTypeHash(A.Mode), GetTypeHash(A.Value));
	}

	static FBlueprintComponentReference ForProperty(const FName& InName)
	{
		return FBlueprintComponentReference(EBlueprintComponentReferenceMode::Property, InName);
	}

	static FBlueprintComponentReference ForPath(const FName& InPath)
	{
		return FBlueprintComponentReference(EBlueprintComponentReferenceMode::Path, InPath);
	}

protected:
	friend class FBlueprintComponentReferenceHelper;

	UPROPERTY(EditAnywhere, Category=Component)
	EBlueprintComponentReferenceMode Mode;

	UPROPERTY(EditAnywhere, Category=Component)
	FName Value;
};

template<>
struct TStructOpsTypeTraits<FBlueprintComponentReference>
	: TStructOpsTypeTraitsBase2<FBlueprintComponentReference>
{
	enum
	{
		WithIdenticalViaEquality = true,
		WithStructuredSerializeFromMismatchedTag = true
	};
};
