// Copyright 2024, Aquanox.


#include "BlueprintComponentReferenceUtils.h"

inline bool TestComponent(UActorComponent* In, UClass* InClass)
{
	return InClass ? IsValid(In) && In->IsA(InClass) : IsValid(In);
}

inline bool ResolveComponentInternal(const FBlueprintComponentReference& Reference, AActor* Actor, UClass* Class, UActorComponent*& Component)
{
	UActorComponent* Result = Reference.GetComponent(Actor);
	if (TestComponent(Result, Class))
	{
		Component = Result;
	}
	return IsValid(Component);
}

bool UBlueprintComponentReferenceUtils::GetReferencedComponent(const FBlueprintComponentReference& Reference, AActor* Actor, UActorComponent*& Component)
{
	return ResolveComponentInternal(Reference, Actor, nullptr, Component);
}

bool UBlueprintComponentReferenceUtils::GetReferencedComponentOfType(const FBlueprintComponentReference& Reference, AActor* Actor,TSubclassOf<UActorComponent> Class, UActorComponent*& Component)
{
	return ResolveComponentInternal(Reference, Actor, Class, Component);
}

bool UBlueprintComponentReferenceUtils::TryGetReferencedComponent(const FBlueprintComponentReference& Reference, AActor* Actor, UActorComponent*& Component)
{
	return ResolveComponentInternal(Reference, Actor, nullptr, Component);
}

bool UBlueprintComponentReferenceUtils::TryGetReferencedComponentOfType(const FBlueprintComponentReference& Reference, AActor* Actor, TSubclassOf<UActorComponent> Class, UActorComponent*& Component)
{
	return ResolveComponentInternal(Reference, Actor, Class, Component);
}

void UBlueprintComponentReferenceUtils::TryGetReferencedComponents(const TArray<FBlueprintComponentReference>& References, AActor* Actor, bool bAllowNull, TArray<UActorComponent*>& Components)
{
	TryGetReferencedComponentsOfType(References, Actor, nullptr, bAllowNull, Components);
}

void UBlueprintComponentReferenceUtils::TryGetReferencedComponentsOfType(const TArray<FBlueprintComponentReference>& References, AActor* Actor, TSubclassOf<UActorComponent> Class, bool bAllowNull, TArray<UActorComponent*>& Components)
{
	Components.Empty();

	for (const FBlueprintComponentReference& Reference : References)
	{
		UActorComponent* Component = nullptr;
		if (ResolveComponentInternal(Reference, Actor, Class, Component) || bAllowNull)
		{
			Components.Add(Component);
		}
	}
}

bool UBlueprintComponentReferenceUtils::IsNullReference(const FBlueprintComponentReference& Reference)
{
	return Reference.IsNull();
}

FBlueprintComponentReference UBlueprintComponentReferenceUtils::MakeLiteralComponentReference(EBlueprintComponentReferenceMode Mode, FName Value)
{
	return FBlueprintComponentReference(Mode, Value);
}

void UBlueprintComponentReferenceUtils::BreakLiteralComponentReference(const FBlueprintComponentReference& Reference, EBlueprintComponentReferenceMode& Mode, FName& Value)
{
	Mode = Reference.GetMode();
	Value = Reference.GetValue();
}

bool UBlueprintComponentReferenceUtils::EqualEqual_ComponentReference(const FBlueprintComponentReference& A, const FBlueprintComponentReference& B)
{
	return A == B;
}

bool UBlueprintComponentReferenceUtils::NotEqual_ComponentReference(const FBlueprintComponentReference& A, const FBlueprintComponentReference& B)
{
	return A != B;
}

FString UBlueprintComponentReferenceUtils::Conv_ComponentReferenceToString(const FBlueprintComponentReference& Reference)
{
	return Reference.GetValueString(true);
}
