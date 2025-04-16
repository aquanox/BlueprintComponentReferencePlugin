#pragma once

#include "BlueprintComponentReference.h"
#include "Containers/Map.h"
#include "Containers/Array.h"

class AActor;
class UActorComponent;
class UObject;

namespace BCRDetails
{
	// wrap raw pointer for the template parameter for pre 5.0?
	template<typename T>
	struct TRawPtr
	{
		using ViewType = T;
		T* Value = nullptr;
		TRawPtr(T* Value = nullptr) : Value(Value) { }
		TRawPtr(const TRawPtr& Other) : Value(Other.Value) { }
		TRawPtr& operator=(const TRawPtr& Other) { Value = Other.Value; return *this; }
		operator bool() const { return Value != nullptr; }
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
 *      // map value entry
 *
 *		UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Test", meta=(AllowedClasses="/Script/Engine.SceneComponent"))
 *		TMap<FGameplayTag, FBlueprintComponentReference> ReferenceMap;
 *
 *		TCachedComponentReferenceMap<USceneComponent, FGameplayTag> CachedReferenceMap { this, &ReferenceMap };
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
	void AddReferencedObjects(class FReferenceCollector& Collector) = delete;
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

		Component* Result = this->GetStorage().Get();
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
	
	/*void AddReferencedObjects(class FReferenceCollector& Collector)
	{
		Collector.AddReferencedObject(this->BaseActor);
		Collector.AddReferencedObject(this->InternalStorage);
	}*/
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

		TWeakObjectPtr<Component>& ElementRef = Storage[Index];

		Component* Result = ElementRef.Get();
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
 *     TMap<FGameplayTag, FBlueprintComponentReference> Component;
 *
 *     TCachedComponentReferenceMap<USceneComponent, &ThisClass::TargetComponents> CachedTargetComp { this };
 * };
 *
 * @endcode
 *
 */
template<typename Component, typename Key, template<typename> typename InternalPtr>
class TCachedComponentReferenceMapV1
	: public TCachedComponentReferenceBase<Component, TMap<Key, FBlueprintComponentReference>, TMap<Key, InternalPtr<Component>>>
{
	using Super = TCachedComponentReferenceBase<Component, TMap<Key, FBlueprintComponentReference>, TMap<Key, InternalPtr<Component>>>;
	using StorageType = typename Super::StorageType;
    using TargetType = typename Super::TargetType;
public:
	explicit TCachedComponentReferenceMapV1(TMap<Key, FBlueprintComponentReference>* InTarget) : Super(InTarget, nullptr) {}
	explicit TCachedComponentReferenceMapV1(UObject* InOwner, TMap<Key, FBlueprintComponentReference>* InTarget) : Super(InTarget, nullptr) {}
	explicit TCachedComponentReferenceMapV1(AActor* InBaseActor, TMap<Key, FBlueprintComponentReference>* InTarget) : Super(InTarget, InBaseActor) {}

	template<typename T = Component>
	Component* Get(const Key& InKey)
	{
		return this->Get<T>(this->GetBaseActorPtr(), InKey);
	}

	template<typename T = Component>
	Component* Get(AActor* InActor, const Key& InKey)
	{
		static_assert(std::is_base_of_v<Component, T>, "T must be a descendant of Component");

		StorageType& Storage = this->GetStorage();
		TargetType& Target = this->GetTarget();

		auto& ElementRef = Storage.FindOrAdd(InKey, {});

		Component* Result = ElementRef.Get();
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
};

#ifdef CBCRV2

/**
 * EXPERIMENTAL. <br/>
 *
 * Another version with templates, compared to V1 it always take `this` as constructor parameter and member is set via template.
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
 *     TCachedComponentReference<USceneComponent, &ThisClass::TargetComponent> CachedTargetComp { this };
 * };
 *
 * @endcode
 *
 */
template<typename TComponent, auto PtrToMember>
class TCachedComponentReferenceSingleV2 : public TCachedComponentReferenceSingleV1<TComponent>
{
	using Super = TCachedComponentReferenceSingleV1<TComponent>;
public:
	template<typename TOwner>
	explicit TCachedComponentReferenceSingleV2(TOwner* InOwner) : Super(&((*InOwner).*PtrToMember))
	{
		if constexpr (std::is_base_of_v<AActor, TOwner>)
		{
			SetBaseActor(InOwner);
		}
	}
};

template<typename TComponent, auto PtrToMember>
class TCachedComponentReferenceArrayV2 : public TCachedComponentReferenceArrayV1<TComponent>
{
	using Super = TCachedComponentReferenceArrayV1<TComponent>;
public:
	template<typename TOwner>
	explicit TCachedComponentReferenceArrayV2(TOwner* InOwner) : Super(&((*InOwner).*PtrToMember))
	{
		if constexpr (std::is_base_of_v<AActor, TOwner>)
		{
			SetBaseActor(InOwner);
		}
	}
};

template<typename TComponent, typename TKey, auto PtrToMember>
class TCachedComponentReferenceMapV2 : public TCachedComponentReferenceMapV1<TComponent, TKey>
{
	using Super = TCachedComponentReferenceMapV1<TComponent, TKey>;
public:
	template<typename TOwner>
	explicit TCachedComponentReferenceMapV2(TOwner* InOwner) : Super(&((*InOwner).*PtrToMember))
	{
		if constexpr (std::is_base_of_v<AActor, TOwner>)
		{
			SetBaseActor(InOwner);
		}
	}
};

#endif

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
using  TCachedComponentReferenceMap = TCachedComponentReferenceMapV1<TComponent, TKey, TInternalPtr>;
