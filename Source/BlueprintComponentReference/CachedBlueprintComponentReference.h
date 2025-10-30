#pragma once

#include "BlueprintComponentReference.h"
#include "Containers/Map.h"
#include "Containers/Array.h"
#include "UObject/ObjectKey.h"
#include "Misc/CoreMiscDefines.h"

class AActor;
class UActorComponent;
class UObject;
class FReferenceCollector;

namespace BCRDetails
{
	/**
	 * A helper to abstract away certain combinations when using CBCR
	 * 
	 * ObjectPtr Actor   WeakPtr Component 
	 * WeakPtr   Actor   WeakPtr Component 
	 * ObjectPtr Actor   ObjectPtr Component 
	 */
	struct TPointerFuncs
	{
		static constexpr bool ExposeActor = false;
		static constexpr bool ExposeComponent = false;
		
		template<typename T>
		static T* ToRawPointer(const T& InPtr) = delete;
		template<typename T>
		static bool IsValidPointer(const T& InPtr) = delete;
		template<typename T>
		static void ExposePointer(const T& InPtr, FReferenceCollector& Collector, const T* ReferencingObject) = delete;
	};
	/**
	 * Weak/Weak combination 
     */
	struct TWeakPointerFuncs : TPointerFuncs
	{
		template<typename T>
		using PtrTypeForActor = TWeakObjectPtr<T>;
		template<typename T>
		using PtrTypeForComponent = TWeakObjectPtr<T>;

		static constexpr bool ExposeActor = false;
		static constexpr bool ExposeComponent = false;

		template<typename T>
		static T* ToRawPointer(const TWeakObjectPtr<T>& InPtr) { return InPtr.Get(); }
		template<typename T>
		static bool IsValidPointer(const TWeakObjectPtr<T>& InPtr) { return InPtr.IsValid(); }
		template<typename T>
		static void ExposePointer(const T& InPtr, FReferenceCollector& Collector, const T* ReferencingObject) {}
	};
	/**
	 * Strong/Strong raw pointers
	 */
	struct TRawPointerFuncs : TPointerFuncs
	{
		template<typename T>
		using PtrTypeForActor = T*;
		template<typename T>
		using PtrTypeForComponent = T*;

		static constexpr bool ExposeActor = true;
		static constexpr bool ExposeComponent = true;

		template<typename T>
		static T* ToRawPointer(T* InPtr) { return InPtr; }
		template<typename T>
		static bool IsValidPointer(T* InPtr) { return ::IsValid(InPtr); }
		template<typename T>
		static void ExposePointer(T*& InPtr, FReferenceCollector& Collector, const T* ReferencingObject)
		{
			Collector.AddReferencedObject(InPtr, ReferencingObject);
		}
	};

	struct TSetKeyFuncs : DefaultKeyFuncs<FBlueprintComponentReference, false>
	{
		using KeyInitType = typename DefaultKeyFuncs<FBlueprintComponentReference, false>::KeyInitType;
		using ElementInitType = typename DefaultKeyFuncs<FBlueprintComponentReference, false>::ElementInitType;
	};
	
	template<typename ValueType>
	struct TMapKeyFuncs : TDefaultMapHashableKeyFuncs<FBlueprintComponentReference, ValueType, false>
	{
		using KeyInitType = typename TDefaultMapHashableKeyFuncs<FBlueprintComponentReference, ValueType, false>::KeyInitType;
		using ElementInitType = typename TDefaultMapHashableKeyFuncs<FBlueprintComponentReference, ValueType, false>::ElementInitType;
		using HashabilityCheck = typename TDefaultMapHashableKeyFuncs<FBlueprintComponentReference, ValueType, false>::HashabilityCheck;
	};
}

#define BCR_DEFAULT_CONSTRUCTORS(TypeName) \
	explicit TypeName(ENoInit) : Super(ENoInit::NoInit) { } \
	explicit TypeName(TargetType* InTarget) : Super(InTarget, nullptr) { } \
	explicit TypeName(AActor* InBaseActor, TargetType* InTarget) : Super(InTarget, InBaseActor) { } \

#define BCR_MOVE_ONLY_TYPE(TypeName) \
	TypeName(const TypeName&) = delete; \
	TypeName& operator=(const TypeName&) = delete; \
	TypeName(TypeName&& Other) : Super(Forward<TypeName>(Other)) {}  \
	TypeName& operator=(TypeName&& Other) { Super::operator=(Forward<TypeName>(Other)); return *this; } \

/**
 * EXPERIMENTAL. <br/>
 *
 * A helper type that proxies FBlueprintComponentReference calls with a cached weak pointer to resolved component with explicit type.
 *
 * Goal is to provide caching of resolved value and type safety for usage in code, without adding cached pointer into original reference struct.
 *
 * Data source expected to be immutable and read-only (changes only in editor at design time, no runtime modification).
 * If original data ever changes in runtime - cache must be invalidated or changes synced by accessing Storage directly.
 *
 * Helpers wrap each common use:
 *
 * - TCachedComponentReference for single entry
 * - TCachedComponentReferenceArray for array entry
 * - TCachedComponentReferenceMap for map value
 *
 * @code
 *
 * UCLASS()
 * class AMyActorClass : public AActor
 * {
 *	   GENERATED_BODY()
 *	public:
 *      // single entry
 *
 *		UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Test", meta=(AllowedClasses="/Script/Engine.SceneComponent"))
 *		FBlueprintComponentReference ReferenceSingle;
 *
 *		TCachedComponentReference<USceneComponent> CachedReferenceSingle { this, &ReferenceSingle };
 *
 *      // array entry
 *
 *		UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Test", meta=(AllowedClasses="/Script/Engine.SceneComponent"))
 *		TArray<FBlueprintComponentReference> ReferenceArray;
 *
 *		TCachedComponentReferenceArray<USceneComponent> CachedReferenceArray { this, &ReferenceArray };
 * };
 *
 * @endcode
 *
 */
class FCachedComponentReferenceBase
{
public:
	FCachedComponentReferenceBase() = default;
	~FCachedComponentReferenceBase() = default;

private:
	FCachedComponentReferenceBase(const FCachedComponentReferenceBase&) = delete;
	FCachedComponentReferenceBase& operator=(const FCachedComponentReferenceBase&) = delete;
};

/**
 * Generic base
 * 
 * @tparam Target Source reference type
 * @tparam Storage Resolved storage type
 * @tparam Traits Internal traits type
 */
template<typename Target, typename Storage, typename Traits>
class TCachedComponentReferenceBase : public FCachedComponentReferenceBase
{
public:
	using TargetType = Target;
	using StorageType = Storage;
	using TraitsType = Traits;
	using ActorType = typename Traits::template PtrTypeForActor<AActor>;
protected:
	// data source
	TargetType* InternalTarget;
	// storage for cached data
	mutable StorageType InternalStorage;
	// base actor for lookup
	mutable ActorType BaseActor;
public:
	TCachedComponentReferenceBase(ENoInit)
		: InternalTarget(nullptr), BaseActor(nullptr)
	{
	}
	
	TCachedComponentReferenceBase(TargetType* InTarget, AActor* InActor)
		: InternalTarget(InTarget), BaseActor(InActor)
	{
	}
	
	TCachedComponentReferenceBase(TCachedComponentReferenceBase&& Other)
		: InternalTarget(MoveTemp(Other.InternalTarget))
		, InternalStorage(MoveTemp(Other.InternalStorage))
		, BaseActor(MoveTemp(Other.BaseActor))
	{
	}

	TCachedComponentReferenceBase& operator=(TCachedComponentReferenceBase&& Other)
	{
		InternalTarget = MoveTemp(Other.InternalTarget);
		InternalStorage = MoveTemp(Other.InternalStorage);
		BaseActor = MoveTemp(Other.BaseActor);
		return *this;
	}

	ActorType& GetBaseActor() /*fake*/ const { return BaseActor; }
	AActor* GetBaseActorPtr() const { return Traits::ToRawPointer(BaseActor); }

	TargetType& GetTarget() const { return *InternalTarget; }
	StorageType& GetStorage() /*fake*/ const { return InternalStorage; }
};

/**
 * EXPERIMENTAL. <br/>
 *
 * Templated wrapper over FBlueprintComponentReference that caches pointer to resolved objects.
 *
 * @code
 *     TCachedComponentReferenceSingle<USceneComponent> CachedTargetCompA  { this, &TargetComponent };
 *     TCachedComponentReferenceSingle<USceneComponent> CachedTargetCompB  { &TargetComponent };
 * @endcode
 * 
 * @tparam Component Expected component type
 * @tparam Traits Internal traits type
 */
template<typename Component, typename Traits = BCRDetails::TWeakPointerFuncs>
class TCachedComponentReferenceSingle
	: public TCachedComponentReferenceBase<FBlueprintComponentReference, typename Traits::template PtrTypeForComponent<Component>, Traits>
{
	using Super = TCachedComponentReferenceBase<FBlueprintComponentReference, typename Traits::template PtrTypeForComponent<Component>, Traits>;
public:
	using StorageType = typename Super::StorageType;
	using TargetType = typename Super::TargetType;

	BCR_DEFAULT_CONSTRUCTORS(TCachedComponentReferenceSingle)
	BCR_MOVE_ONLY_TYPE(TCachedComponentReferenceSingle)

	template<typename T = Component>
	Component* Get()
	{
		return this->template Get<T>(this->GetBaseActorPtr());
	}

	template<typename T = Component>
	Component* Get(AActor* InActor)
	{
		static_assert(std::is_base_of_v<Component, T>, "T must be a descendant of Component");

		Component* Result = Traits::ToRawPointer(this->GetStorage());
		if (Result && Result->GetOwner() == InActor)
		{
			return Cast<T>(Result);
		}

		Result = this->GetTarget().template GetComponent<Component>(InActor);
		this->GetStorage() = Result;
		return Cast<T>(Result);
	}

	void Invalidate()
	{
		this->GetStorage() = nullptr;
	}

	void AddReferencedObjects(FReferenceCollector& Collector, const UObject* ReferencingObject = nullptr)
	{
		if (Traits::ExposeActor)
		{
			Traits::ExposePointer(this->GetBaseActor(), Collector, ReferencingObject);
		}
		if (Traits::ExposeComponent)
		{
			Traits::ExposePointer(this->GetStorage(), Collector, ReferencingObject);
		}
	}
};

template<typename Component, typename Traits = BCRDetails::TWeakPointerFuncs>
using  TCachedComponentReference = TCachedComponentReferenceSingle<Component, Traits>;

/**
 * EXPERIMENTAL. <br/>
 * 
 * Templated wrapper over TArray<FBlueprintComponentReference> that stores pointers to resolved objects.
 *
 * @code
 * UCLASS()
 * class AMyActorClass : public AActor
 * {
 *	   GENERATED_BODY()
 *	public:
 *     UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, AllowedClasses="/Script/Engine.SceneComponent")
 *     TArray<FBlueprintComponentReference> TargetComponents;
 *
 *     TCachedComponentReferenceArray<USceneComponent> CachedTargetComp { this, &TargetComponents };
 * };
 *
 * @endcode
 *
 * @tparam Component Expected component type
 * @tparam Traits Internal type traits
 */
template<typename Component, typename Traits = BCRDetails::TWeakPointerFuncs>
class TCachedComponentReferenceArray
	: public TCachedComponentReferenceBase<TArray<FBlueprintComponentReference>, TArray<typename Traits::template PtrTypeForComponent<Component>>, Traits>
{
	using Super = TCachedComponentReferenceBase<TArray<FBlueprintComponentReference>, TArray<typename Traits::template PtrTypeForComponent<Component>>, Traits>;
public:
	using StorageType = typename Super::StorageType;
	using TargetType = typename Super::TargetType;

	BCR_DEFAULT_CONSTRUCTORS(TCachedComponentReferenceArray)
	BCR_MOVE_ONLY_TYPE(TCachedComponentReferenceArray)

	template<typename T = Component>
	T* Get(int32 Index)
	{
		return this->template Get<T>(this->GetBaseActorPtr(), Index);
	}

	template<typename T = Component>
	T* Get(AActor* InActor, int32 Index)
	{
		static_assert(std::is_base_of_v<Component, T>, "T must be a descendant of Component");

		StorageType& Storage = this->GetStorage();
		TargetType& Target = this->GetTarget();

		if (Storage.Num() != Target.Num())
		{
			Storage.Reset();//purge it all, dont try track modification
			Storage.SetNum(Target.Num());
		}

		if (!Storage.IsValidIndex(Index))
		{
			return nullptr;
		}

		auto& ElementRef = Storage[Index];

		Component* Result = Traits::ToRawPointer(ElementRef);
		if (Result && Result->GetOwner() == InActor)
		{
			return Cast<T>(Result);
		}

		Result = Target[Index].template GetComponent<Component>(InActor);
		ElementRef = Result;
		return Cast<T>(Result);
	}
	
	void GetAll(AActor* InActor)
	{
		if (!InActor)
		{
			InActor = this->GetBaseActorPtr();
		}
		
		TargetType& Target = this->GetTarget();
		StorageType& Storage = this->GetStorage();
		Storage.SetNum(Target.Num());

		for (int32 Index = 0, Num = Target.Num(); Index < Num; ++Index)
		{
			Storage[Index] = Target[Index].template GetComponent<Component>(InActor);
		}
	}

	/**  reset cached component */
	void Invalidate()
	{
		for (auto& ElementRef : this->GetStorage())
		{
			ElementRef = nullptr;
		}
	}
	
	/**  reset cached component at index */
	void InvalidateAt(int32 Index)
	{
		StorageType& Storage = this->GetStorage();
		if (Storage.IsValidIndex(Index))
		{
			Storage[Index] = nullptr;
		}
	}

	int32 Num() const
	{
		return this->GetTarget().Num();
	}

	bool IsEmpty() const
	{
		return this->GetTarget().Num() == 0;
	}
	
	void AddReferencedObjects(FReferenceCollector& Collector, const UObject* ReferencingObject = nullptr)
	{
		if (Traits::ExposeActor)
		{
			Traits::ExposePointer(this->GetBaseActor(), Collector, ReferencingObject);
		}
		if (Traits::ExposeComponent)
		{
			for (auto& ElementRef : this->GetStorage())
			{
				Traits::ExposePointer(ElementRef, Collector, ReferencingObject);
			}
		}
	}
};

/**
 * EXPERIMENTAL. <br/>
 *
 * Templated wrapper over TMap<TKey, FBlueprintComponentReference> that stores pointers to resolved objects
 *
 * @code
 * UCLASS()
 * class AMyActorClass : public AActor
 * {
 *	   GENERATED_BODY()
 *	public:
 *     UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, AllowedClasses="/Script/Engine.SceneComponent")
 *     TMap<FGameplayTag, FBlueprintComponentReference> TargetComponents;
 *
 *     TCachedComponentReferenceMapValue<USceneComponent, FGameplayTag> CachedTargetComp { this, &TargetComponents };
 * };
 *
 * @endcode
 *
 * @tparam Component Expected component type
 * @tparam Key Map key type
 * @tparam Traits Internal type traits
 */
template<typename Component, typename Key, typename Traits = BCRDetails::TWeakPointerFuncs>
class TCachedComponentReferenceMapValue
	: public TCachedComponentReferenceBase<TMap<Key, FBlueprintComponentReference>, TMap<Key, typename Traits::template PtrTypeForComponent<Component>>, Traits>
{
	using Super = TCachedComponentReferenceBase<TMap<Key, FBlueprintComponentReference>, TMap<Key, typename Traits::template PtrTypeForComponent<Component>>, Traits>;
public:
	using StorageType = typename Super::StorageType;
    using TargetType = typename Super::TargetType;

	BCR_DEFAULT_CONSTRUCTORS(TCachedComponentReferenceMapValue)
	BCR_MOVE_ONLY_TYPE(TCachedComponentReferenceMapValue)

	template<typename T = Component>
	Component* Get(const Key& InKey)
	{
		return this->template Get<T>(this->GetBaseActorPtr(), InKey);
	}

	template<typename T = Component>
	Component* Get(AActor* InActor, const Key& InKey)
	{
		static_assert(std::is_base_of_v<Component, T>, "T must be a descendant of Component");

		TargetType& Target = this->GetTarget();
		StorageType& Storage = this->GetStorage();

		auto& ElementRef = Storage.FindOrAdd(InKey, {});

		Component* Result = Traits::ToRawPointer(ElementRef);
		if (Result && Result->GetOwner() == InActor)
		{
			return Cast<T>(Result);
		}

		if (const FBlueprintComponentReference* Ref = Target.Find(InKey))
		{
			Result = Ref->GetComponent<Component>(InActor);
			ElementRef = Result;
		}
		else
		{
			Result = nullptr;
			ElementRef = nullptr;
		}

		return Cast<T>(Result);
	}

	void Invalidate()
	{
		this->GetStorage().Empty();
	}

	void AddReferencedObjects(FReferenceCollector& Collector, const UObject* ReferencingObject = nullptr)
	{
		if (Traits::ExposeActor)
		{
			Traits::ExposePointer(this->GetBaseActor(), Collector, ReferencingObject);
		}
		if (Traits::ExposeComponent)
		{
			for (auto& ElementRef : this->GetStorage())
			{
				Traits::ExposePointer(ElementRef.Value, Collector, ReferencingObject);
			}
		}
	}
};


/**
 * EXPERIMENTAL. <br/>
 * 
 * Templated wrapper over TMap<FBlueprintComponentReference, TValue> that stores pointers to resolved objects.
 *
 * This version always using TObjectKey for internal storage key.
 * 
 * @code
 * UCLASS()
 * class AMyActorClass : public AActor
 * {
 *	   GENERATED_BODY()
 *	public:
 *     UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, AllowedClasses="/Script/Engine.SceneComponent")
 *     TMap<FBlueprintComponentReference, FDemoStruct> TargetComponents;
 *
 *     TCachedComponentReferenceMapKey<USceneComponent, FDemoStruct> CachedTargetComp { this, &TargetComponents };
 * };
 *
 * @endcode
 * 
 * @tparam Component Expected component type
 * @tparam Value Map key type
 * @tparam Traits Internal type traits
 */
template<typename Component, typename Value, typename Traits = BCRDetails::TWeakPointerFuncs>
class TCachedComponentReferenceMapKey
	: public TCachedComponentReferenceBase<TMap<FBlueprintComponentReference, Value>, TMap<TObjectKey<Component>, Value*>, Traits>
{
	using Super = TCachedComponentReferenceBase<TMap<FBlueprintComponentReference, Value>, TMap<TObjectKey<Component>, Value*>, Traits>;
public:
	using StorageType = typename Super::StorageType;
	using TargetType = typename Super::TargetType;

	BCR_DEFAULT_CONSTRUCTORS(TCachedComponentReferenceMapKey)
	BCR_MOVE_ONLY_TYPE(TCachedComponentReferenceMapKey)

	Value* Get(const Component* InKey)
	{
		return this->Get(this->GetBaseActorPtr(), InKey);
	}

	Value* Get(AActor* InActor, const Component* InKey)
	{
		if (InActor && InKey && InActor != InKey->GetOwner())
		{ // no point in search in bad context
			return nullptr;
		}
		if (!InActor && InKey)
		{ // pull actor from component
			InActor = InKey->GetOwner(); 
		}
		
		TargetType& Target = this->GetTarget();
		StorageType& Storage = this->GetStorage();

		// if Storage is Ptr->BCR 
		// cache hit would require to do lookup to ensure taking appropriate data from target
		// cached raw ptr -> cref
		// target cref -> value
		// cache miss would require resolve loop to find who references input component
		/////////////////////////////////
		// if Storage is Ptr->Value Ptr
		// cache hit will give value pointer
		// cache miss will require resolve loop
		Value* FoundValue = nullptr;

		const auto* StoragePtr = Storage.Find(InKey);
		if (StoragePtr != nullptr)
		{
			FoundValue = *StoragePtr;// Target.Find(*StoragePtr);
		}
		else
		{    // if nothing found the only way is to do loop and find our component
			for (auto& RefToValue : Target)
			{
				const FBlueprintComponentReference& Ref = RefToValue.Key;
				if (Ref.GetComponent<Component>(InActor) == InKey)
				{
					Storage.Add(InKey, &RefToValue.Value);

					FoundValue = &RefToValue.Value;
					break;
				}
			}
		}
		
		return FoundValue;
	}

	void GetAll(AActor* InActor)
	{
		if (!InActor)
		{
			InActor = this->GetBaseActorPtr();
		}
		
		TargetType& Target = this->GetTarget();
		StorageType& Storage = this->GetStorage();

		for (auto& RefToValue : Target)
		{
			Component* Resolved = RefToValue.Key.template GetComponent<Component>(InActor);
			if (Resolved)
			{
			     Storage.Add(Resolved, &RefToValue.Value);
			}
		}
	}

	void Invalidate()
	{
		this->GetStorage().Reset();
	}

	template<typename T = UObject>
	void AddReferencedObjects(class FReferenceCollector& Collector, const T* ReferencingObject = nullptr)
	{
		if (Traits::ExposeActor)
		{
			Traits::ExposePointer(this->GetBaseActor(), Collector, ReferencingObject);
		}
	}
};
