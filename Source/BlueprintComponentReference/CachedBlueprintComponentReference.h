#pragma once

#include "BlueprintComponentReference.h"
#include "Containers/Map.h"
#include "Containers/Array.h"

class AActor;
class UActorComponent;
class UObject;

namespace BCRDetails
{
	inline AActor* ResolveBaseActor(const TObjectPtr<AActor>& InActor) { return InActor.Get(); }
	inline AActor* ResolveBaseActor(const TWeakObjectPtr<AActor>& InActor) { return InActor.Get(); }
	inline AActor* ResolveBaseActor(AActor* InActor) { return InActor; }
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

template<typename Component, typename Target, typename Storage, typename ActorPtr = TWeakObjectPtr<AActor>>
class TCachedComponentReferenceBase : public FCachedComponentReferenceBase
{
public:
	using ComponentType = Component;
	using TargetType = Target;
	using StorageType = Storage;
	using ActorPtrType = ActorPtr;
private:
	// data source
	TargetType* const InternalTarget;
	// storage for cached data
	mutable StorageType InternalStorage;
	// base actor for lookup
	ActorPtrType BaseActor;
public:
	TCachedComponentReferenceBase(TargetType* InTarget, AActor* InActor) : InternalTarget(InTarget), BaseActor(InActor) {}
	~TCachedComponentReferenceBase() = default;

	AActor* GetBaseActor()  { return BCRDetails::ResolveBaseActor(BaseActor); }
	void SetBaseActor(AActor* InActor)  { BaseActor = InActor; }
	
	TargetType& GetTarget() const { return *InternalTarget; }
	StorageType& GetStorage() /*fake*/const { return InternalStorage; }
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
template<typename Component>
class TCachedComponentReferenceSingleV1
	: public TCachedComponentReferenceBase<Component, FBlueprintComponentReference,  TWeakObjectPtr<Component>>
{
	using Super = TCachedComponentReferenceBase<Component, FBlueprintComponentReference, TWeakObjectPtr<Component>>;
	using StorageType = typename Super::StorageType;
	using TargetType = typename Super::TargetType;
public:
	explicit TCachedComponentReferenceSingleV1(FBlueprintComponentReference* InTarget) : Super(InTarget, nullptr) { }
	explicit TCachedComponentReferenceSingleV1(UObject* InOwner, FBlueprintComponentReference* InTarget) : Super(InTarget, nullptr) { }
	explicit TCachedComponentReferenceSingleV1(AActor* InBaseActor, FBlueprintComponentReference* InTarget) : Super(InTarget, InBaseActor) { }

	template<typename T = Component>
	Component* Get()
	{
		return this->Get<T>(this->GetBaseActor());
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
template<typename Component>
class TCachedComponentReferenceArrayV1
	: public TCachedComponentReferenceBase<Component, TArray<FBlueprintComponentReference>, TArray<TWeakObjectPtr<Component>>>
{
	using Super = TCachedComponentReferenceBase<Component, TArray<FBlueprintComponentReference>, TArray<TWeakObjectPtr<Component>>>;
	using StorageType = typename Super::StorageType;
	using TargetType = typename Super::TargetType;
public:
	explicit TCachedComponentReferenceArrayV1(TArray<FBlueprintComponentReference>* InTarget) : Super(InTarget, nullptr) {}
	explicit TCachedComponentReferenceArrayV1(UObject* InOwner, TArray<FBlueprintComponentReference>* InTarget) : Super(InTarget, nullptr) {}
	explicit TCachedComponentReferenceArrayV1(AActor* InBaseActor, TArray<FBlueprintComponentReference>* InTarget) : Super(InTarget, InBaseActor) {}

	template<typename T = Component>
	T* Get(int32 Index)
	{
		return this->Get<T>(this->GetBaseActor(), Index);
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
template<typename Component, typename Key>
class TCachedComponentReferenceMapV1
	: public TCachedComponentReferenceBase<Component, TMap<Key, FBlueprintComponentReference>, TMap<Key, TWeakObjectPtr<Component>>>
{
	using Super = TCachedComponentReferenceBase<Component, TMap<Key, FBlueprintComponentReference>, TMap<Key, TWeakObjectPtr<Component>>>;
	using StorageType = typename Super::StorageType;
    using TargetType = typename Super::TargetType;
public:
	explicit TCachedComponentReferenceMapV1(TMap<Key, FBlueprintComponentReference>* InTarget) : Super(InTarget, nullptr) {}
	explicit TCachedComponentReferenceMapV1(UObject* InOwner, TMap<Key, FBlueprintComponentReference>* InTarget) : Super(InTarget, nullptr) {}
	explicit TCachedComponentReferenceMapV1(AActor* InBaseActor, TMap<Key, FBlueprintComponentReference>* InTarget) : Super(InTarget, InBaseActor) {}

	template<typename T = Component>
	Component* Get(const Key& InKey)
	{
		return this->Get<T>(this->GetBaseActor(), InKey);
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

template<typename TComponent>
using  TCachedComponentReference = TCachedComponentReferenceSingleV1<TComponent>;
template<typename TComponent>
using  TCachedComponentReferenceArray = TCachedComponentReferenceArrayV1<TComponent>;
template<typename TComponent, typename TKey>
using  TCachedComponentReferenceMap = TCachedComponentReferenceMapV1<TComponent, TKey>;
