// Copyright 2024, Aquanox.

#pragma once

#include "Components/ActorComponent.h"
#include "BlueprintComponentReference.generated.h"

class FOutputDevice;

/**
 *
 */
UENUM()
enum class EBlueprintComponentReferenceMode
{
	None UMETA(Hidden),
	VariableName,
	ObjectPath,
};

/**
 * Struct that allows referencing actor components within blueprint.
 *
 * Supported meta-specifiers:
 * - AllowedClasses=classlist
 * - DisallowedClasses=classlist
 * - ImplementsInterface=classlist
 *
 * - NoNavigate
 * - NoPicker
 *
 * - ShowBlueprint=bool
 * - ShowNative=bool
 * - ShowInstanced=bool
 * - ShowPathOnly=bool
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
	 * Construct reference from smart path
	 */
	explicit FBlueprintComponentReference(const FString& InValue);
	/**
	 * Construct reference manually
	 */
	explicit FBlueprintComponentReference(EBlueprintComponentReferenceMode InMode, const FName& InValue);

	/**
	 *
	 * @param InValue
	 */
	void SetValueFromString(const FString& InValue);

	/**
	 *
	 * @return
	 */
	FString GetValueString(bool bFull = true) const;

	/**
	 *
	 **/
	void Reset();

	/** Get the actual component pointer from this reference */
	UActorComponent* GetComponent(AActor* SearchActor) const;

	/** Get the actual component pointer from this reference */
	template<typename T>
	T* GetComponent(AActor* SearchActor) const
	{
		return Cast<T>(GetComponent(SearchActor));
	}

	/** Does this reference has any value set */
	bool IsNull() const;

	bool ImportTextItem(const TCHAR*& Buffer, int32 PortFlags, UObject* Parent, FOutputDevice* ErrorText);

	bool ExportTextItem(FString& ValueStr, const FBlueprintComponentReference& DefaultValue, UObject* Parent, int32 PortFlags, UObject* ExportRootScope) const;

	bool SerializeFromMismatchedTag(const FPropertyTag& Tag, FStructuredArchive::FSlot Slot);

	/** get current selection mode */
	EBlueprintComponentReferenceMode GetMode() const {return Mode;}
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

/*
USTRUCT()
struct FBlueprintCachedComponentReference : public FBlueprintComponentReference
{
	GENERATED_BODY()
public:
	UActorComponent* GetComponent(AActor* SearchActor) const
	{
		if (UActorComponent* Local = CachedValue.Get())
		{
			return Local;
		}

		if (auto Resolved = FBlueprintComponentReference::GetComponent(SearchActor))
		{
			CachedValue = Resolved;
			return Resolved;
		}

		return nullptr;
	}

	template<typename T>
	T* GetComponent(AActor* SearchActor) const
	{
		return Cast<T>(GetComponent(SearchActor));
	}

	bool IsValid() const
	{
		return CachedValue.IsValid();
	}

protected:
	UPROPERTY(Transient, Category=Component)
	mutable TWeakObjectPtr<UActorComponent> CachedValue;
};


template<>
struct TStructOpsTypeTraits<FBlueprintCachedComponentReference> : TStructOpsTypeTraitsBase2<FBlueprintComponentReference>
{
	enum
	{
		WithExportTextItem = false,
		WithImportTextItem = false,
		WithStructuredSerializeFromMismatchedTag = true,
	};
};

*/
