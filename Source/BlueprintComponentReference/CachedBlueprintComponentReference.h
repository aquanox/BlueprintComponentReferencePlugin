#pragma once

#include "BlueprintComponentReference.h"
#include "Containers/Map.h"
#include "Containers/Array.h"
#include "UObject/ObjectKey.h"

class AActor;
class UActorComponent;
class UObject;

namespace BCRDetails
{
	template<typename T>
	struct TUnused {};
	
	// wrap raw pointer for the template parameter for pre 5.0?
	template<typename T>
	struct TRawPtr
	{
		using ViewType = T;
		T* Value = nullptr;
		TRawPtr(T* Value = nullptr) : Value(Value) { }
		TRawPtr(TRawPtr&& Other) : Value(Other.Value) { }
		TRawPtr(const TRawPtr& Other) : Value(Other.Value) { }
		TRawPtr& operator=(const TRawPtr& Other) { Value = Other.Value; return *this; }
		TRawPtr& operator=(TRawPtr&& Other) { Value = Other.Value; return *this; }
		operator bool() const { return Value != nullptr; }
		void Reset() { Value = nullptr; }
		T* operator->() const { return Value; }
		T* Get() const { return Value; }
		T& operator*() const { return *Value; }
	};

	template<typename T>
	T* ResolvePointer(const TObjectPtr<T>& InActor) { return InActor.Get(); }
	template<typename T>
	T* ResolvePointer(const TWeakObjectPtr<T>& InActor) { return InActor.Get(); }
	template<typename T>
	T* ResolvePointer(const TRawPtr<T>& InActor) { return InActor.Value; }

	template<typename T>
	bool ValidatePointer(const TObjectPtr<T>& InActor) { return InActor.Get() != nullptr; }
	template<typename T>
	bool ValidatePointer(const TWeakObjectPtr<T>& InActor) { return InActor.IsValid(); }
	template<typename T>
	bool ValidatePointer(const TRawPtr<T>& InActor) { return InActor.Get() != nullptr; }
	
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

/**
 * EXPERIMENTAL. <br/>
 *
 * A helper type that wraps FBlueprintComponentReference with a cached weak pointer to component and explicit type.
 *
 * Goal is to provide caching behavior and type safety for usage in code on hot paths similar to other "accessor" assistants in engine.
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
 *		UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Test", meta=(AllowedClasses="/Script/Engine.SceneComponent"))
 *		FBlueprintComponentReference ReferenceSingle;
 *
 *		TCachedComponentReference<USceneComponent> CachedReferenceSingle { this, &ReferenceSingle };
 *
 *      // array entry
 *
 *		UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Test", meta=(AllowedClasses="/Script/Engine.SceneComponent"))
 *		TArray<FBlueprintComponentReference> ReferenceArray;
 *
 *		TCachedComponentReferenceArray<USceneComponent> CachedReferenceArray { this, &ReferenceArray };
 *
 * };
 *
 * //
 * UCLASS()
 * class UMyDataAsset : public UDataAsset
 * {
 *	   GENERATED_BODY()
 *	public:
 *      // single entry
 *
 *		UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Test", meta=(AllowedClasses="/Script/Engine.SceneComponent"))
 *		FBlueprintComponentReference ReferenceSingle;
 *
 *		TCachedComponentReference<USceneComponent> CachedReferenceSingle { this, &ReferenceSingle };
 *
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
 * 
 * @tparam Component Expected component type
 * @tparam Target Source reference type
 * @tparam Storage Resolved storage type
 * @tparam InternalPtr Owning actor pointer type
 */
template<typename Component, typename Target, typename Storage, template<typename> typename InternalPtr = TWeakObjectPtr>
class TCachedComponentReferenceBase : public FCachedComponentReferenceBase
{
public:
	using ComponentType = Component;
	using TargetType = Target;
	using StorageType = Storage;
	using ActorPtrType = InternalPtr<AActor>;
protected:
	// data source
	TargetType* const InternalTarget;
	// storage for cached data
	mutable StorageType InternalStorage;
	// base actor for lookup
	ActorPtrType BaseActor;
public:
	TCachedComponentReferenceBase(TargetType* InTarget, AActor* InActor) : InternalTarget(InTarget), BaseActor(InActor) {}
	~TCachedComponentReferenceBase() = default;

	ActorPtrType& GetBaseActor() { return BaseActor; }
	AActor* GetBaseActorPtr() const { return BCRDetails::ResolvePointer(BaseActor); }
	void SetBaseActor(AActor* InActor)  { BaseActor = InActor; }

	TargetType& GetTarget() const { return *InternalTarget; }
	StorageType& GetStorage() /*fake*/const { return InternalStorage; }

	// TBD: with custom pointer types has to be managed externally
	void AddReferencedObjects(class FReferenceCollector& Collector, const UObject* ReferencingObject = nullptr) = delete;
};

/**
 * EXPERIMENTAL. <br/>
 *
 * Version 1: Cached componenent reference over a simple property target
 *
 * @code
 *     TCachedComponentReferenceSingle<USceneComponent> CachedTargetCompA  { this, &TargetComponent };
 *     TCachedComponentReferenceSingle<USceneComponent> CachedTargetCompB  { &TargetComponent };
 * @endcode
 *
 */
template<typename Component, template<typename> typename InternalPtr>
class TCachedComponentReferenceSingleV1
	: public TCachedComponentReferenceBase<Component, FBlueprintComponentReference,  InternalPtr<Component>>
{
	using Super = TCachedComponentReferenceBase<Component, FBlueprintComponentReference,  InternalPtr<Component>>;
	using StorageType = typename Super::StorageType;
	using TargetType = typename Super::TargetType;
public:
	explicit TCachedComponentReferenceSingleV1(FBlueprintComponentReference* InTarget) : Super(InTarget, nullptr) { }
	explicit TCachedComponentReferenceSingleV1(UObject* InOwner, FBlueprintComponentReference* InTarget) : Super(InTarget, nullptr) { }
	explicit TCachedComponentReferenceSingleV1(AActor* InBaseActor, FBlueprintComponentReference* InTarget) : Super(InTarget, InBaseActor) { }

	template<typename T = Component>
	Component* Get()
	{
		return this->Get<T>(this->GetBaseActorPtr());
	}

	template<typename T = Component>
	Component* Get(AActor* InActor)
	{
		static_assert(std::is_base_of_v<Component, T>, "T must be a descendant of Component");

		Component* Result = BCRDetails::ResolvePointer(this->GetStorage());
		if (Result && Result->GetOwner() == InActor)
		{
			return Cast<T>(Result);
		}

		Result = this->GetTarget().template GetComponent<Component>(InActor);
		this->GetStorage() = Result;
		return Cast<T>(Result);
	}

	void InvalidateCache()
	{
		this->GetStorage().Reset();
	}

	template<typename T = UObject>
	void AddReferencedObjects(class FReferenceCollector& Collector, const T* ReferencingObject = nullptr)
	{
		Collector.AddReferencedObject(this->GetBaseActor(), ReferencingObject);
		Collector.AddReferencedObject(this->GetStorage(), ReferencingObject);
	}
};

/**
 * EXPERIMENTAL. <br/>
 *
 * Array note: there is no point in specifying other template params for array, as blueprints work only with default arrays.
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
 *     TCachedComponentReferenceArray<USceneComponent> CachedTargetComp { this, &ThisClass::TargetComponents };
 * };
 *
 * @endcode
 *
 */
template<typename Component, template<typename> typename InternalPtr>
class TCachedComponentReferenceArrayV1
	: public TCachedComponentReferenceBase<Component, TArray<FBlueprintComponentReference>, TArray<InternalPtr<Component>>>
{
	using Super = TCachedComponentReferenceBase<Component, TArray<FBlueprintComponentReference>, TArray<InternalPtr<Component>>>;
	using StorageType = typename Super::StorageType;
	using TargetType = typename Super::TargetType;
public:
	explicit TCachedComponentReferenceArrayV1(TArray<FBlueprintComponentReference>* InTarget) : Super(InTarget, nullptr) {}
	explicit TCachedComponentReferenceArrayV1(UObject* InOwner, TArray<FBlueprintComponentReference>* InTarget) : Super(InTarget, nullptr) {}
	explicit TCachedComponentReferenceArrayV1(AActor* InBaseActor, TArray<FBlueprintComponentReference>* InTarget) : Super(InTarget, InBaseActor) {}

	template<typename T = Component>
	T* Get(int32 Index)
	{
		return this->Get<T>(this->GetBaseActorPtr(), Index);
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

		Component* Result = BCRDetails::ResolvePointer(ElementRef);
		if (Result && Result->GetOwner() == InActor)
		{
			return Cast<T>(Result);
		}


		Result = Target[Index].template GetComponent<Component>(InActor);
		ElementRef = Result;
		return Cast<T>(Result);
	}

	/**  reset cached component */
	void InvalidateCache(int32 Index = -1)
	{
		StorageType& Storage = this->GetStorage();
		if (Storage.IsValidIndex(Index))
		{
			Storage[Index].Reset();
		}
		else
		{
			Storage.Reset();
		}
	}

	int32	Num() const
	{
		return this->GetTarget().Num();
	}

	bool	IsEmpty() const
	{
		return this->GetTarget().Num() == 0;
	}
	
	template<typename T = UObject>
	void AddReferencedObjects(class FReferenceCollector& Collector, const T* ReferencingObject = nullptr)
	{
		Collector.AddReferencedObject(this->GetBaseActor(), ReferencingObject);
		for (auto& ElementRef : this->GetStorage())
		{
			Collector.AddReferencedObject(ElementRef, ReferencingObject);
		}
	}
};

/**
 * EXPERIMENTAL. <br/>
 *
 * Map note: there is no point in specifying other template params for map, as blueprints work only with default maps.
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
 *     TCachedComponentReferenceMapValue<USceneComponent, FGameplayTag> CachedTargetComp { this, &ThisClass::TargetComponents };
 * };
 *
 * @endcode
 *
 */
template<typename Component, typename Key, template<typename> typename InternalPtr>
class TCachedComponentReferenceMapValueV1
	: public TCachedComponentReferenceBase<Component, TMap<Key, FBlueprintComponentReference>, TMap<Key, InternalPtr<Component>>>
{
	using Super = TCachedComponentReferenceBase<Component, TMap<Key, FBlueprintComponentReference>, TMap<Key, InternalPtr<Component>>>;
	using StorageType = typename Super::StorageType;
    using TargetType = typename Super::TargetType;
public:
	explicit TCachedComponentReferenceMapValueV1(TMap<Key, FBlueprintComponentReference>* InTarget) : Super(InTarget, nullptr) {}
	explicit TCachedComponentReferenceMapValueV1(UObject* InOwner, TMap<Key, FBlueprintComponentReference>* InTarget) : Super(InTarget, nullptr) {}
	explicit TCachedComponentReferenceMapValueV1(AActor* InBaseActor, TMap<Key, FBlueprintComponentReference>* InTarget) : Super(InTarget, InBaseActor) {}

	template<typename T = Component>
	Component* Get(const Key& InKey)
	{
		return this->Get<T>(this->GetBaseActorPtr(), InKey);
	}

	template<typename T = Component>
	Component* Get(AActor* InActor, const Key& InKey)
	{
		static_assert(std::is_base_of_v<Component, T>, "T must be a descendant of Component");

		TargetType& Target = this->GetTarget();
		StorageType& Storage = this->GetStorage();

		auto& ElementRef = Storage.FindOrAdd(InKey, {});

		Component* Result = BCRDetails::ResolvePointer(ElementRef);
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

	void InvalidateCache()
	{
		this->GetStorage().Reset();
	}

	int32 Num() const
	{
		return this->GetTarget().Num();
	}

	bool IsEmpty() const
	{
		return this->GetTarget().Num() == 0;
	}
	
	template<typename T = UObject>
	void AddReferencedObjects(class FReferenceCollector& Collector, const T* ReferencingObject = nullptr)
	{
		Collector.AddReferencedObject(this->GetBaseActor(), ReferencingObject);
		for (auto& ElementRef : this->GetStorage())
		{
			Collector.AddReferencedObject(ElementRef.Value, ReferencingObject);
		}
	}
};

/**
 * EXPERIMENTAL. <br/>
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
 *     TCachedComponentReferenceMapKey<USceneComponent, FDemoStruct> CachedTargetComp { this, &ThisClass::TargetComponents };
 * };
 *
 * @endcode
 */
template<typename Component, typename Value, template<typename> typename InternalPtr = BCRDetails::TUnused>
class TCachedComponentReferenceMapKeyV1
	: public TCachedComponentReferenceBase<Component, TMap<FBlueprintComponentReference, Value>, TMap<TObjectKey<Component>, FBlueprintComponentReference>>
{
	using Super = TCachedComponentReferenceBase<Component, TMap<FBlueprintComponentReference, Value>, TMap<TObjectKey<Component>, FBlueprintComponentReference>>;
	using StorageType = typename Super::StorageType;
	using TargetType = typename Super::TargetType;
public:
	explicit TCachedComponentReferenceMapKeyV1(TMap<FBlueprintComponentReference, Value>* InTarget) : Super(InTarget, nullptr) {}
	explicit TCachedComponentReferenceMapKeyV1(UObject* InOwner, TMap<FBlueprintComponentReference, Value>* InTarget) : Super(InTarget, nullptr) {}
	explicit TCachedComponentReferenceMapKeyV1(AActor* InBaseActor, TMap<FBlueprintComponentReference, Value>* InTarget) : Super(InTarget, InBaseActor) {}

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

		// cache hit would require to do lookup to ensure taking appropriate data from target
		// cached raw ptr -> cref
		// target cref -> value
		// cache miss would require resolve loop to find who references input component
		Value* FoundValue = nullptr;

		const FBlueprintComponentReference* StoragePtr = Storage.Find(TObjectKey<Component>(InKey));
		if (StoragePtr != nullptr)
		{
			FoundValue = Target.Find(*StoragePtr);
		}
		else
		{    // if nothing found the only way is to do loop and find our component
			for (auto& RefToValue : Target)
			{
				const FBlueprintComponentReference& Ref = RefToValue.Key;
				if (Ref.GetComponent<Component>(InActor) == InKey)
				{
					Storage.Add(InKey, Ref);

					FoundValue = &RefToValue.Value;
					break;
				}
			}
		}
		
		return FoundValue;
	}

	void InvalidateCache()
	{
		this->GetStorage().Reset();
	}

	int32 Num() const
	{
		return this->GetTarget().Num();
	}

	bool IsEmpty() const
	{
		return this->GetTarget().Num() == 0;
	}
	
	template<typename T = UObject>
	void AddReferencedObjects(class FReferenceCollector& Collector, const T* ReferencingObject = nullptr)
	{
		Collector.AddReferencedObject(this->GetBaseActor(), ReferencingObject);
	}
};

/**
 * Templated wrapper over FBlueprintComponentReference that caches pointer to resolved objects
 * 
 * @tparam TComponent Component expected type
 * @tparam TInternalPtr Internal storage type
 */
template<typename TComponent, template<typename> typename TInternalPtr = TWeakObjectPtr>
using  TCachedComponentReference = TCachedComponentReferenceSingleV1<TComponent, TInternalPtr>;

/**
 * Templated wrapper over TArray<FBlueprintComponentReference> that stores pointers to resolved objects
 *
 * @tparam TComponent Component expected type
 * @tparam TInternalPtr Internal storage type
 */
template<typename TComponent, template<typename> typename TInternalPtr = TWeakObjectPtr>
using  TCachedComponentReferenceArray = TCachedComponentReferenceArrayV1<TComponent, TInternalPtr>;

/**
 * Templated wrapper over TMap<TKey, FBlueprintComponentReference> that stores pointers to resolved objects
 *
 * @tparam TComponent Component expected type
 * @tparam TKey Map key type
 * @tparam TInternalPtr Internal storage type
 */
template<typename TComponent, typename TKey, template<typename> typename TInternalPtr = TWeakObjectPtr>
using  TCachedComponentReferenceMap = TCachedComponentReferenceMapValueV1<TComponent, TKey, TInternalPtr>;

/**
 * Templated wrapper over TMap<FBlueprintComponentReference, TValue> that stores pointers to resolved objects
 *
 * @tparam TComponent Component expected type
 * @tparam TKey Map key type
 */
template<typename TComponent, typename TValue>
using  TCachedComponentReferenceMapKey = TCachedComponentReferenceMapKeyV1<TComponent, TValue>;
