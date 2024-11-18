﻿// Copyright 2024, Aquanox.

#include "BlueprintComponentReferenceMetadata.h"
#include "BlueprintComponentReferenceHelper.h"
#include "BlueprintComponentReferenceEditor.h"
#include "Engine/Blueprint.h"
#include "Engine/SCS_Node.h"
#include "Templates/TypeHash.h"
#include "Containers/ScriptArray.h"
#include "Misc/EngineVersionComparison.h"

#include "UObject/UObjectIterator.h"

const FName FCRMetadataKey::AllowedClasses = "AllowedClasses";
const FName FCRMetadataKey::DisallowedClasses = "DisallowedClasses";
const FName FCRMetadataKey::NoClear = "NoClear";
const FName FCRMetadataKey::NoNavigate = "NoNavigate";
const FName FCRMetadataKey::NoPicker = "NoPicker";
const FName FCRMetadataKey::ShowBlueprint = "ShowBlueprint";
const FName FCRMetadataKey::ShowNative = "ShowNative";
const FName FCRMetadataKey::ShowInstanced = "ShowInstanced";
const FName FCRMetadataKey::ShowPathOnly = "ShowPathOnly";

void FBlueprintComponentReferenceMetadata::ResetSettings()
{
	static const FBlueprintComponentReferenceMetadata DefaultValues;

	//AllowedClassesMetadata.Empty();
	//DisallowedClassesMetadata.Empty();
	AllowedClasses.Empty();
	DisallowedClasses.Empty();

	bUsePicker = DefaultValues.bUsePicker;

	bUseNavigate = DefaultValues.bUseNavigate;
	bUseClear = DefaultValues.bUseClear;

	bShowNative = DefaultValues.bShowNative;
	bShowBlueprint = DefaultValues.bShowBlueprint;
	bShowInstanced = DefaultValues.bShowInstanced;
	bShowPathOnly = DefaultValues.bShowPathOnly;
}

void FBlueprintComponentReferenceMetadata::LoadSettingsFromProperty(const FProperty* InProp)
{
	UE_LOG(LogComponentReferenceEditor, Verbose, TEXT("LoadSettingsFromProperty(%s)"), *InProp->GetFName().ToString());

	static const FBlueprintComponentReferenceMetadata DefaultValues;
	
	// picker
	bUsePicker = !HasMetaDataValue(InProp, FCRMetadataKey::NoPicker);
	// actions
	bUseNavigate = !HasMetaDataValue(InProp, FCRMetadataKey::NoNavigate);
	bUseClear = !(InProp->PropertyFlags & CPF_NoClear) && !HasMetaDataValue(InProp, FCRMetadataKey::NoClear);
	// filters
	bShowNative = GetBoolMetaDataValue(InProp, FCRMetadataKey::ShowNative, DefaultValues.bShowNative);
	bShowBlueprint = GetBoolMetaDataValue(InProp, FCRMetadataKey::ShowBlueprint, DefaultValues.bShowBlueprint);
	bShowInstanced = GetBoolMetaDataValue(InProp, FCRMetadataKey::ShowInstanced,  DefaultValues.bShowInstanced);
	bShowPathOnly = GetBoolMetaDataValue(InProp, FCRMetadataKey::ShowPathOnly,  DefaultValues.bShowPathOnly);

	GetClassListMetadata(InProp, FCRMetadataKey::AllowedClasses, [this](UClass* InClass)
	{
		AllowedClasses.AddUnique(InClass);
		//AllowedClassesMetadata.AddUnique(InClass);
	});

	GetClassListMetadata(InProp, FCRMetadataKey::DisallowedClasses, [this](UClass* InClass)
	{
		DisallowedClasses.AddUnique(InClass);
		//DisallowedClassesMetadata.AddUnique(InClass);
	});
}

void FBlueprintComponentReferenceMetadata::LoadSettingsFromProperty_Generic(const FProperty* InProp, void* TargetData, UScriptStruct* TargetType)
{
#if 0
	for (TFieldIterator<FProperty> It(TargetType); *It; ++It)
	{
		FName Specifier = *(*It)->GetMetaData("MDSpecifier");
		if (Specifier.IsNone()) continue;
		
		FName Handler = *(*It)->GetMetaData("MDHandler");
		if (Handler.IsNone()) continue;

		if (Handler == TEXT("Bool"))
		{ // standard bool value
			bool const& BaseValue = CastFieldChecked<FBoolProperty>(*It)->GetPropertyValue_InContainer(TargetType);
			bool MetadataValue = GetBoolMetaDataValue(InProp, Specifier, BaseValue);
			CastFieldChecked<FBoolProperty>(*It)->SetPropertyValue_InContainer(TargetType, MetadataValue);
		}
		else if (Handler == TEXT("InverseBool"))
		{ // inversed bool value
			bool const& BaseValue =  CastFieldChecked<FBoolProperty>(*It)->GetPropertyValue_InContainer(TargetType);
			bool MetadataValue = !GetBoolMetaDataValue(InProp, Specifier, !BaseValue);
			CastFieldChecked<FBoolProperty>(*It)->SetPropertyValue_InContainer(TargetType, MetadataValue);
		}
		else if (Handler == TEXT("BoolFlag"))
		{ // bool with no need of value, if present => true
		}
		else if (Handler == TEXT("String"))
		{
		}
		else if (Handler == TEXT("Integer"))
		{
		}
		else if (Handler == TEXT("Float"))
		{
		}
		else if (Handler == TEXT("ClassList"))
		{
			FScriptArray const& BaseValue = CastFieldChecked<FArrayProperty>(*It)->GetPropertyValue_InContainer(TargetType);

		}
	}
#endif
}

void FBlueprintComponentReferenceMetadata::ApplySettingsToProperty(UBlueprint* InBlueprint, FProperty* InProperty, const FName& InChanged)
{
	UE_LOG(LogComponentReferenceEditor, Verbose, TEXT("ApplySettingsToProperty(%s)"), *InProperty->GetName());
	
	auto BoolToString = [](bool b)
	{
		return b ? TEXT("true") : TEXT("false");
	};

	auto ArrayToString = [](const TArray<TSubclassOf<UActorComponent>>& InArray)
	{
		TArray<FString, TInlineAllocator<8>> Paths;
		for (auto& Class : InArray)
		{
			//if (!Class.IsNull() && Class.IsValid())
			if (IsValid(Class))
			{
				Paths.AddUnique(Class->GetClassPathName().ToString());
			}
		}
		return FString::Join(Paths, TEXT(","));
	};

	auto SetMetaData = [InProperty, InBlueprint](const FName& InName, const FString& InValue)
	{
		if (InProperty)
		{
			if (::IsValid(InBlueprint))
			{
				for (FBPVariableDescription& VariableDescription : InBlueprint->NewVariables)
				{
					if (VariableDescription.VarName == InProperty->GetFName())
					{
						if (!InValue.IsEmpty())
						{
							InProperty->SetMetaData(InName, *InValue);
							VariableDescription.SetMetaData(InName, InValue);
						}
						else
						{
							InProperty->RemoveMetaData(InName);
							VariableDescription.RemoveMetaData(InName);
						}

						InBlueprint->Modify();
					}
				}
			}
		}
	};

	auto SetMetaDataFlag = [InProperty, InBlueprint](const FName& InName, bool InValue)
	{
		if (InProperty)
		{
			if (::IsValid(InBlueprint))
			{
				for (FBPVariableDescription& VariableDescription : InBlueprint->NewVariables)
				{
					if (VariableDescription.VarName == InProperty->GetFName())
					{
						if (InValue)
						{
							InProperty->SetMetaData(InName, TEXT(""));
							VariableDescription.SetMetaData(InName, TEXT(""));
						}
						else
						{
							InProperty->RemoveMetaData(InName);
							VariableDescription.RemoveMetaData(InName);
						}

						InBlueprint->Modify();
					}
				}
			}
		}
	};

	if (InChanged.IsNone() || InChanged == GET_MEMBER_NAME_CHECKED(FBlueprintComponentReferenceMetadata, bUsePicker))
	{
		SetMetaDataFlag(FCRMetadataKey::NoPicker, !bUsePicker);
	}
	if (InChanged.IsNone() || InChanged == GET_MEMBER_NAME_CHECKED(FBlueprintComponentReferenceMetadata, bUseNavigate))
	{
		SetMetaDataFlag(FCRMetadataKey::NoNavigate, !bUseNavigate);
	}
	if (InChanged.IsNone() || InChanged == GET_MEMBER_NAME_CHECKED(FBlueprintComponentReferenceMetadata, bUseClear))
	{
		SetMetaDataFlag(FCRMetadataKey::NoClear, !bUseClear);
	}

	if (InChanged.IsNone() || InChanged == GET_MEMBER_NAME_CHECKED(FBlueprintComponentReferenceMetadata, bShowNative))
	{
		SetMetaData(FCRMetadataKey::ShowNative, BoolToString(bShowNative));
	}
	if (InChanged.IsNone() || InChanged == GET_MEMBER_NAME_CHECKED(FBlueprintComponentReferenceMetadata, bShowBlueprint))
	{
		SetMetaData(FCRMetadataKey::ShowBlueprint, BoolToString(bShowBlueprint));
	}
	if (InChanged.IsNone() || InChanged == GET_MEMBER_NAME_CHECKED(FBlueprintComponentReferenceMetadata, bShowInstanced))
	{
		SetMetaData(FCRMetadataKey::ShowInstanced, BoolToString(bShowInstanced));
	}
	if (InChanged.IsNone() || InChanged == GET_MEMBER_NAME_CHECKED(FBlueprintComponentReferenceMetadata, bShowPathOnly))
	{
		SetMetaData(FCRMetadataKey::ShowPathOnly, BoolToString(bShowPathOnly));
	}
	if (InChanged.IsNone() || InChanged == GET_MEMBER_NAME_CHECKED(FBlueprintComponentReferenceMetadata, AllowedClasses))
	{
		SetMetaData(FCRMetadataKey::AllowedClasses, ArrayToString(AllowedClasses));
	}
	if (InChanged.IsNone() || InChanged == GET_MEMBER_NAME_CHECKED(FBlueprintComponentReferenceMetadata, DisallowedClasses))
	{
		SetMetaData(FCRMetadataKey::DisallowedClasses, ArrayToString(DisallowedClasses));
	}
}

bool FBlueprintComponentReferenceMetadata::HasMetaDataValue(const FProperty* Property, const FName& InName)
{
	return Property->HasMetaData(InName);
}

TOptional<bool> FBlueprintComponentReferenceMetadata::GetBoolMetaDataOptional(const FProperty* Property, const FName& InName)
{
	if (Property->HasMetaData(InName))
	{
		bool bResult = true;

		const FString& ValueString = Property->GetMetaData(InName);
		if (!ValueString.IsEmpty())
		{
			if (ValueString == TEXT("true"))
			{
				bResult = true;
			}
			else if (ValueString == TEXT("false"))
			{
				bResult = false;
			}
		}

		return TOptional<bool>(bResult);
	}

	return TOptional<bool>();
}

bool FBlueprintComponentReferenceMetadata::GetBoolMetaDataValue(const FProperty* Property, const FName& InName, bool bDefaultValue)
{
	bool bResult = bDefaultValue;

	if (Property->HasMetaData(InName))
	{
		bResult = true;

		const FString& ValueString = Property->GetMetaData(InName);
		if (!ValueString.IsEmpty())
		{
			if (ValueString == TEXT("true"))
			{
				bResult = true;
			}
			else if (ValueString == TEXT("false"))
			{
				bResult = false;
			}
		}
	}

	return bResult;
}

void FBlueprintComponentReferenceMetadata::SetBoolMetaDataValue(FProperty* Property, const FName& InName, TOptional<bool> Value)
{
	if (Value.IsSet())
	{
		Property->SetMetaData(InName, Value.GetValue() ? TEXT("true") : TEXT("false"));
	}
	else
	{
		Property->RemoveMetaData(InName);
	}

	if (UObject* ParamOwner = Property->GetOwnerUObject())
	{
		ParamOwner->Modify();
	}
}

void FBlueprintComponentReferenceMetadata::GetClassListMetadata(const FProperty* Property, const FName& InName, const TFunctionRef<void(UClass*)>& Func)
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
			// If the class is an interface, expand it to be all classes in memory that implement the class.
			if (Class->HasAnyClassFlags(CLASS_Interface))
			{
				for (TObjectIterator<UClass> ClassIt; ClassIt; ++ClassIt)
				{
					UClass* const ClassWithInterface = (*ClassIt);
					if (ClassWithInterface->IsChildOf(UActorComponent::StaticClass()) && ClassWithInterface->ImplementsInterface(Class))
					{
						Func(ClassWithInterface);
					}
				}
			}
			else if (Class->IsChildOf(UActorComponent::StaticClass()))
			{
				Func(Class);
			}
		}
	}
}

void FBlueprintComponentReferenceMetadata::SetClassListMetadata(FProperty* Property, const FName& InName, const TFunctionRef<void(TArray<FString>&)>& PathSource)
{
	TArray<FString> Result;
	PathSource(Result);

	FString Complete = FString::Join(Result, TEXT(","));
	if (Complete.IsEmpty())
	{
		Property->RemoveMetaData(InName);
	}
	else
	{
		Property->SetMetaData(InName, MoveTemp(Complete));
	}

	if (UObject* ParamOwner = Property->GetOwnerUObject())
	{
		ParamOwner->Modify();
	}
}
