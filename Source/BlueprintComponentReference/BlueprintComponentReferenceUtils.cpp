// Copyright 2024, Aquanox.


#include "BlueprintComponentReferenceUtils.h"

UActorComponent* UBlueprintComponentReferenceUtils::ResolveComponentReference(const FBlueprintComponentReference& Reference, AActor* Actor)
{
	return Reference.GetComponent(Actor);
}

void UBlueprintComponentReferenceUtils::TryResolveComponentReference(const FBlueprintComponentReference& Reference, AActor* Actor, TSubclassOf<UActorComponent> Class, UActorComponent*& Component)
{
	Component = nullptr;

	UActorComponent* Result = Reference.GetComponent(Actor);
	if (::IsValid(Result) && (!Class || Result->IsA(Class)))
	{
		Component = Result;
	}
}

void UBlueprintComponentReferenceUtils::SetComponentReference_FromName(FBlueprintComponentReference& Reference, AActor* Actor, FName PropertyName)
{
	Reference.SetValueFromString(PropertyName.ToString());
}

void UBlueprintComponentReferenceUtils::SetComponentReference_FromComponent(FBlueprintComponentReference& Reference, UActorComponent* Component)
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
