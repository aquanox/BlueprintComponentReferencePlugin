// Copyright 2024, Aquanox.

#include "BlueprintComponentReferenceMetadata.h"
#include "MetadataCore/MetadataMarshallerSource.h"
#include "BlueprintComponentReferenceHelper.h"
#include "BlueprintComponentReferenceEditor.h"
#include "UObject/UObjectIterator.h"

const FName FCRMetadataKey::ActorClass = "ActorClass";
const FName FCRMetadataKey::AllowedClasses = "AllowedClasses";
const FName FCRMetadataKey::DisallowedClasses = "DisallowedClasses";
const FName FCRMetadataKey::NoClear = "NoClear";
const FName FCRMetadataKey::NoNavigate = "NoNavigate";
const FName FCRMetadataKey::NoPicker = "NoPicker";
const FName FCRMetadataKey::ComponentViewMode = "ComponentViewMode";
const FName FCRMetadataKey::ShowBlueprint = "ShowBlueprint";
const FName FCRMetadataKey::ShowNative = "ShowNative";
const FName FCRMetadataKey::ShowInstanced = "ShowInstanced";
const FName FCRMetadataKey::ShowHidden = "ShowHidden";
const FName FCRMetadataKey::ShowEditor = "ShowEditor";
const FName FCRMetadataKey::ShowRoot = "ShowRoot";
const FName FCRMetadataKey::ComponentFilter = "ComponentFilter";


void FBlueprintComponentReferenceMetadata::ResetSettings()
{
#if !WITH_METADATA_MARSHALLER
	*this = MetadataMarshallerDetail::GetDefaultStruct<FBlueprintComponentReferenceMetadata>();
#else // WITH_METADATA_MARSHALLER
	FMetadataMarshaller::Reset<FBlueprintComponentReferenceMetadata>(*this);
#endif
}

void FBlueprintComponentReferenceMetadata::LoadSettings(const FMetadataSettingsSource& Source)
{
	using namespace MetadataMarshallerDetail;

	UE_LOG(LogComponentReferenceEditor, Verbose, TEXT("LoadSettingsFromProperty(%s)"), *Source.GetName());

#if !WITH_METADATA_MARSHALLER
#define _PROCESS_PROPERTY_LOAD(PropertyName, MetaName, GetterFn) \
	{ \
		auto ValueRead = Source.GetterFn(MetaName);  \
		if (ValueRead.IsSet()) { \
			this->PropertyName = ( ValueRead.GetValue() ); \
		} \
	}

	// picker
	_PROCESS_PROPERTY_LOAD(ComponentViewMode, FCRMetadataKey::ComponentViewMode, GetEnumValue<EBlueprintComponentReferenceViewMode>)
	if (Source.HasValue(FCRMetadataKey::NoPicker)) // handle legacy NoPicker
		ComponentViewMode = EBlueprintComponentReferenceViewMode::Off;

	// actions
	_PROCESS_PROPERTY_LOAD(bDisableNavigate, FCRMetadataKey::NoNavigate, GetFlagValue)
	_PROCESS_PROPERTY_LOAD(bDisableClear, FCRMetadataKey::NoClear, GetFlagValue)
	if (Source.IsNoClear()) // handle legacy NoClear modifier
		bDisableClear = true;

	// filters
	_PROCESS_PROPERTY_LOAD(bShowNative, FCRMetadataKey::ShowNative, GetFlagValue)
	_PROCESS_PROPERTY_LOAD(bShowBlueprint, FCRMetadataKey::ShowBlueprint, GetFlagValue)
	_PROCESS_PROPERTY_LOAD(bShowInstanced, FCRMetadataKey::ShowInstanced, GetFlagValue)
	_PROCESS_PROPERTY_LOAD(bShowHidden, FCRMetadataKey::ShowHidden, GetFlagValue)
	_PROCESS_PROPERTY_LOAD(bShowEditor, FCRMetadataKey::ShowEditor, GetFlagValue)
	_PROCESS_PROPERTY_LOAD(bShowRoot, FCRMetadataKey::ShowRoot, GetFlagValue)

	_PROCESS_PROPERTY_LOAD(ComponentFilter, FCRMetadataKey::ComponentFilter, GetStringValue)

	// externals
	_PROCESS_PROPERTY_LOAD(ActorClass, FCRMetadataKey::ComponentFilter, GetLazyClassValue<AActor>)
	_PROCESS_PROPERTY_LOAD(AllowedClasses, FCRMetadataKey::AllowedClasses, GetLazyClassListValue<UActorComponent>)
	_PROCESS_PROPERTY_LOAD(DisallowedClasses, FCRMetadataKey::DisallowedClasses, GetLazyClassListValue<UActorComponent>)

#undef _PROCESS_PROPERTY_LOAD
#else // WITH_METADATA_MARSHALLER
	FMetadataMarshaller::Load<FBlueprintComponentReferenceMetadata>(Source, *this);
#endif
}

void FBlueprintComponentReferenceMetadata::ApplySettings(FMetadataSettingsSource& Source, const FName& InChanged)
{
	using namespace MetadataMarshallerDetail;
	using ThisStruct = FBlueprintComponentReferenceMetadata;

	UE_LOG(LogComponentReferenceEditor, Verbose, TEXT("ApplySettingsToProperty(%s)"), *Source.GetName());

#if !WITH_METADATA_MARSHALLER

#define _PROCESS_PROPERTY_APPLY(PropertyName, MetaName, SetterFn) \
	if (InChanged.IsNone() || InChanged == GET_MEMBER_NAME_CHECKED(ThisStruct, PropertyName)) \
	{ \
		using PropertyType = decltype(this->PropertyName); \
		if (this->PropertyName == GetDefaultStruct<ThisStruct>().PropertyName) { \
			Source.SetterFn(MetaName, TOptional<PropertyType>() ); \
		} else { \
			Source.SetterFn(MetaName, TOptional<PropertyType>( this->PropertyName ) ); \
		} \
	}

	_PROCESS_PROPERTY_APPLY(ComponentViewMode, FCRMetadataKey::ComponentViewMode, SetEnumValue)
	_PROCESS_PROPERTY_APPLY(bDisableNavigate, FCRMetadataKey::NoNavigate, SetFlagValue)
	_PROCESS_PROPERTY_APPLY(bDisableClear, FCRMetadataKey::NoClear, SetFlagValue)

	_PROCESS_PROPERTY_APPLY(bShowNative, FCRMetadataKey::ShowNative, SetBooleanValue)
	_PROCESS_PROPERTY_APPLY(bShowBlueprint, FCRMetadataKey::ShowBlueprint, SetBooleanValue)
	_PROCESS_PROPERTY_APPLY(bShowInstanced, FCRMetadataKey::ShowInstanced, SetBooleanValue)
	_PROCESS_PROPERTY_APPLY(bShowHidden, FCRMetadataKey::ShowHidden, SetBooleanValue)
	_PROCESS_PROPERTY_APPLY(bShowEditor, FCRMetadataKey::ShowEditor, SetBooleanValue)
	_PROCESS_PROPERTY_APPLY(bShowRoot, FCRMetadataKey::ShowRoot, SetBooleanValue)

	_PROCESS_PROPERTY_APPLY(ComponentFilter, FCRMetadataKey::ComponentFilter, SetStringValue)

	_PROCESS_PROPERTY_APPLY(ActorClass, FCRMetadataKey::ActorClass, SetLazyClassValue)
	_PROCESS_PROPERTY_APPLY(AllowedClasses, FCRMetadataKey::AllowedClasses, SetLazyClassListValue)
	_PROCESS_PROPERTY_APPLY(DisallowedClasses, FCRMetadataKey::DisallowedClasses, SetLazyClassListValue)

#undef _PROCESS_PROPERTY_APPLY
#else // WITH_METADATA_MARSHALLER
	FMetadataMarshaller::Apply<FBlueprintComponentReferenceMetadata>(Source, *this, InChanged);
#endif
}
