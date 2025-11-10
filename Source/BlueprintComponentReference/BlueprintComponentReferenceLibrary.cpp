// Copyright 2024, Aquanox.

#include "BlueprintComponentReferenceLibrary.h"
#include "GameFramework/Actor.h"

inline bool TestComponentClass(UActorComponent* In, UClass* InClass)
{
	return InClass ? In && In->IsA(InClass) : In != nullptr;
}

template<typename TBase = FBlueprintComponentReference>
bool ResolveComponentInternal(const TBase& Reference, AActor* Actor, UClass* Class, UActorComponent*& Component)
{
	UActorComponent* Result = Reference.GetComponent(Actor);
	if (!TestComponentClass(Result, Class))
	{
		Result = nullptr;
	}
	Component = Result;
	return Result != nullptr;
}

bool UBlueprintComponentReferenceLibrary::GetReferencedComponent(const FBlueprintComponentReference& Reference, AActor* Actor, TSubclassOf<UActorComponent> Class, UActorComponent*& Component)
{
	return ResolveComponentInternal(Reference, Actor, Class, Component);
}

void UBlueprintComponentReferenceLibrary::TryGetReferencedComponent(const FBlueprintComponentReference& Reference, AActor* Actor, TSubclassOf<UActorComponent> Class, EComponentSearchResult& Result, UActorComponent*& Component)
{
	Result = ResolveComponentInternal(Reference, Actor, Class, Component) ? EComponentSearchResult::Found : EComponentSearchResult::NotFound;
}

void UBlueprintComponentReferenceLibrary::GetReferencedComponents(const TArray<FBlueprintComponentReference>& References, AActor* Actor, TSubclassOf<UActorComponent> Class, bool bKeepNulls, TArray<UActorComponent*>& Components)
{
	Components.Empty();

	for (const FBlueprintComponentReference& Reference : References)
	{
		UActorComponent* Component = nullptr;
		ResolveComponentInternal(Reference, Actor, Class, Component);

		if (Component != nullptr || bKeepNulls)
		{
			Components.Add(Component);
		}
	}
}

void UBlueprintComponentReferenceLibrary::GetSetReferencedComponents(const TSet<FBlueprintComponentReference>& References, AActor* Actor, TSubclassOf<UActorComponent> Class, TSet<UActorComponent*>& Components)
{
	Components.Empty();

	for (const FBlueprintComponentReference& Reference : References)
	{
		UActorComponent* Component = nullptr;
		ResolveComponentInternal(Reference, Actor, Class, Component);

		if (Component != nullptr)
		{
			Components.Add(Component);
		}
	}
}

bool UBlueprintComponentReferenceLibrary::IsNullComponentReference(const FBlueprintComponentReference& Reference)
{
	return Reference.IsNull();
}

bool UBlueprintComponentReferenceLibrary::IsValidComponentReference(const FBlueprintComponentReference& Reference)
{
	return !Reference.IsNull();
}

void UBlueprintComponentReferenceLibrary::InvalidateComponentReference(FBlueprintComponentReference& Reference)
{
	Reference.Invalidate();
}

bool UBlueprintComponentReferenceLibrary::EqualEqual_ComponentReference(const FBlueprintComponentReference& A, const FBlueprintComponentReference& B)
{
	return A == B;
}

bool UBlueprintComponentReferenceLibrary::NotEqual_ComponentReference(const FBlueprintComponentReference& A, const FBlueprintComponentReference& B)
{
	return A != B;
}

FString UBlueprintComponentReferenceLibrary::Conv_ComponentReferenceToString(const FBlueprintComponentReference& Reference)
{
	return Reference.ToString();
}

bool UBlueprintComponentReferenceLibrary::Array_ContainsComponent(const TArray<FBlueprintComponentReference>& TargetArray, UActorComponent* ItemToFind)
{
	if(TargetArray.Num() && ItemToFind && ItemToFind->GetOwner())
	{
		AActor* SearchTarget = ItemToFind->GetOwner();
		for (const FBlueprintComponentReference& Reference : TargetArray)
		{
			if (Reference.GetComponent(SearchTarget) != nullptr)
			{
				return true;
			}
		}
	}
	return false;
}

bool UBlueprintComponentReferenceLibrary::Set_ContainsComponent(const TSet<FBlueprintComponentReference>& TargetSet, UActorComponent* ItemToFind)
{
	if(TargetSet.Num() && ItemToFind && ItemToFind->GetOwner())
	{
		AActor* SearchTarget = ItemToFind->GetOwner();
		for (const FBlueprintComponentReference& Reference : TargetSet)
		{
			if (Reference.GetComponent(SearchTarget) != nullptr)
			{
				return true;
			}
		}
	}
	return false;
}

bool UBlueprintComponentReferenceLibrary::Map_FindComponent(const TMap<int32, int32>& TargetMap, UActorComponent* Component, int32& Value)
{
	checkNoEntry();
	return false;
}

bool UBlueprintComponentReferenceLibrary::Map_FindComponent_Impl(const void* TargetMap, const FMapProperty* MapProperty, const void* KeyPtr, void* OutValuePtr)
{
	if (!MapProperty->KeyProp->IsA(FStructProperty::StaticClass()) ||
		CastFieldChecked<FStructProperty>(MapProperty->KeyProp)->Struct != StaticStruct<FBlueprintComponentReference>())
	{
		FFrame::KismetExecutionMessage(
			*FString::Printf(TEXT("Attempted use 'FindComponentInRefMap' node with map '%s' that does not use 'FBlueprintComponentReference' key!"),
			*MapProperty->GetName()), ELogVerbosity::Error);
		return false;
	}
	
	const UActorComponent* SearchComponent = static_cast<const UActorComponent*>(KeyPtr);
	if(TargetMap && IsValid(SearchComponent) && IsValid(SearchComponent->GetOwner()))
	{
		AActor* SearchTarget = SearchComponent->GetOwner();
		uint8* FoundValuePtr = nullptr;
		
		FScriptMapHelper MapHelper(MapProperty, TargetMap);

#if UE_VERSION_OLDER_THAN(5, 0, 0)
		int32 Size = MapHelper.Num();
		for( int32 I = 0; Size; ++I )
		{
			if(MapHelper.IsValidIndex(I))
			{
				const FBlueprintComponentReference* Reference = reinterpret_cast<const FBlueprintComponentReference*>(MapHelper.GetKeyPtr(I));
				if (Reference->GetComponent(SearchTarget) == SearchComponent)
				{
					FoundValuePtr = MapHelper.GetValuePtr(I);
					break;
				}
				--Size;
			}
		}
#else
		for (FScriptMapHelper::FIterator It(MapHelper); It; ++It)
		{
			const FBlueprintComponentReference* pKey = reinterpret_cast<const FBlueprintComponentReference*>(MapHelper.GetKeyPtr(It));
			uint8* pValue = MapHelper.GetValuePtr(It);

			if (pKey->GetComponent(SearchTarget) == SearchComponent)
			{
				FoundValuePtr = pValue;
				break;
			}
		}
#endif
		
		if (OutValuePtr)
		{
			if (FoundValuePtr)
			{
				MapProperty->ValueProp->CopyCompleteValueFromScriptVM(OutValuePtr, FoundValuePtr);
			}
			else
			{
				MapProperty->ValueProp->InitializeValue(OutValuePtr);
			}
		}
		
		return FoundValuePtr != nullptr;
	}
	return false;
}
