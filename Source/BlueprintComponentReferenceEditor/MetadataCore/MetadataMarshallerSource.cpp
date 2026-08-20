// Copyright 2024, Aquanox.

#include  "MetadataMarshallerSource.h"

#include "BlueprintComponentReferenceHelper.h"
#include "MetadataMarshallerContainer.h"
#include "Engine/Blueprint.h"
#include "Templates/TypeHash.h"
#include "Misc/EngineVersionComparison.h"

namespace
{
	FString NormalizeClassName(const FString& InName)
	{
		return InName;
	}
}

TOptional<bool> FMetadataSettingsSource::GetBooleanValue(const FName& InName, bool IfEmpty) const
{
	TOptional<FString> ValueStr = GetStringValue(InName);
	if (ValueStr.IsSet())
	{
		const FString& ValueString = *ValueStr;
		if (ValueString.Equals(TEXT("true"), ESearchCase::IgnoreCase))
		{
			return TOptional<bool>(true);
		}
		else if (ValueString.Equals(TEXT("false"), ESearchCase::IgnoreCase))
		{
			return TOptional<bool>(false);
		}
		return TOptional<bool>(IfEmpty);
	}
	return TOptional<bool>();
}

TOptional<bool> FMetadataSettingsSource::GetFlagValue(const FName& InName) const
{
	if (HasValue(InName))
	{ // content does not matter for flag, even if =false it is true that flag is present
		return TOptional<bool>(true);
	}
	return TOptional<bool>();
}

TOptional<int64> FMetadataSettingsSource::GetEnumValue(const UEnum* Enum, const FName& InName) const
{
	TOptional<FString> ValueStr = GetStringValue(InName);
	if (ValueStr.IsSet())
	{
		int64 Value = Enum->GetValueByNameString(*ValueStr);
		// note: invalid enum value treated as not set
		if (Value != INDEX_NONE)
		{
			return TOptional<int64>(Value);
		}
	}
	return TOptional<int64>();
}

TOptional<FSoftClassPath> FMetadataSettingsSource::GetLazyClassValue(const FName& InName) const
{
	TOptional<FString> ValueStr = GetStringValue(InName);
	if (ValueStr.IsSet())
	{
		FString NormalizedClassPath = NormalizeClassName(*ValueStr);
		if (UClass* Class = FBlueprintComponentReferenceHelper::FindClassByName(NormalizedClassPath))
		{
			return TOptional<FSoftClassPath>(FSoftClassPath(Class));
		}
		// unresolved class
		return TOptional<FSoftClassPath>(FSoftClassPath(NormalizedClassPath));
	}
	return TOptional<FSoftClassPath>();
}

TOptional<TArray<FSoftClassPath>> FMetadataSettingsSource::GetLazyClassListValue(const FName& InName) const
{
	TOptional<FString> ValueStr = GetStringValue(InName);
	if (ValueStr.IsSet())
	{
		TArray<FString> ClassFilterNames;
		(*ValueStr).ParseIntoArrayWS(ClassFilterNames, TEXT(","), true);

		TArray<FSoftClassPath> Result;
		for (auto& ClassName : ClassFilterNames)
		{
			ClassName = NormalizeClassName(ClassName);
			if (UClass* Class = FBlueprintComponentReferenceHelper::FindClassByName(ClassName))
			{
				Result.Add(FSoftClassPath(Class));
			}
			else
			{
				Result.Add(FSoftClassPath(ClassName));
			}
		}
		return TOptional<TArray<FSoftClassPath>>(Result);
	}
	return TOptional<TArray<FSoftClassPath>>();
}

void FMetadataSettingsSource::SetBooleanValue(const FName& InName, TOptional<bool> InValue)
{
	TOptional<FString> AsString;
	if (InValue.IsSet())
	{
		AsString = (InValue.GetValue() ? TEXT("True") : TEXT("False"));
	}
	SetStringValue(InName, AsString);
}

void FMetadataSettingsSource::SetFlagValue(const FName& InName, TOptional<bool> InValue)
{
	TOptional<FString> AsString;
	if (InValue.IsSet())
	{
		AsString = TEXT("");
	}
	SetStringValue(InName, AsString);
}

void FMetadataSettingsSource::SetLazyClassValue(const FName& InName, TOptional<FSoftClassPath> InValue)
{
	TOptional<FString> AsString;
	if (InValue.IsSet())
	{
		AsString = (*InValue).ToString();
	}
	SetStringValue(InName, AsString);
}

void FMetadataSettingsSource::SetLazyClassListValue(const FName& InName, TOptional<TArray<FSoftClassPath>> InValue)
{
	TOptional<FString> AsString;
	if (InValue.IsSet())
	{
		TArray<FString, TInlineAllocator<8>> Paths;
		for (const auto& Class : *InValue)
		{
			Paths.AddUnique(Class.ToString());
		}
		AsString = FString::Join(Paths, TEXT(","));
	}
	SetStringValue(InName, AsString);
}

bool FMetadataSettingsPropertySource::HasValue(const FName& InName) const
{
	return Source->FindMetaData(InName) != nullptr;
}

void FMetadataSettingsPropertySource::UnsetValue(const FName& InName)
{
	if (IsEditable())
	{
		SetStringValue(InName, TOptional<FString>());
	}
}

TOptional<FString> FMetadataSettingsPropertySource::GetStringValue(const FName& InName) const
{
	if (const FString* Value = Source->FindMetaData(InName))
	{
		return *Value;
	}
	return TOptional<FString>();
}

void FMetadataSettingsPropertySource::SetStringValue(const FName& InName, TOptional<FString> InValue) const
{
	if (!IsEditable()) return;

	UBlueprint* const InBlueprint = const_cast<UBlueprint*>(Context);
	FProperty* const Target = const_cast<FProperty*>(Source);
	if (::IsValid(InBlueprint))
	{
		for (FBPVariableDescription& VariableDescription : InBlueprint->NewVariables)
		{
			if (VariableDescription.VarName == Source->GetFName())
			{
				if (InValue.IsSet())
				{
					Target->SetMetaData(InName, *InValue.GetValue());
					VariableDescription.SetMetaData(InName, InValue.GetValue());
				}
				else
				{
					Target->RemoveMetaData(InName);
					VariableDescription.RemoveMetaData(InName);
				}

				InBlueprint->Modify();
				break;
			}
		}
	}
	else
	{
		if (InValue.IsSet())
		{
			Target->SetMetaData(InName, *InValue.GetValue());
		}
		else
		{
			Target->RemoveMetaData(InName);
		}
	}
}

bool FMetadataSettingsTypeSource::HasValue(const FName& InName) const
{
	return Source->FindMetaData(InName) != nullptr;
}

TOptional<FString> FMetadataSettingsTypeSource::GetStringValue(const FName& InName) const
{
	// Exact search
	if (const FString* Value = Source->FindMetaData(InName))
	{
		return *Value;
	}
	// Hierarchical search
	for (const UStruct* TestStruct = Source; TestStruct != nullptr; TestStruct = TestStruct->GetSuperStruct())
	{
		if (const FString* FoundMetaData = TestStruct->FindMetaData(InName))
		{
			return *FoundMetaData;
		}
	}
	return TOptional<FString>();
}
