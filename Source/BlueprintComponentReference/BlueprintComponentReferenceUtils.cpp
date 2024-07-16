// Copyright 2024, Aquanox.


#include "BlueprintComponentReferenceUtils.h"

bool UBlueprintComponentReferenceUtils::ResolveComponentReference(const FBlueprintComponentReference& Reference, AActor* Actor, UActorComponent*& Component)
{
	return ResolveComponentReferenceOfType(Reference, Actor, UActorComponent::StaticClass(), Component);
}

bool UBlueprintComponentReferenceUtils::ResolveComponentReferenceOfType(const FBlueprintComponentReference& Reference, AActor* Actor, TSubclassOf<UActorComponent> Class, UActorComponent*& Component)
{
	Component = nullptr;

	UActorComponent* Result = Reference.GetComponent(Actor);
	if (IsValid(Result) && Result->IsA(Class))
	{
		Component = Result;
	}

	return IsValid(Component);
}

void UBlueprintComponentReferenceUtils::ResolveComponentReferenceArray(const TArray<FBlueprintComponentReference>& References, AActor* Actor, bool bPreserveOrder, TArray<UActorComponent*>& Components)
{
	ResolveComponentReferenceArrayOfType(References, Actor, UActorComponent::StaticClass(), bPreserveOrder, Components);
}

void UBlueprintComponentReferenceUtils::ResolveComponentReferenceArrayOfType(const TArray<FBlueprintComponentReference>& References, AActor* Actor, TSubclassOf<UActorComponent> Class, bool bPreserveOrder, TArray<UActorComponent*>& Components)
{
	Components.Empty();

	for (const FBlueprintComponentReference& Reference : References)
	{
		UActorComponent* Component = Reference.GetComponent(Actor);
		Component = (!Class || Component->IsA(Class)) ? Component : nullptr;
		if (IsValid(Component) || bPreserveOrder)
		{
			Components.Add(Component);
		}
	}
}

bool UBlueprintComponentReferenceUtils::IsNullReference(const FBlueprintComponentReference& Reference)
{
	return Reference.IsNull();
}

void UBlueprintComponentReferenceUtils::SetReferencFromName(FBlueprintComponentReference& Reference, AActor* Actor, FName PropertyName)
{
	Reference = FBlueprintComponentReference(EBlueprintComponentReferenceMode::VariableName, PropertyName);
}

void UBlueprintComponentReferenceUtils::SetReferenceFromObjectPath(FBlueprintComponentReference& Reference, AActor* Actor, FString ObjectPath)
{
	Reference = FBlueprintComponentReference(EBlueprintComponentReferenceMode::ObjectPath, *ObjectPath);
}

void UBlueprintComponentReferenceUtils::SetReferenceFromComponent(FBlueprintComponentReference& Reference, UActorComponent* Component)
{
	if (!Component)
	{
		Reference.Reset();
	}
	else
	{
		Reference.SetValueFromString(Component->GetName());
	}
}
