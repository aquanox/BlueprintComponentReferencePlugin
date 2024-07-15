// Copyright 2024, Aquanox.


#include "BlueprintComponentReferenceUtils.h"

bool UBlueprintComponentReferenceUtils::ResolveComponentReference(const FBlueprintComponentReference& Reference, AActor* Actor, UActorComponent*& Component)
{
	if (!Reference.IsNull())
	{
		Component = Reference.GetComponent(Actor);
	}
	return IsValid(Component);
}

bool UBlueprintComponentReferenceUtils::ResolveComponentReferenceOfType(const FBlueprintComponentReference& Reference, AActor* Actor, TSubclassOf<UActorComponent> Class, UActorComponent*& Component)
{
	Component = nullptr;

	UActorComponent* Result = Reference.GetComponent(Actor);
	if (IsValid(Result) && (!Class || Result->IsA(Class)))
	{
		Component = Result;
	}

	return IsValid(Component);
}

void UBlueprintComponentReferenceUtils::ResolveComponentReferenceArray(const TArray<FBlueprintComponentReference>& References, AActor* Actor, TArray<UActorComponent*>& Components)
{
	for (const FBlueprintComponentReference& Reference : References)
	{
		UActorComponent* Component = nullptr;
		if (!Reference.IsNull())
		{
			Component = Reference.GetComponent(Actor);
		}
		if (IsValid(Component))
		{
			Components.Add(Component);
		}
	}
}

void UBlueprintComponentReferenceUtils::ResolveComponentReferenceArrayOfType(const TArray<FBlueprintComponentReference>& References, AActor* Actor, TSubclassOf<UActorComponent> Class, TArray<UActorComponent*>& Components)
{
	for (const FBlueprintComponentReference& Reference : References)
	{
		UActorComponent* Component = nullptr;
		if (!Reference.IsNull())
		{
			Component = Reference.GetComponent(Actor);
		}
		if (IsValid(Component) && (!Class || Component->IsA(Class)))
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
