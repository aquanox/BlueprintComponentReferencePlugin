// Copyright 2024, Aquanox.

#include "BlueprintComponentReference.h"
#include "Modules/ModuleManager.h"
#include "Components/ActorComponent.h"
#include "Misc/OutputDevice.h"
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
	SetValueFromString(InValue);
}

FBlueprintComponentReference::FBlueprintComponentReference(EBlueprintComponentReferenceMode InMode, const FName& InValue)
	: Mode(InMode), Value(InValue)
{

}

void FBlueprintComponentReference::SetValueFromString(const FString& InValue)
{
	Mode = EBlueprintComponentReferenceMode::None;
	Value = NAME_None;

	if (InValue.StartsWith(TEXT("path:"), ESearchCase::IgnoreCase))
	{
		Mode = EBlueprintComponentReferenceMode::Path;
		Value = *InValue.RightChop(5);
	}
	else if (InValue.StartsWith(TEXT("var:"), ESearchCase::IgnoreCase))
	{
		Mode = EBlueprintComponentReferenceMode::Property;
		Value = *InValue.RightChop(4);
	}
	else if (InValue.StartsWith(TEXT("prop:"), ESearchCase::IgnoreCase))
	{
		Mode = EBlueprintComponentReferenceMode::Property;
		Value = *InValue.RightChop(5);
	}
	else if (!InValue.IsEmpty())
	{
		Mode = EBlueprintComponentReferenceMode::Property;
		Value = *InValue;
	}
}

FString FBlueprintComponentReference::GetValueString(bool bIncludeMode) const
{
	if (bIncludeMode)
	{
		FString Result;
		switch(Mode)
		{
		case EBlueprintComponentReferenceMode::Property:
			Result.Append(TEXT("var:"));
			Result.Append(Value.ToString());
			break;
		case EBlueprintComponentReferenceMode::Path:
			Result.Append(TEXT("path:"));
			Result.Append(Value.ToString());
			break;
		default:
			break;
		}
		return Result;
	}
	else
	{
		return Value.IsNone() ? TEXT("") : Value.ToString();
	}
}

void FBlueprintComponentReference::Reset()
{
	Mode = EBlueprintComponentReferenceMode::Property;
	Value = NAME_None;
}

UActorComponent* FBlueprintComponentReference::GetComponent(AActor* SearchActor, bool bFallbackToRoot) const
{
	UActorComponent* Result = nullptr;

	if(IsValid(SearchActor))
	{
		// Variation 1: property name
		if(Mode == EBlueprintComponentReferenceMode::Property)
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
	return Value.IsNone() || Mode == EBlueprintComponentReferenceMode::None;
}

bool FBlueprintComponentReference::ImportTextItem(const TCHAR*& Buffer, int32 PortFlags, UObject* Parent, FOutputDevice* ErrorText)
{
	FString ImportedString = TEXT("");
	const TCHAR* NewBuffer = FPropertyHelpers::ReadToken(Buffer, ImportedString, true);

	if (!NewBuffer)
	{
		return false;
	}

	SetValueFromString(ImportedString);
	Buffer = NewBuffer;

	return true;
}

bool FBlueprintComponentReference::ExportTextItem(FString& ValueStr, const FBlueprintComponentReference& DefaultValue, UObject* Parent, int32 PortFlags, UObject* ExportRootScope) const
{
	if (!(PortFlags & PPF_Delimited))
	{
		ValueStr += GetValueString(true);
	}
	else
	{
		ValueStr += FString::Printf(TEXT("\"%s\""), *GetValueString(true).ReplaceCharWithEscapedChar());
	}

	return true;
}

bool FBlueprintComponentReference::SerializeFromMismatchedTag(const FPropertyTag& Tag, FStructuredArchive::FSlot Slot)
{
	static const FName ComponentReferenceContextName("BlueprintComponentReference");
#if UE_VERSION_OLDER_THAN(5,4,0)
	if (Tag.Type == NAME_StructProperty && Tag.StructName == ComponentReferenceContextName)
#else
	if (Tag.GetType().IsStruct(ComponentReferenceContextName))
#endif
	{
		FBlueprintComponentReference Reference;
		FBlueprintComponentReference::StaticStruct()->SerializeItem(Slot, &Reference, nullptr);
		if (!Reference.IsNull())
		{
			Mode = Reference.Mode;
			Value = Reference.Value;
		}
		return true;
	}

	return false;

}
