// Copyright 2024, Aquanox.

#pragma once

#include "UObject/SoftObjectPtr.h"
#include "Components/ActorComponent.h"
#include "BlueprintComponentReference.generated.h"

DECLARE_DYNAMIC_DELEGATE_RetVal_OneParam(bool, FBlueprintComponentFilter, UActorComponent*, Component);

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
	Path UMETA(DisplayName="Object Path"),
	/**
	 *
	 */
	//Dynamic  UMETA(DisplayName="Selector")
};

/**
 * Struct that allows referencing actor components within blueprint.
 *
 * Supported uses:
 * - Class/Struct member property
 * - Local Blueprint variable
 * - Array property
 * - Map property as Value
 *
 * Supported meta-specifiers:
 *
 * - ActorClass=/Script/Module.ClassName
 *      Specifies actor class that would be used to suggest components in dropdown
 *      This specifier is used only in scenarios when automatic discovery of actor type is not possible (Data Asset member)
 *      or there is a need to enforce specific actor type
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
 * - ShowHidden=bool
 *		Should include instanced components that have no variable bound to?
 *		Default: false
 *
 * - ShowEditor=bool
 *		Should include editor-only components?
 *		Default: true
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
		WithIdenticalViaEquality = true
	};
};
