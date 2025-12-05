// Copyright 2024, Aquanox.

#include "BlueprintComponentReference.h"
#include "Modules/ModuleManager.h"
#include "Components/ActorComponent.h"
#include "Misc/EngineVersionComparison.h"
#include "Engine/EngineTypes.h"
#include "GameFramework/Actor.h"
#include "Serialization/StructuredArchive.h"
#include "UObject/PropertyTag.h"

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
	switch (Mode)
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

UActorComponent* FBlueprintComponentReference::GetComponent(AActor* SearchActor) const
{
	UActorComponent* Result = nullptr;

	if (SearchActor)
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
			Result = FindObjectFast<UActorComponent>(SearchActor, Value);
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

bool FBlueprintComponentReference::SerializeFromMismatchedTag(const FPropertyTag& Tag, FStructuredArchive::FSlot Slot)
{
	static const FName RootComponentReferencePropertyName = TEXT("RootComponent");

	// migration of Engine CR -> BCR, drops actor as context will be determined by detail customization
	static const FName EngineComponentReferenceContextName("ComponentReference");
#if UE_VERSION_OLDER_THAN(5, 0, 0)
	if (Tag.Type == EngineComponentReferenceContextName)
#else
	if (Tag.GetType().IsStruct(EngineComponentReferenceContextName))
#endif
	{
		FComponentReference Reference;
		FComponentReference::StaticStruct()->SerializeItem(Slot, &Reference, nullptr);
		if (!Reference.ComponentProperty.IsNone())
		{
			Mode = EBlueprintComponentReferenceMode::Property;
			Value = Reference.ComponentProperty;
		}
		else if (!Reference.PathToComponent.IsEmpty())
		{
			Mode = EBlueprintComponentReferenceMode::Path;
			Value = *Reference.PathToComponent;
		}
#if UE_VERSION_OLDER_THAN(5, 0, 0)
		else if (Reference.OtherActor)
#else
		else if (Reference.OtherActor.IsValid())
#endif
		{
			Mode = EBlueprintComponentReferenceMode::Property;
			Value = RootComponentReferencePropertyName;
		}
		return true;
	}

#if UE_VERSION_NEWER_THAN(5, 1, 0)
	// migration of UE5+ SoftCR -> BCR, drops actor as context will be determined by detail customization
	static const FName EngineSoftComponentReferenceContextName("SoftComponentReference");
	if (Tag.GetType().IsStruct(EngineSoftComponentReferenceContextName))
	{
		FSoftComponentReference Reference;
		FSoftComponentReference::StaticStruct()->SerializeItem(Slot, &Reference, nullptr);
		if (!Reference.ComponentProperty.IsNone())
		{
			Mode = EBlueprintComponentReferenceMode::Property;
			Value = Reference.ComponentProperty;
		}
		else if (!Reference.PathToComponent.IsEmpty())
		{
			Mode = EBlueprintComponentReferenceMode::Path;
			Value = *Reference.PathToComponent;
		}
		else if (Reference.OtherActor.IsValid())
		{
			Mode = EBlueprintComponentReferenceMode::Property;
			Value = RootComponentReferencePropertyName;
		}
		return true;
	}
#endif

	return false;
}
