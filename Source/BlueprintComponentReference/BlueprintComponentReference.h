// Copyright 2024, Aquanox.

#pragma once

#include "UObject/SoftObjectPtr.h"
#include "Components/ActorComponent.h"
#include "BlueprintComponentReference.generated.h"

#ifndef WITH_CACHED_COMPONENT_REFERENCE
#define WITH_CACHED_COMPONENT_REFERENCE 0
#endif

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
 * - Local Blueprint variable
 * - Array property
 * - Map property as Value
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

#if WITH_CACHED_COMPONENT_REFERENCE

/**
 * EXPERIMENTAL. <br/>
 * 
 * A helper type that wraps `FBlueprintComponentReference` with a cached weak pointer to component.
 * 
 * Goal is to provide caching behavior and type safety for usage in code.
 *
 * Version 1: This version takes BCR pointer and optional owning actor.
 *
 * @code
 * UCLASS()
 * class AMyActorClass : public AActor
 * {
 *	   GENERATED_BODY()
 *	public:
 *     UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, AllowedClasses="/Script/Engine.SceneComponent")
 *     FBlueprintComponentReference TargetComponent;
 *
 *     TCachedComponentReference<USceneComponent> CachedTargetCompA  { this, &TargetComponent };
 *
 *     TCachedComponentReference<USceneComponent> CachedTargetCompB  { &TargetComponent };
 *     
 * };
 *
 * @endcode 
 * 
 * @tparam TComponent Component type
 */
template<typename TComponent /*, bool bStrictOwnerCheck = true */>
struct TCachedComponentReferenceV1
{
private:
	FBlueprintComponentReference* const Target; // target component reference 
	TWeakObjectPtr<AActor>				ActorPtr; // base actor for lookup
	TWeakObjectPtr<TComponent>			ValuePtr; // storage for cached value 
public:
	explicit TCachedComponentReferenceV1() : Target(nullptr) {}
	
	explicit TCachedComponentReferenceV1(FBlueprintComponentReference* InTarget) : Target(InTarget) { }

	explicit TCachedComponentReferenceV1(AActor* Owner, FBlueprintComponentReference* InTarget) : Target(InTarget), ActorPtr(Owner) { }
	
	/** @see FBlueprintComponentReference::Get */
	TComponent* Get(AActor* InActor = nullptr)
	{
		if (!InActor)
		{ // prefer input value over inner ptr
			InActor = ActorPtr.Get();
		}
		
		TComponent* Component = ValuePtr.Get();
		if (Component && Component->GetOwner() == InActor)
		{
			return Component;
		}
		
		Component = Target->GetComponent<TComponent>(InActor);
		ValuePtr = Component;
		return Component;
	}
	/** @see FBlueprintComponentReference::IsNull */
	bool IsNull() const
	{
		return Target->IsNull();
	}
	/**  reset cached component */
	void InvalidateCache()
	{
		ValuePtr.Reset();
	}
};

/**
 * EXPERIMENTAL. <br/>
 * 
 * Another version with templates, compared to V1 it always take `this` as constructor value and member is set via template.
 *
 * @code
 * UCLASS()
 * class AMyActorClass : public AActor
 * {
 *	   GENERATED_BODY()
 *	public:
 *     UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, AllowedClasses="/Script/Engine.SceneComponent")
 *     FBlueprintComponentReference TargetComponent;
 *    
 *    
 *     // version 2 always with this passed as first arg, determine if actor or not inside 
 *     TCachedComponentReference<USceneComponent, &ThisClass::TargetComponent> CachedTargetComp { this };
 * };
 *
 * @endcode
 * 
 */
template<typename TComponent, /* typename TOwner, */ auto PtrToMember /*, bool bStrictOwnerCheck = true */>
struct TCachedComponentReferenceV2
{
private:
	FBlueprintComponentReference* const Target; // target component reference
	TWeakObjectPtr<AActor> ActorPtr; //  base actor for lookup
	TWeakObjectPtr<TComponent> ValuePtr; // storage for cached value 
public:
	explicit TCachedComponentReferenceV2() : Target(nullptr) {}
	
	template<typename TOwner>
	explicit TCachedComponentReferenceV2(TOwner* InOwner) : Target(&((*InOwner).*PtrToMember))
	{
		if constexpr (std::is_base_of_v<AActor, TOwner>)
		{ // todo: enableif constructor picking for actor with assign / for uobject without?
			ActorPtr = InOwner;
		}
	}

	/** @see FBlueprintComponentReference::Get */
	TComponent* Get()
	{
		if (AActor* Actor = ActorPtr.Get())
		{
			return Get(Actor);
		}
		return nullptr;
	}
	
	/** @see FBlueprintComponentReference::Get */
	TComponent* Get(AActor* InActor)
	{
		TComponent* Component = ValuePtr.Get();
		if (Component && Component->GetOwner() == InActor)
		{
			return Component;
		}

		Component = Target->GetComponent<TComponent>(InActor);
		ValuePtr = Component;
		return Component;
	}
	
	/** @see FBlueprintComponentReference::IsNull */
    bool IsNull() const
    {
    	return Target->IsNull();
    }
    /**  reset cached component */
    void InvalidateCache()
    {
    	ValuePtr.Reset();
    }
};

#endif
