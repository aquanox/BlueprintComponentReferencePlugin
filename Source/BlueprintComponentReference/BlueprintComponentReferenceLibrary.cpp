// Copyright 2024, Aquanox.


#include "BlueprintComponentReferenceLibrary.h"

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

template<typename TBase, typename TContainer>
void ResolveComponentArrayInternal(TBase References, AActor* Actor, UClass* Class, bool bKeepNulls, TContainer Components)
{
	Components.Empty();

	for (const auto& Reference : References)
	{
		UActorComponent* Component = nullptr;
		ResolveComponentInternal(Reference, Actor, Class, Component);
		
		if (Component != nullptr || bKeepNulls)
		{
			Components.Add(Component);
		}
	}
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
	ResolveComponentArrayInternal(References, Actor, Class, bKeepNulls, Components);
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
