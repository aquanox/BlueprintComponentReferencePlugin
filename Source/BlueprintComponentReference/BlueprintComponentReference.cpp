// Copyright 2024, Aquanox.

#include "BlueprintComponentReference.h"
#include "Modules/ModuleManager.h"
#include "Components/ActorComponent.h"
#include "Misc/EngineVersionComparison.h"
#include "GameFramework/Actor.h"

IMPLEMENT_MODULE(FDefaultModuleImpl, BlueprintComponentReference);

FBlueprintComponentReference::FBlueprintComponentReference()
	: Mode(EBlueprintComponentReferenceMode::None)
{
}

FBlueprintComponentReference::FBlueprintComponentReference(const FString& InValue)
	: Mode(EBlueprintComponentReferenceMode::None)
{
	ParseString(InValue);
}

FBlueprintComponentReference::FBlueprintComponentReference(EBlueprintComponentReferenceMode InMode, const FName& InValue)
	: Mode(InMode), Value(InValue)
{

}

bool FBlueprintComponentReference::ParseString(const FString& InValue)
{
	FString ParsedMode, ParsedValue;
	if (InValue.Split(TEXT(":"), &ParsedMode, &ParsedValue, ESearchCase::CaseSensitive))
	{
		if (ParsedMode.Equals(TEXT("property"), ESearchCase::IgnoreCase))
		{
			Mode = EBlueprintComponentReferenceMode::Property;
			Value = *ParsedValue;
			return true;
		}
		if (ParsedMode.Equals(TEXT("path"), ESearchCase::IgnoreCase))
        {
        	Mode = EBlueprintComponentReferenceMode::Path;
			Value = *ParsedValue;
			return true;
        }
	}
	else if (!InValue.IsEmpty())
	{
		Mode = EBlueprintComponentReferenceMode::Property;
		Value = *InValue;
		return true;
	}

	return false;
}

FString FBlueprintComponentReference::ToString() const
{
	TStringBuilder<32> Result;
	switch(Mode)
	{
	case EBlueprintComponentReferenceMode::Property:
		Result.Append(TEXT("property:"));
		Result.Append(Value.ToString());
		break;
	case EBlueprintComponentReferenceMode::Path:
		Result.Append(TEXT("path:"));
		Result.Append(Value.ToString());
		break;
	default:
		break;
	}
	return Result.ToString();
}

UActorComponent* FBlueprintComponentReference::GetComponent(AActor* SearchActor, bool bFallbackToRoot) const
{
	UActorComponent* Result = nullptr;

	if(SearchActor)
	{
		// Variation 1: property name
		if (Mode == EBlueprintComponentReferenceMode::Property)
		{
			if(FObjectPropertyBase* ObjProp = FindFProperty<FObjectPropertyBase>(SearchActor->GetClass(), Value))
			{
				Result = Cast<UActorComponent>(ObjProp->GetObjectPropertyValue_InContainer(SearchActor));
			}
		}
		// Variation 2: subobject path
		else if (Mode == EBlueprintComponentReferenceMode::Path)
		{
			Result = FindObject<UActorComponent>(SearchActor, *Value.ToString());
		}
		// Fallback compatibility mode with engine references
		if (!Result && bFallbackToRoot)
		{
			Result = SearchActor->GetRootComponent();
		}
	}

	return Result;
}

bool FBlueprintComponentReference::IsNull() const
{
	return Value.IsNone() && Mode == EBlueprintComponentReferenceMode::None;
}

void FBlueprintComponentReference::Invalidate()
{
	Mode = EBlueprintComponentReferenceMode::None;
	Value = NAME_None;
}
