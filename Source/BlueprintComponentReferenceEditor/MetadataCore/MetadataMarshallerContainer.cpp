// Copyright 2024, Aquanox.

#include  "MetadataMarshallerContainer.h"
#include  "MetadataMarshallerSource.h"

void FMetadataContainerBase::LoadSettingsFromType(const UStruct* InSource)
{
	LoadSettings(FMetadataSettingsTypeSource(InSource));
}

void FMetadataContainerBase::LoadSettingsFromProperty(const FProperty* InSource)
{
	LoadSettings(FMetadataSettingsPropertySource(InSource));
}

void FMetadataContainerBase::ApplySettingsToProperty(UBlueprint* InBlueprint, FProperty* InSource, const FName& InChanged)
{
	FMetadataSettingsPropertySource Source(InSource, InBlueprint);
	ApplySettings(Source, InChanged);
}
