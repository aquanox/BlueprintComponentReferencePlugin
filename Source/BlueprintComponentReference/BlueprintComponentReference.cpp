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
		if (ParsedMode.Equals(TEXT("property"), ESearchCase::IgnoreCase)
			|| ParsedMode.Equals(TEXT("var"), ESearchCase::IgnoreCase)) // legacy compat
		{
			Mode = EBlueprintComponentReferenceMode::Property;
			Value = *ParsedValue.TrimEnd();
			return true;
		}
		if (ParsedMode.Equals(TEXT("path"), ESearchCase::IgnoreCase))
        {
        	Mode = EBlueprintComponentReferenceMode::Path;
			Value = *ParsedValue.TrimEnd();
			return true;
        }
	}
	else if (!InValue.IsEmpty())
	{
		Mode = EBlueprintComponentReferenceMode::Property;
		Value = *InValue.TrimStartAndEnd();
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

UActorComponent* FBlueprintComponentReference::GetComponent(const AActor* SearchActor) const
{
	UActorComponent* Result = nullptr;

	if(SearchActor)
	{
		switch (Mode)
		{
		case EBlueprintComponentReferenceMode::Property:
			// Variation 1: property name
			if(FObjectPropertyBase* ObjProp = FindFProperty<FObjectPropertyBase>(SearchActor->GetClass(), Value))
			{
				Result = Cast<UActorComponent>(ObjProp->GetObjectPropertyValue_InContainer(SearchActor));
			}
			break;
		case EBlueprintComponentReferenceMode::Path:
			// Variation 2: subobject path
			// const-cast is used as FindObjectFast does not provide a signature with const AActor*
			Result = FindObjectFast<UActorComponent>(const_cast<AActor*>SearchActor, Value);
			break;
		//case EBlueprintComponentReferenceMode::Dynamic:
			// Variation 3: dynamic selection
			//break;
		case EBlueprintComponentReferenceMode::None:
		default:
			break;
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
