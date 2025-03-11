// Copyright 2024, Aquanox.

#include "BlueprintComponentReferenceMetadata.h"
#include "BlueprintComponentReferenceHelper.h"
#include "BlueprintComponentReferenceEditor.h"
#include "Engine/Blueprint.h"
#include "Engine/SCS_Node.h"
#include "Templates/TypeHash.h"
#include "Containers/ScriptArray.h"
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

	ActorClass.Reset();
	AllowedClasses.Empty();
	DisallowedClasses.Empty();

	bUsePicker = DefaultValues.bUsePicker;

	bUseNavigate = DefaultValues.bUseNavigate;
	bUseClear = DefaultValues.bUseClear;

	bShowNative = DefaultValues.bShowNative;
	bShowBlueprint = DefaultValues.bShowBlueprint;
	bShowInstanced = DefaultValues.bShowInstanced;
	bShowHidden = DefaultValues.bShowHidden;
	bShowEditor = DefaultValues.bShowEditor;
}

void FBlueprintComponentReferenceMetadata::LoadSettingsFromProperty(const FProperty* InProp)
{
	// FMetadataMarshaller::Get().LoadFromProperty(*this, InProp);
	
	UE_LOG(LogComponentReferenceEditor, Verbose, TEXT("LoadSettingsFromProperty(%s)"), *InProp->GetFName().ToString());

	static const FBlueprintComponentReferenceMetadata DefaultValues;
	
	// picker
	bUsePicker = !FMetadataMarshaller::HasMetaDataValue(InProp, FCRMetadataKey::NoPicker);
	// actions
	bUseNavigate = !FMetadataMarshaller::HasMetaDataValue(InProp, FCRMetadataKey::NoNavigate);
	bUseClear = !(InProp->PropertyFlags & CPF_NoClear) && !FMetadataMarshaller::HasMetaDataValue(InProp, FCRMetadataKey::NoClear);
	// filters
	bShowNative = FMetadataMarshaller::GetBoolMetaDataValue(InProp, FCRMetadataKey::ShowNative, DefaultValues.bShowNative);
	bShowBlueprint = FMetadataMarshaller::GetBoolMetaDataValue(InProp, FCRMetadataKey::ShowBlueprint, DefaultValues.bShowBlueprint);
	bShowInstanced = FMetadataMarshaller::GetBoolMetaDataValue(InProp, FCRMetadataKey::ShowInstanced,  DefaultValues.bShowInstanced);
	bShowHidden = FMetadataMarshaller::GetBoolMetaDataValue(InProp, FCRMetadataKey::ShowHidden,  DefaultValues.bShowHidden);
	bShowEditor = FMetadataMarshaller::GetBoolMetaDataValue(InProp, FCRMetadataKey::ShowEditor,  DefaultValues.bShowEditor);

	FMetadataMarshaller::GetClassMetadata(InProp, FCRMetadataKey::ActorClass, [this](UClass* InClass)
	{
		if (InClass && InClass->IsChildOf(AActor::StaticClass()))
		{
			ActorClass = InClass;
		}
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
	// FMetadataMarshaller::Get().ApplyToProperty(*this, InBlueprint, InProperty, InChanged);
	
	UE_LOG(LogComponentReferenceEditor, Verbose, TEXT("ApplySettingsToProperty(%s)"), *InProperty->GetName());
	
	auto BoolToString = [](bool b)
	{
		return b ? TEXT("true") : TEXT("false");
	};

	auto ClassToString = [](const UClass* InClass)
	{
		return InClass ? InClass->GetClassPathName().ToString() : TEXT("");	
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
	if (InChanged.IsNone() || InChanged == GET_MEMBER_NAME_CHECKED(FBlueprintComponentReferenceMetadata, bShowHidden))
	{
		SetMetaData(FCRMetadataKey::ShowHidden, BoolToString(bShowHidden));
	}
	if (InChanged.IsNone() || InChanged == GET_MEMBER_NAME_CHECKED(FBlueprintComponentReferenceMetadata, bShowEditor))
	{
		SetMetaData(FCRMetadataKey::ShowEditor, BoolToString(bShowEditor));
	}
	if (InChanged.IsNone() || InChanged == GET_MEMBER_NAME_CHECKED(FBlueprintComponentReferenceMetadata, AllowedClasses))
	{
		SetMetaData(FCRMetadataKey::AllowedClasses, ArrayToString(AllowedClasses));
	}
	if (InChanged.IsNone() || InChanged == GET_MEMBER_NAME_CHECKED(FBlueprintComponentReferenceMetadata, DisallowedClasses))
	{
		SetMetaData(FCRMetadataKey::DisallowedClasses, ArrayToString(DisallowedClasses));
	}
	if (InChanged.IsNone() || InChanged == GET_MEMBER_NAME_CHECKED(FBlueprintComponentReferenceMetadata, ActorClass))
	{
		SetMetaData(FCRMetadataKey::ActorClass, ActorClass.ToString());
	}
}

bool FMetadataMarshaller::HasMetaDataValue(const FProperty* Property, const FName& InName)
{
	return Property->HasMetaData(InName);
}

TOptional<bool> FMetadataMarshaller::GetBoolMetaDataOptional(const FProperty* Property, const FName& InName)
{
	if (Property->HasMetaData(InName))
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

bool FMetadataMarshaller::GetBoolMetaDataValue(const FProperty* Property, const FName& InName, bool bDefaultValue)
{
	return GetBoolMetaDataOptional(Property, InName).Get(bDefaultValue);
}

void FMetadataMarshaller::SetBoolMetaDataValue(FProperty* Property, const FName& InName, TOptional<bool> Value)
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

void FMetadataMarshaller::SetClassMetadata(FProperty* Property, const FName& InName, class UClass* InClass)
{
	FString Complete = InClass ? InClass->GetClassPathName().ToString() : TEXT("");
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

void FMetadataMarshaller::SetClassListMetadata(FProperty* Property, const FName& InName, const TFunctionRef<void(TArray<FString>&)>& PathSource)
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

#if WITH_EXPERIMENTS

FMetadataMarshaller& FMetadataMarshaller::Get()
{
	static FMetadataMarshaller Instance;
	return Instance;
}

FMetadataMarshaller::FMetadataMarshaller()
{
	// Handlers.Add(TEXT("Default"), MakeShared<FDefaultHandler>());
	// Handlers.Add(TEXT("Bool"), MakeShared<FBoolHandler>());
	// Handlers.Add(TEXT("InverseBool"), MakeShared<FInverseBoolHandler>());
	// Handlers.Add(TEXT("Class"), MakeShared<FClassHandler>());
}

void FMetadataMarshaller::ApplyInternal(FStructData Container, UBlueprint* InBlueprint, FProperty* InProperty, const FName& InChanged)
{
	TMap<FName, TOptional<FString>> MetadataToApply;
	
	for (TFieldIterator<FProperty> It(Container.ScriptStruct, EFieldIterationFlags::IncludeSuper); It; ++It)
	{
		const FName Specifier = *It->GetMetaData(TEXT("MDSpecifier"));
		if (Specifier.IsNone())
		{ // invalid declaration, specifier is required to be present 
			continue;
		}

		if (!InChanged.IsNone() && InChanged != Specifier)
		{ // update single mode
			continue;
		}
		
		const bool bIsTransient = !!( It->PropertyFlags & CPF_Transient );
		if (bIsTransient)
		{ // unset all transient things
			MetadataToApply.Add(Specifier, TOptional<FString>());
			continue;
		}
		
		const bool bIsIdentical = It->Identical_InContainer(Container.GetStructMemory(), Container.GetDefaultStructMemory());
		if (bIsIdentical)
		{ // property value is no different from default - unset the value
			MetadataToApply.Add(Specifier, TOptional<FString>());
			continue;
		}
		
		FMarshallContext Ctx;
		Ctx.ScriptStruct = const_cast<UScriptStruct*>(Container.ScriptStruct);
		Ctx.StructMemory = Container.GetStructMemory();
		Ctx.StructDefaultMemory = Container.GetDefaultStructMemory();
		Ctx.StructProperty = *It;
		
		Ctx.Blueprint = InBlueprint;
		Ctx.Property = InProperty;

		TOptional<FString> ApplyValue;

		const FName HandlerType = *It->GetMetaData(TEXT("MDHandler"));
		if (HandlerType == TEXT("Bool")) ApplyHandlerBool(Ctx, Specifier, ApplyValue);
		else if (HandlerType == TEXT("InverseBool")) ApplyHandlerInverseBool(Ctx, Specifier, ApplyValue);
		else if (HandlerType == TEXT("Class")) ApplyHandlerClass(Ctx, Specifier, ApplyValue);
		else ApplyHandlerDefault(Ctx, Specifier, ApplyValue);

		MetadataToApply.Add(Specifier, MoveTemp(ApplyValue));
	}

	auto SetMetaData = [InProperty, InBlueprint](const FName& InName, const TOptional<FString>& InValue)
	{
		if (InProperty)
		{
			if (::IsValid(InBlueprint))
			{
				for (FBPVariableDescription& VariableDescription : InBlueprint->NewVariables)
				{
					if (VariableDescription.VarName == InProperty->GetFName())
					{
						if (!InValue.IsSet())
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
					}
				}
			}
			
			if (UObject* ParamOwner = InProperty->GetOwnerUObject())
			{
				ParamOwner->Modify();
			}
		}
	};

	for (auto& ToApply : MetadataToApply)
	{
		if (!ToApply.Value.IsSet())
		{
			UE_LOG(LogTemp, Log, TEXT("%s => unset"),  *ToApply.Key.ToString());
			SetMetaData(ToApply.Key, ToApply.Value);
		}
		else
		{
			UE_LOG(LogTemp, Log, TEXT("%s => %s"),  *ToApply.Key.ToString(), *ToApply.Value.GetValue());
			SetMetaData(ToApply.Key, ToApply.Value);
		}
	}
}
// Relies on ExportText 
void FMetadataMarshaller::ApplyHandlerDefault(FMarshallContext& Context, FName MetadataKey, TOptional<FString>& OutValue)
{
	int32 PortFlags = 0;
			
	FString ValueStr;
	for (int32 Index = 0, Count = 0; Index < Context.StructProperty->ArrayDim; Index++)
	{
		FString InnerValue;
		
		if (Context.StructProperty->ExportText_InContainer(Index, InnerValue, Context.StructMemory, Context.StructMemory, nullptr, PortFlags))
		{
			if (Count++ > 0)
			{
				ValueStr += TEXT(",");
			}
			ValueStr += InnerValue;
		}
	}

	OutValue =  TOptional<FString>(ValueStr);
}
// Converts value to Bool
void FMetadataMarshaller::ApplyHandlerBool(FMarshallContext& Context, FName MetadataKey, TOptional<FString>& OutValue)
{
	if (const FBoolProperty* AsBool = CastField<FBoolProperty>(Context.StructProperty))
	{
		bool bValue = AsBool->GetPropertyValue_InContainer(Context.StructMemory);
		OutValue = bValue ? TEXT("True") : TEXT("False");
	}
}
// Converts inverted value to Bool
void FMetadataMarshaller::ApplyHandlerInverseBool(FMarshallContext& Context, FName MetadataKey, TOptional<FString>& OutValue)
{
	if (const FBoolProperty* AsBool = CastField<FBoolProperty>(Context.StructProperty))
	{
		bool bValue = AsBool->GetPropertyValue_InContainer(Context.StructMemory);
		OutValue = !bValue ? TEXT("True") : TEXT("False");
	}
}

void FMetadataMarshaller::ApplyHandlerClass(FMarshallContext& Context, FName MetadataKey, TOptional<FString>& OutValue)
{
	auto ClassToString = [](const UClass* InClass)
	{
		return InClass ? InClass->GetClassPathName().ToString() : TEXT("");	
	};

	FString Result;
	
	if (const FArrayProperty* ArrayProperty = CastField<FArrayProperty>(Context.StructProperty))
	{
		const FProperty* Inner = ArrayProperty->Inner;
		FScriptArrayHelper Helper(ArrayProperty, ArrayProperty->ContainerPtrToValuePtr<void>(Context.StructMemory));
		
		for (int32 Index = 0, Count = 0; Index < Helper.Num(); Index++)
		{
			if (Count > 0) Result += TEXT(",");

			uint8* PropData = Helper.GetRawPtr(Index);

			FString ValueStr;
			Inner->ExportTextItem_Direct(ValueStr, PropData, PropData, nullptr, PPF_None, nullptr);
			if (!ValueStr.IsEmpty())
			{
				Result += ValueStr;
				++Count;
			}
		}
	}

	if (!Result.IsEmpty())
	{
		OutValue = Result;
	}
}

void FMetadataMarshaller::ApplyHandlerClassArray(FMarshallContext& Context, FName MetadataKey, TOptional<FString>& OutValue)
{
}

void FMetadataMarshaller::LoadInternal(FStructData Container, const FProperty* InProperty)
{
	for (TFieldIterator<FProperty> It(Container.ScriptStruct); It; ++It)
	{
		const FName Specifier = *It->GetMetaData(TEXT("MDSpecifier"));
		if (Specifier.IsNone())
			continue;

		FName HandlerType = *It->GetMetaData(TEXT("MDHandler"));
		// no metadata pressent - skip load / keep default
		if (!InProperty->FindMetaData(Specifier))
			continue;
	}
}

#endif // WITH_EXPERIMENTS
