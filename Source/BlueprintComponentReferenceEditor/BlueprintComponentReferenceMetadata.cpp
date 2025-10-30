// Copyright 2024, Aquanox.

#include "BlueprintComponentReferenceMetadata.h"
#include "BlueprintComponentReferenceHelper.h"
#include "BlueprintComponentReferenceEditor.h"
#include "Engine/Blueprint.h"
#include "Templates/TypeHash.h"
#include "Misc/EngineVersionComparison.h"

#include "UObject/UObjectIterator.h"

const FName FCRMetadataKey::ActorClass = "ActorClass";
const FName FCRMetadataKey::AllowedClasses = "AllowedClasses";
const FName FCRMetadataKey::DisallowedClasses = "DisallowedClasses";
const FName FCRMetadataKey::NoClear = "NoClear";
const FName FCRMetadataKey::NoNavigate = "NoNavigate";
const FName FCRMetadataKey::NoPicker = "NoPicker";
const FName FCRMetadataKey::ShowBlueprint = "ShowBlueprint";
const FName FCRMetadataKey::ShowNative = "ShowNative";
const FName FCRMetadataKey::ShowInstanced = "ShowInstanced";
const FName FCRMetadataKey::ShowHidden = "ShowHidden";
const FName FCRMetadataKey::ShowEditor = "ShowEditor";

void FBlueprintComponentReferenceMetadata::ResetSettings()
{
	static const FBlueprintComponentReferenceMetadata DefaultValues;
	*this = DefaultValues;
}

void FBlueprintComponentReferenceMetadata::LoadSettingsFromProperty(const FProperty* InProp)
{
	UE_LOG(LogComponentReferenceEditor, Verbose, TEXT("LoadSettingsFromProperty(%s)"), *InProp->GetFName().ToString());

	static const FBlueprintComponentReferenceMetadata DefaultValues;

	// picker
	bUsePicker = !FMetadataMarshaller::HasMetaDataValue(InProp, FCRMetadataKey::NoPicker);
	// actions
	bUseNavigate = !FMetadataMarshaller::HasMetaDataValue(InProp, FCRMetadataKey::NoNavigate);
	bUseClear = !(InProp->PropertyFlags & CPF_NoClear) && !FMetadataMarshaller::HasMetaDataValue(InProp, FCRMetadataKey::NoClear);
	// filters
	bShowNative = FMetadataMarshaller::GetBoolMetaDataValue(InProp, FCRMetadataKey::ShowNative).Get(DefaultValues.bShowNative);
	bShowBlueprint = FMetadataMarshaller::GetBoolMetaDataValue(InProp, FCRMetadataKey::ShowBlueprint).Get(DefaultValues.bShowBlueprint);
	bShowInstanced = FMetadataMarshaller::GetBoolMetaDataValue(InProp, FCRMetadataKey::ShowInstanced).Get(DefaultValues.bShowInstanced);
	bShowHidden = FMetadataMarshaller::GetBoolMetaDataValue(InProp, FCRMetadataKey::ShowHidden).Get(DefaultValues.bShowHidden);
	bShowEditor = FMetadataMarshaller::GetBoolMetaDataValue(InProp, FCRMetadataKey::ShowEditor).Get( DefaultValues.bShowEditor);

	FMetadataMarshaller::GetClassMetadata(InProp, FCRMetadataKey::ActorClass, [this](UClass* InClass)
	{
		ActorClass = InClass;
	});

	FMetadataMarshaller::GetClassListMetadata(InProp, FCRMetadataKey::AllowedClasses, [this](UClass* InClass)
	{
		AllowedClasses.AddUnique(InClass);
	});

	FMetadataMarshaller::GetClassListMetadata(InProp, FCRMetadataKey::DisallowedClasses, [this](UClass* InClass)
	{
		DisallowedClasses.AddUnique(InClass);
	});
}

void FBlueprintComponentReferenceMetadata::ApplySettingsToProperty(UBlueprint* InBlueprint, FProperty* InProperty, const FName& InChanged)
{
	UE_LOG(LogComponentReferenceEditor, Verbose, TEXT("ApplySettingsToProperty(%s)"), *InProperty->GetName());

	auto BoolToString = [](bool b) ->  TOptional<FString>
	{
		return TOptional<FString>(b ? TEXT("True") : TEXT("False"));
	};

	auto BoolToFlag = [](bool b) ->  TOptional<FString>
	{
		return b ? TOptional<FString>(TEXT("")): TOptional<FString>();
	};

	auto ClassToString = [](const UClass* InClass) ->  TOptional<FString>
	{
		if (!IsValid(InClass))
		{
			return TOptional<FString>();
		}
		return FString::Printf(TEXT("%s.%s"), *InClass->GetOuter()->GetFName().ToString(), *InClass->GetFName().ToString());
	};

	auto ArrayToString = [ClassToString](const TArray<TSubclassOf<UActorComponent>>& InArray)
	{
		TArray<FString, TInlineAllocator<8>> Paths;
		for (const TSubclassOf<UActorComponent>& Class : InArray)
		{
			TOptional<FString> Result = ClassToString(Class);
			if (Result.IsSet())
			{
				Paths.AddUnique(Result.GetValue());
			}
		}
		return FString::Join(Paths, TEXT(","));
	};

	if (InChanged.IsNone() || InChanged == GET_MEMBER_NAME_CHECKED(FBlueprintComponentReferenceMetadata, bUsePicker))
	{
		FMetadataMarshaller::SetMetaDataValue(InBlueprint, InProperty, FCRMetadataKey::NoPicker, BoolToFlag(!bUsePicker));
	}
	if (InChanged.IsNone() || InChanged == GET_MEMBER_NAME_CHECKED(FBlueprintComponentReferenceMetadata, bUseNavigate))
	{
		FMetadataMarshaller::SetMetaDataValue(InBlueprint, InProperty, FCRMetadataKey::NoNavigate, BoolToFlag(!bUseNavigate));
	}
	if (InChanged.IsNone() || InChanged == GET_MEMBER_NAME_CHECKED(FBlueprintComponentReferenceMetadata, bUseClear))
	{
		FMetadataMarshaller::SetMetaDataValue(InBlueprint, InProperty, FCRMetadataKey::NoClear, BoolToFlag(!bUseClear));
	}

	if (InChanged.IsNone() || InChanged == GET_MEMBER_NAME_CHECKED(FBlueprintComponentReferenceMetadata, bShowNative))
	{
		FMetadataMarshaller::SetMetaDataValue(InBlueprint, InProperty, FCRMetadataKey::ShowNative, BoolToString(bShowNative));
	}
	if (InChanged.IsNone() || InChanged == GET_MEMBER_NAME_CHECKED(FBlueprintComponentReferenceMetadata, bShowBlueprint))
	{
		FMetadataMarshaller::SetMetaDataValue(InBlueprint, InProperty, FCRMetadataKey::ShowBlueprint, BoolToString(bShowBlueprint));
	}
	if (InChanged.IsNone() || InChanged == GET_MEMBER_NAME_CHECKED(FBlueprintComponentReferenceMetadata, bShowInstanced))
	{
		FMetadataMarshaller::SetMetaDataValue(InBlueprint, InProperty, FCRMetadataKey::ShowInstanced, BoolToString(bShowInstanced));
	}
	if (InChanged.IsNone() || InChanged == GET_MEMBER_NAME_CHECKED(FBlueprintComponentReferenceMetadata, bShowHidden))
	{
		FMetadataMarshaller::SetMetaDataValue(InBlueprint, InProperty, FCRMetadataKey::ShowHidden, BoolToString(bShowHidden));
	}
	if (InChanged.IsNone() || InChanged == GET_MEMBER_NAME_CHECKED(FBlueprintComponentReferenceMetadata, bShowEditor))
	{
		FMetadataMarshaller::SetMetaDataValue(InBlueprint, InProperty, FCRMetadataKey::ShowEditor, BoolToString(bShowEditor));
	}
	if (InChanged.IsNone() || InChanged == GET_MEMBER_NAME_CHECKED(FBlueprintComponentReferenceMetadata, AllowedClasses))
	{
		FMetadataMarshaller::SetMetaDataValue(InBlueprint, InProperty, FCRMetadataKey::AllowedClasses, ArrayToString(AllowedClasses));
	}
	if (InChanged.IsNone() || InChanged == GET_MEMBER_NAME_CHECKED(FBlueprintComponentReferenceMetadata, DisallowedClasses))
	{
		FMetadataMarshaller::SetMetaDataValue(InBlueprint, InProperty, FCRMetadataKey::DisallowedClasses, ArrayToString(DisallowedClasses));
	}
	if (InChanged.IsNone() || InChanged == GET_MEMBER_NAME_CHECKED(FBlueprintComponentReferenceMetadata, ActorClass))
	{
		FMetadataMarshaller::SetMetaDataValue(InBlueprint, InProperty, FCRMetadataKey::ActorClass, ClassToString(ActorClass.Get()));
	}
}

bool FMetadataMarshaller::HasMetaDataValue(const FProperty* Property, const FName& InName)
{
	return Property->HasMetaData(InName);
}

void FMetadataMarshaller::SetMetaDataValue(UBlueprint* InBlueprint, FProperty* InProperty, const FName& InName, TOptional<FString> InValue)
{
	check(InProperty);

	if (::IsValid(InBlueprint))
	{
		for (FBPVariableDescription& VariableDescription : InBlueprint->NewVariables)
		{
			if (VariableDescription.VarName == InProperty->GetFName())
			{
				if (InValue.IsSet())
				{
					InProperty->SetMetaData(InName, *InValue.GetValue());
					VariableDescription.SetMetaData(InName, InValue.GetValue());
				}
				else
				{
					InProperty->RemoveMetaData(InName);
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
			InProperty->SetMetaData(InName, *InValue.GetValue());
		}
		else
		{
			InProperty->RemoveMetaData(InName);
		}
	}
}

TOptional<bool> FMetadataMarshaller::GetBoolMetaDataValue(const FProperty* Property, const FName& InName)
{
	if (Property->FindMetaData(InName) != nullptr)
	{
		bool bResult = true;

		const FString& ValueString = Property->GetMetaData(InName);
		if (!ValueString.IsEmpty())
		{
			if (ValueString.Equals(TEXT("true"), ESearchCase::IgnoreCase))
			{
				bResult = true;
			}
			else if (ValueString.Equals(TEXT("false"), ESearchCase::IgnoreCase))
			{
				bResult = false;
			}
		}

		return TOptional<bool>(bResult);
	}

	return TOptional<bool>();
}

void FMetadataMarshaller::GetClassMetadata(const FProperty* Property, const FName& InName, const TFunctionRef<void(UClass*)>& Func)
{
	const FString& ClassName = Property->GetMetaData(InName);
	if (ClassName.IsEmpty())
	{
		return;
	}

	if (UClass* Class = FBlueprintComponentReferenceHelper::FindClassByName(ClassName))
	{
		Func(Class);
	}
}

void FMetadataMarshaller::GetClassListMetadata(const FProperty* Property, const FName& InName, const TFunctionRef<void(UClass*)>& Func)
{
	const FString& MetaDataString = Property->GetMetaData(InName);
	if (MetaDataString.IsEmpty())
	{
		return;
	}

	TArray<FString> ClassFilterNames;
	MetaDataString.ParseIntoArrayWS(ClassFilterNames, TEXT(","), true);

	for (const FString& ClassName : ClassFilterNames)
	{
		if (UClass* Class = FBlueprintComponentReferenceHelper::FindClassByName(ClassName))
		{
			if (Class->HasAnyClassFlags(CLASS_Interface) || Class->IsChildOf(UActorComponent::StaticClass()))
			{
				Func(Class);
			}
		}
	}
}
