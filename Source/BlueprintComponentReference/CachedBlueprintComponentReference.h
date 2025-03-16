#pragma once

#include "BlueprintComponentReference.h"
#include "Containers/Map.h"
#include "Containers/Array.h"

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
 *		TCachedComponentReference<USceneComponent, &ThisClass::ReferenceSingle> CachedReferenceSingle { this };
 *
 *      // array entry
 *
 *		UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Test", meta=(AllowedClasses="/Script/Engine.SceneComponent"))
 *		TArray<FBlueprintComponentReference> ReferenceArray;
 *		
 *		TCachedComponentReferenceArray<USceneComponent, &ThisClass::ReferenceArray> CachedReferenceArray { this };
 *
 *      // map value entry
 *    
 *		UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Test", meta=(AllowedClasses="/Script/Engine.SceneComponent"))
 *		TMap<FGameplayTag, FBlueprintComponentReference> ReferenceMap;
 *		
 *		TCachedComponentReferenceMap<USceneComponent, decltype(ReferenceMap)::KeyType, &ThisClass::ReferenceMap> CachedReferenceMap { this };
 *
 * };
 *
 * @endcode
 * 
 */
struct FCachedComponentReferenceBase
{
	// todo: move BaseActor/SetBase to base?
	
	FCachedComponentReferenceBase() = default;
	~FCachedComponentReferenceBase() = default;
private:
	FCachedComponentReferenceBase(const FCachedComponentReferenceBase&) = delete;
	FCachedComponentReferenceBase& operator=(const FCachedComponentReferenceBase&) = delete;
	
};

/**
 * EXPERIMENTAL. <br/>
 * 
 * Version 1: This version takes BCR pointer and optional owning actor.
 *
 * @code
 *     TCachedComponentReference<USceneComponent> CachedTargetCompA  { this, &TargetComponent };
 *     TCachedComponentReference<USceneComponent> CachedTargetCompB  { &TargetComponent };
 * @endcode 
 * 
 */
template<typename TComponent>
struct TCachedComponentReferenceV1 : public FCachedComponentReferenceBase
{
private:
	FBlueprintComponentReference* const Target; // target component reference 
	TWeakObjectPtr<AActor>				BaseActor; // base actor for lookup
	TWeakObjectPtr<TComponent>			CachedValue; // storage for cached value 
public:
	explicit TCachedComponentReferenceV1() : Target(nullptr) {}
	explicit TCachedComponentReferenceV1(FBlueprintComponentReference* InTarget) : Target(InTarget) { }
	explicit TCachedComponentReferenceV1(AActor* Owner, FBlueprintComponentReference* InTarget) : Target(InTarget), BaseActor(Owner) { }
	
	/** @see FBlueprintComponentReference::Get */
	TComponent* Get(AActor* InActor = nullptr)
	{
		if (!InActor)
		{ // prefer input value over inner ptr
			InActor = BaseActor.Get();
		}
		
		TComponent* Component = CachedValue.Get();
		if (Component && Component->GetOwner() == InActor)
		{
			return Component;
		}
		
		Component = Target->GetComponent<TComponent>(InActor);
		CachedValue = Component;
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
		CachedValue.Reset();
	}
};

/**
 * EXPERIMENTAL. <br/>
 * 
 * Another version with templates, compared to V1 it takes `this` as constructor value and member is set via template.
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
 *     // version 2 always with this passed as first arg, determine if actor or not inside 
 *     TCachedComponentReference<USceneComponent, &ThisClass::TargetComponent> CachedTargetComp { this };
 * };
 *
 * @endcode
 * 
 */
template<typename TComponent, auto PtrToMember>
struct TCachedComponentReferenceV2 : public FCachedComponentReferenceBase
{
private:
	FBlueprintComponentReference* const Target; // target component reference
	TWeakObjectPtr<AActor> BaseActor; //  base actor for lookup
	TWeakObjectPtr<TComponent> CachedValue; // storage for cached value 
public:
	explicit TCachedComponentReferenceV2() : Target(nullptr) {}
	
	template<typename TOwner>
	explicit TCachedComponentReferenceV2(TOwner* InOwner) : Target(&((*InOwner).*PtrToMember))
	{
		if constexpr (std::is_base_of_v<AActor, TOwner>)
		{ // todo: enableif constructor picking for actor with assign / for uobject without?
			BaseActor = InOwner;
		}
	}

	void SetBaseActor(AActor* InActor)
	{
		BaseActor = InActor;
	}

	/** @see FBlueprintComponentReference::Get */
	TComponent* Get()
	{
		if (AActor* Actor = BaseActor.Get())
		{
			return Get(Actor);
		}
		return nullptr;
	}
	
	/** @see FBlueprintComponentReference::Get */
	TComponent* Get(AActor* InActor)
	{
		TComponent* Component = CachedValue.Get();
		if (Component && Component->GetOwner() == InActor)
		{
			return Component;
		}

		Component = Target->GetComponent<TComponent>(InActor);
		CachedValue = Component;
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
    	CachedValue.Reset();
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
 *     TArray<FBlueprintComponentReference> TargetComponents;
 *    
 *     TCachedComponentReferenceArray<USceneComponent, &ThisClass::TargetComponents> CachedTargetComp { this };
 * };
 *
 * @endcode
 * 
 */
template<typename TComponent, auto PtrToMember>
struct TCachedComponentReferenceArrayV2 : public FCachedComponentReferenceBase
{
private:
	TArray<FBlueprintComponentReference>* const Target; // target component reference
	TWeakObjectPtr<AActor> BaseActor; //  base actor for lookup
	TArray<TWeakObjectPtr<TComponent>> CachedValues; // storage for cached values 
public:
	explicit TCachedComponentReferenceArrayV2() : Target(nullptr) {}
	
	template<typename TOwner>
	explicit TCachedComponentReferenceArrayV2(TOwner* InOwner) : Target(&((*InOwner).*PtrToMember))
	{
		if constexpr (std::is_base_of_v<AActor, TOwner>)
		{ // todo: enableif constructor picking for actor with assign / for uobject without?
			BaseActor = InOwner;
		}
	}
	
	void SetBaseActor(AActor* InActor)
	{
		BaseActor = InActor;
	}

	// todo: expose iterators?

	/** @see FBlueprintComponentReference::Get */
	TComponent* Get(int32 Index)
	{
		if (AActor* Actor = BaseActor.Get())
		{
			return Get(Actor, Index);
		}
		return nullptr;
	}
	
	/** @see FBlueprintComponentReference::Get */
	TComponent* Get(AActor* InActor, int32 Index)
	{
		if (CachedValues.Num() != Target->Num())
		{
			CachedValues.SetNum(Target->Num());
		}

		if (!CachedValues.IsValidIndex(Index))
		{
			return nullptr;
		}

		TWeakObjectPtr<TComponent>& ElementRef = CachedValues[Index];
		
		TComponent* Component = ElementRef.Get();
		if (Component && Component->GetOwner() == InActor)
		{
			return Component;
		}


		Component = ((*Target)[Index]).GetComponent<TComponent>(InActor);
		ElementRef = Component;
		return Component;
	}
	
	/**  reset cached component */
	void InvalidateCache(int32 Index = -1)
	{
		if (CachedValues.IsValidIndex(Index))
		{
			CachedValues[Index].Reset();
		}
		else
		{
			CachedValues.Reset();
		}
	}

	// forwards from Target

	int32	Num() const { return Target->Num(); }
	bool	IsEmpty() const { return Target->Num() == 0; }
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
 *     TMap<FGameplayTag, FBlueprintComponentReference> Component;
 *    
 *     TCachedComponentReferenceMap<USceneComponent, &ThisClass::TargetComponents> CachedTargetComp { this };
 * };
 *
 * @endcode
 * 
 */
template<typename TComponent, typename TKey, auto PtrToMember>
struct TCachedComponentReferenceMapV2 : public FCachedComponentReferenceBase
{
private:
	TMap<TKey, FBlueprintComponentReference>* const Target; // target component reference
	TWeakObjectPtr<AActor> BaseActor; //  base actor for lookup
	TMap<TKey, TWeakObjectPtr<TComponent>> CachedValues; // storage for cached values 
public:
	explicit TCachedComponentReferenceMapV2() : Target(nullptr) {}

	template<typename TOwner>
	explicit TCachedComponentReferenceMapV2(TOwner* InOwner) : Target(&((*InOwner).*PtrToMember))
	{
		if constexpr (std::is_base_of_v<AActor, TOwner>)
		{
			BaseActor = InOwner;
		}
	}
	
	void SetBaseActor(AActor* InActor)
	{
		BaseActor = InActor;
	}
	
	// todo: expose iterators?

	/** @see FBlueprintComponentReference::Get */
	TComponent* Get(const TKey& Key)
	{
		if (AActor* Actor = BaseActor.Get())
		{
			return Get(Actor, Key);
		}
		return nullptr;
	}
	
	/** @see FBlueprintComponentReference::Get */
	TComponent* Get(AActor* InActor, const TKey& Key)
	{
		TWeakObjectPtr<TComponent>& ElementRef = CachedValues.FindOrAdd(Key, TWeakObjectPtr<TComponent>());
		
		TComponent* Component = ElementRef.Get();
		if (Component && Component->GetOwner() == InActor)
		{
			return Component;
		}

		if (FBlueprintComponentReference* Ref = Target->Find(Key))
		{
			Component = Ref->GetComponent<TComponent>(InActor);
			ElementRef = Component;
		}
		else
		{
			Component = nullptr;
			ElementRef = nullptr;
		}
		
		return Component;
	}
	
	/**  reset cached component */
	void InvalidateCache()
	{
		CachedValues.Reset();
	}
	
	// forwards from Target

	int32	Num() const { return Target->Num(); }
	bool	IsEmpty() const { return Target->Num() == 0; }
};

template<typename TComponent, auto PtrToMember>
using  TCachedComponentReference = TCachedComponentReferenceV2<TComponent, PtrToMember>;
template<typename TComponent, auto PtrToMember>
using  TCachedComponentReferenceArray = TCachedComponentReferenceArrayV2<TComponent, PtrToMember>;
template<typename TComponent, typename TKey, auto PtrToMember>
using  TCachedComponentReferenceMap = TCachedComponentReferenceMapV2<TComponent, TKey, PtrToMember>;
