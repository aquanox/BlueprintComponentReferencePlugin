// Copyright 2024, Aquanox.

#pragma once

#include "MetadataMarshallerContainer.generated.h"

class FMetadataSettingsSource;

#ifndef WITH_METADATA_MARSHALLER
#define WITH_METADATA_MARSHALLER 0
#endif

/**
 * Internal struct for metadata containers
 */
USTRUCT()
struct BLUEPRINTCOMPONENTREFERENCEEDITOR_API FMetadataContainerBase
{
	GENERATED_BODY()
public:
	virtual ~FMetadataContainerBase() = default;

	virtual void ResetSettings()
	{
		unimplemented();
	}

	virtual void LoadSettingsFromType(const UStruct* InSource);
	virtual void LoadSettingsFromProperty(const FProperty* InSource);
	virtual void LoadSettings(const FMetadataSettingsSource& InSource)
	{
		unimplemented();
	}

	virtual void ApplySettingsToProperty(UBlueprint* InBlueprint, FProperty* InSource, const FName& InChanged);
	virtual void ApplySettings(FMetadataSettingsSource& InSource, const FName& InChanged)
	{
		unimplemented();
	}
};

namespace MetadataMarshallerDetail
{
	template<typename T>
	const T& GetDefaultStruct()
	{
		static const T Instance;
		return Instance;
	}

	template<typename TSrc>
	FORCEINLINE const TSrc& Modifier_None(TSrc const& Src)
	{
		return Src;
	}

	template<typename TSrc>
	FORCEINLINE TSrc Modifier_Invert(TSrc const& Src)
	{
		return !Src;
	}
}
